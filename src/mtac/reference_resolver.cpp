//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "cpp_utils/assert.hpp"

#include "Variable.hpp"
#include "iterators.hpp"
#include "FunctionContext.hpp"
#include "GlobalContext.hpp"

#include "mtac/reference_resolver.hpp"
#include "mtac/Program.hpp"
#include "mtac/Function.hpp"
#include "mtac/Quadruple.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

mtac::Argument variant_cast(Offset source){
    if (auto * ptr = boost::get<int>(&source)) {
        return *ptr;
    }

    if (auto * ptr = boost::get<std::shared_ptr<Variable>>(&source)) {
        return *ptr;
    }

    cpp_unreachable("Invalid source type");
}

void mtac::resolve_references(mtac::Program& program){
    timing_timer timer(program.context.timing(), "resolve_references");

    for(auto& function : program.functions){
        //This pass is run before basic blocks are extracted
        auto& statements = function.get_statements();

        auto it = iterate(statements);

        std::unordered_set<std::shared_ptr<Variable>> variables;

        while(it.has_next()){
            auto& quadruple = *it;
            auto op = quadruple.op;

            //x = (r)z => x = (ref(r))(z+offset(r))
            if((op == mtac::Operator::DOT || op == mtac::Operator::FDOT || op == mtac::Operator::PDOT) && mtac::optional_is<std::shared_ptr<Variable>>(quadruple.arg1)){
                auto var = boost::get<std::shared_ptr<Variable>>(*quadruple.arg1);

                if(var->is_reference()){
                    if(var->type()->is_dynamic_array()){
                        auto ptr_temp = function.context->new_temporary(var->type());

                        quadruple.arg1 = ptr_temp;

                        it.insert_no_move(mtac::Quadruple(ptr_temp, var->reference(), mtac::Operator::DOT, variant_cast(var->reference_offset())));
                    } else {
                        auto index = *quadruple.arg2;
                        auto temp = function.context->new_temporary(INT);

                        //The reference itself is replaced by its pointed var
                        quadruple.arg1 = var->reference();

                        //The new offset is the sum of the old ones
                        quadruple.arg2 = temp;

                        it.insert_no_move(mtac::Quadruple(temp, index, mtac::Operator::ADD, variant_cast(var->reference_offset())));
                    }

                    continue;
                }
            } 
            //(r)z = x => (ref(r))(z+offset(r)) = x
            else if((op == mtac::Operator::DOT_ASSIGN || op == mtac::Operator::DOT_FASSIGN)&& quadruple.result->is_reference()){
                auto var = quadruple.result;

                if(var->type()->is_dynamic_array()){
                    auto ptr_temp = function.context->new_temporary(var->type());

                    quadruple.result = ptr_temp;

                    it.insert_no_move(mtac::Quadruple(ptr_temp, var->reference(), mtac::Operator::DOT, variant_cast(var->reference_offset())));
                } else {
                    auto temp = function.context->new_temporary(INT);
                    auto index = *quadruple.arg1;

                    //The reference itself is replaced by its pointed var
                    quadruple.result = var->reference();

                    //The new offset is the sum of the old ones
                    quadruple.arg1 = temp;

                    it.insert_no_move(mtac::Quadruple(temp, index, mtac::Operator::ADD, variant_cast(var->reference_offset())));
                }

                continue;
            }

            //ref = x
            if(mtac::erase_result(quadruple.op)){
                auto result = quadruple.result;

                if(result && result->is_reference()){
                    //The first times a variable is encountered, it is its initialization
                    if(variables.find(result) == variables.end()){
                        variables.insert(result);

                        if(result->type()->is_dynamic_array()){
                            it.erase();
                            continue;
                        }
                    } else {
                        if(result->type() == FLOAT){
                            it.insert_after(mtac::Quadruple(result->reference(), variant_cast(result->reference_offset()), mtac::Operator::DOT_FASSIGN, result));
                        } else if(result->type()->is_pointer()){
                            it.insert_after(mtac::Quadruple(result->reference(), variant_cast(result->reference_offset()), mtac::Operator::DOT_PASSIGN, result));
                        } else {
                            it.insert_after(mtac::Quadruple(result->reference(), variant_cast(result->reference_offset()), mtac::Operator::DOT_ASSIGN, result));
                        }
                    }
                }
            }

            ++it;
        }
    }
}
