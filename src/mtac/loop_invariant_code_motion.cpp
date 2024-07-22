//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <map>
#include <unordered_map>

#include "iterators.hpp"
#include "VisitorUtils.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "logging.hpp"
#include "Variable.hpp"

#include "mtac/loop.hpp"
#include "mtac/loop_invariant_code_motion.hpp"
#include "mtac/Function.hpp"
#include "mtac/ControlFlowGraph.hpp"
#include "mtac/Utils.hpp"
#include "mtac/variable_usage.hpp"
#include "mtac/Quadruple.hpp"

using namespace eddic;

namespace {

bool is_invariant(boost::optional<mtac::Argument>& argument, mtac::Usage& usage){
    if(argument){
        if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*argument)){
            return usage.written[*ptr] == 0;
        }
    }

    return true;
}

bool is_invariant(mtac::Quadruple& quadruple, mtac::Usage& usage){
    if(mtac::erase_result(quadruple.op)){
        //If there are more than one write to this variable, the computation is not invariant
        if(usage.written[quadruple.result] > 1){
            return false;
        }

        return is_invariant(quadruple.arg1, usage) && is_invariant(quadruple.arg2, usage);
    }

    return false;
}

bool is_arithmetic_expression(mtac::Operator op){
    return op >= mtac::Operator::ADD && op <= mtac::Operator::FDIV;
}

/*!
 * \brief Test if an invariant is valid or not.
 * An invariant defining v is valid if:
 * 1. It is in a basic block that dominates all other uses of v
 * 2. It is in a basic block that dominates all exit blocks of the loop
 * 3. It is not an NOP
 */
bool is_valid_invariant(mtac::basic_block_p source_bb, mtac::Quadruple& quadruple, mtac::loop& loop){
    //It is not necessary to move statements with no effects.
    if(quadruple.op == mtac::Operator::NOP){
        return false;
    }

    auto var = quadruple.result;

    for(auto& bb : loop){
        //A bb always dominates itself => no need to consider the source basic block
        if(bb != source_bb){
            if(use_variable(bb, var)){
                auto dominator = bb->dominator;

                //If the bb is not dominated by the source bb, it is not valid
                if(dominator != source_bb){
                    return false;
                }
            }
        }
    }

    //An arithmetic expression has no side effects and so the last condition can
    //be relaxed
    if(is_arithmetic_expression(quadruple.op)){
        return true;
    }

    auto exit_block = loop.find_exit();

    if(exit_block == source_bb){
        return true;
    }

    auto dominator = exit_block->dominator;

    //If the exit bb is not dominated by the source bb, it is not valid
    if(dominator != source_bb){
        return false;
    }

    return true;
}

bool loop_invariant_code_motion(mtac::loop& loop, mtac::Function& function){
    mtac::basic_block_p pre_header;

    bool optimized = false;

    auto usage = compute_write_usage(loop);

    for(auto& bb : loop){
        for(auto& statement : bb->statements){
            if(is_invariant(statement, usage)){
                LOG<Trace>("ICM") << "Found invariant " << statement << log::endl;

                if(is_valid_invariant(bb, statement, loop)){
                    LOG<Trace>("ICM") << "Found valid invariant " << statement << log::endl;

                    //Create the preheader if necessary
                    if(!pre_header){
                        pre_header = loop.find_safe_preheader(function, true);
                    }

                    function.context->global().stats().inc_counter("invariant_moved");

                    pre_header->statements.push_back(statement);
                    mtac::transform_to_nop(statement);

                    optimized = true;
                }
            }
        }
    }

    return optimized;
}

} //end of anonymous namespace

bool mtac::loop_invariant_code_motion::operator()(mtac::Function& function){
    if(function.loops().empty()){
        return false;
    }

    bool optimized = false;

    for(auto& loop : function.loops()){
        optimized |= ::loop_invariant_code_motion(loop, function);
    }

    return optimized;
}
