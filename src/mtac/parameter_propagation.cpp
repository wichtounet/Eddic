//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <unordered_map>
#include <vector>

#include "logging.hpp"
#include "Function.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "Variable.hpp"

#include "mtac/parameter_propagation.hpp"
#include "mtac/Program.hpp"
#include "mtac/Quadruple.hpp"
#include "mtac/Function.hpp"
#include "mtac/VariableReplace.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

namespace {

typedef std::unordered_map<std::string, std::vector<std::unordered_map<std::size_t, mtac::Argument>>> Arguments;

Arguments collect_arguments(mtac::Program& program){
    Arguments arguments;

    for(auto& function : program.functions){
        for(auto& block : function){
            for(auto& quadruple : block){
                if(quadruple.op == mtac::Operator::CALL){
                    auto& function = quadruple.function();

                    if(!function.standard() && !function.parameters().empty()){
                        std::unordered_map<std::size_t, mtac::Argument> function_arguments;

                        auto parameters = function.parameters().size();

                        mtac::basic_block::reverse_iterator it;
                        mtac::basic_block::reverse_iterator end;

                        if(block->statements.front() == quadruple){
                            it = block->prev->statements.rbegin();
                            end = block->prev->statements.rend();
                        } else {
                            it = block->statements.rbegin();
                            end = block->statements.rend();

                            while(*it != quadruple){
                                ++it;
                            }
                        }

                        std::size_t discovered = 0;

                        while(it != end && discovered < parameters){
                            auto& param_quadruple = *it;

                            if(param_quadruple.op == mtac::Operator::PARAM || param_quadruple.op == mtac::Operator::PPARAM){
                                if(param_quadruple.param()->type() == INT){
                                    function_arguments[discovered] = *param_quadruple.arg1;
                                }

                                ++discovered;
                            }

                            ++it;
                        }

                        arguments[function.mangled_name()].push_back(std::move(function_arguments));
                    }
                }
            }
        }
    }
    
    return arguments;
}

} //end of anonymous namespace 

bool mtac::parameter_propagation::operator()(mtac::Program& program){
    bool optimized = false;

    auto arguments = collect_arguments(program);

    for(auto& function_map : arguments){
        const auto& function_name = function_map.first;
        auto& function = program.context.getFunction(function_name);
        auto& function_arguments = function_map.second;

        std::vector<std::pair<std::size_t, int>> constant_parameters;

        for(std::size_t i = 0; i < function.parameters().size(); ++i){
            bool found = false;
            int constant_value = 0;

            for(auto& arguments : function_arguments){
                auto& arg = arguments[i];

                if(auto* ptr = boost::get<int>(&arg)){
                    if(found){
                        if(*ptr != constant_value){
                            found = false;
                            break;
                        }
                    } else {
                        found = true;
                        constant_value = *ptr;
                    }
                } else {
                    found = false;
                    break;
                }
            }

            if(found){
                constant_parameters.emplace_back(i, constant_value);
            }
        }

        if(!constant_parameters.empty()){
            std::sort(constant_parameters.begin(), constant_parameters.end(), 
                    [](const std::pair<int, int>& p1, const std::pair<int, int>& p2){ return p1.first > p2.first; });

            //Replace the parameter by the constant in each use of the parameter
            auto& mtac_function = program.mtac_function(function);
            mtac::VariableClones clones;

            for(auto& parameter : constant_parameters){
                auto param = mtac_function.context->getVariable(function.parameter(parameter.first).name());

                log::emit<Debug>("Optimizer") << "Propagate " << param->name() << " by " << parameter.second  << " in function " << function.name() << log::endl;
                program.context.stats().inc_counter("propagated_parameter");

                clones[param] = parameter.second;
            }

            VariableReplace replacer(clones);
            for(auto& block : mtac_function){
                for(auto& quadruple : block){
                    replacer.replace(quadruple);
                }
            }
            
            for(auto& parameter : constant_parameters){
                auto param = function.context()->getVariable(function.parameter(parameter.first).name());
                function.context()->removeVariable(param); 
            }

            for(auto& parameter : constant_parameters){
                //Remove the parameter passing for each call to the function
                for(auto& mtac_function : program.functions){
                    for(auto& block : mtac_function){
                        for(auto& quadruple : block){
                            if(quadruple.op == mtac::Operator::CALL){
                                auto& param_function = quadruple.function();

                                if(param_function == function){
                                    mtac::basic_block_p param_block;
                                    mtac::basic_block::reverse_iterator it;
                                    mtac::basic_block::reverse_iterator end;

                                    if(block->statements.front() == quadruple){
                                        param_block = block->prev;
                                        it = block->prev->statements.rbegin();
                                        end = block->prev->statements.rend();
                                    } else {
                                        param_block = block;
                                        it = block->statements.rbegin();
                                        end = block->statements.rend();

                                        while(*it != quadruple){
                                            ++it;
                                        }
                                    }

                                    std::size_t discovered = 0;

                                    while(it != end && discovered < function.parameters().size()){
                                        auto& param_quadruple = *it;

                                        if(param_quadruple.op == mtac::Operator::PARAM || param_quadruple.op == mtac::Operator::PPARAM){
                                            if(discovered == parameter.first){
                                                mtac::transform_to_nop(param_quadruple);
                                                optimized = true;
                                            }

                                            ++discovered;
                                        }

                                        ++it;
                                    }
                                }
                            }
                        }
                    }
                }

                //Remove the parameter from the function definition
                function.parameters().erase(function.parameters().begin() + parameter.first);
            }
        }
    }

    return optimized;
}
