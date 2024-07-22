//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "iterators.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "logging.hpp"

#include "mtac/loop.hpp"
#include "mtac/remove_empty_loops.hpp"
#include "mtac/Function.hpp"
#include "mtac/ControlFlowGraph.hpp"
#include "mtac/Utils.hpp"
#include "mtac/Quadruple.hpp"

using namespace eddic;

bool mtac::remove_empty_loops::operator()(mtac::Function& function){
    if(function.loops().empty()){
        return false;
    }

    bool optimized = false;
    
    auto lit = iterate(function.loops());

    while(lit.has_next()){
        auto loop = *lit;

        if(loop.has_estimate() && loop.blocks().size() == 1){
            auto it = loop.estimate();
            auto bb = *loop.begin();

            if(bb->size_no_nop() == 2){
                std::shared_ptr<Variable> result;
                for(auto&  statement : bb->statements){
                    if(statement.op != mtac::Operator::NOP){
                       result = statement.result; 
                       break;
                    }
                }

                auto& basic_induction_variables = loop.basic_induction_variables();
                if(basic_induction_variables.find(result) != basic_induction_variables.end()){
                    auto linear_equation = basic_induction_variables.begin()->second;
                    auto initial_value = loop.initial_value();

                    bool loop_removed = false;

                    //The loop does not iterate
                    if(it == 0){
                        bb->statements.clear();
                        loop_removed = true;
                    } else if(it > 0){
                        bb->statements.clear();
                        loop_removed = true;

                        bb->emplace_back(result, static_cast<int>(initial_value + it * linear_equation.d), mtac::Operator::ASSIGN);
                    }

                    if(loop_removed){
                        function.context->global().stats().inc_counter("empty_loop_removed");

                        //It is not a loop anymore
                        mtac::remove_edge(bb, bb);

                        lit.erase();

                        optimized = true;

                        continue;
                    }
                }
            }
        }

        ++lit;
    }

    return optimized;
}
