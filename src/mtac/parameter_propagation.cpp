//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <unordered_map>
#include <vector>

#include "Function.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"

#include "mtac/parameter_propagation.hpp"
#include "mtac/Program.hpp"
#include "mtac/Function.hpp"
#include "mtac/Call.hpp"
#include "mtac/Argument.hpp"
#include "mtac/Param.hpp"
#include "mtac/VariableReplace.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

namespace {

typedef std::unordered_map<std::string, std::vector<std::vector<mtac::Argument>>> Arguments;

Arguments collect_arguments(mtac::Program& program){
    Arguments arguments;

    for(auto& function : program.functions){
        for(auto& block : function){
            for(auto& statement : block){
                if(auto* ptr = boost::get<std::shared_ptr<mtac::Call>>(&statement)){
                    auto& function = (*ptr)->functionDefinition;

                    if(!function.standard && !function.parameters().empty()){
                        std::vector<mtac::Argument> function_arguments;

                        auto parameters = function.parameters().size();
                        auto param_block = block->prev;

                        auto it = param_block->statements.rbegin();
                        auto end = param_block->statements.rend();

                        auto discovered = 0;

                        while(it != end && discovered < parameters){
                            auto& param_statement = *it;

                            if(auto* param_ptr = boost::get<std::shared_ptr<mtac::Param>>(&param_statement)){
                                ++discovered;

                                function_arguments.push_back((*param_ptr)->arg);
                            }

                            ++it;
                        }
                        
                        arguments[function.mangledName].push_back(std::move(function_arguments));
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

    auto global_context = program.context;

    auto arguments = collect_arguments(program);

    for(auto& function_map : arguments){
        auto& function_name = function_map.first;
        auto& function = global_context->getFunction(function_name);
        auto& function_arguments = function_map.second;

        std::vector<std::pair<int, int>> constant_parameters;

        for(int i = 0; i < function.parameters().size(); ++i){
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
            std::sort(constant_parameters.begin(), constant_parameters.end(), [](const std::pair<int, int>& p1, const std::pair<int, int>& p2){ return p1.first > p2.first; });

            for(auto& parameter : constant_parameters){
                //Remove the parameter passing for each call to the function
                for(auto& mtac_function : program.functions){
                    for(auto& block : mtac_function){
                        for(auto& statement : block){
                            if(auto* ptr = boost::get<std::shared_ptr<mtac::Call>>(&statement)){
                                auto& param_function = (*ptr)->functionDefinition;

                                if(param_function == function){
                                    auto param_block = block->prev;

                                    auto it = param_block->statements.rbegin();
                                    auto end = param_block->statements.rend();

                                    auto discovered = 0;

                                    while(it != end && discovered < function.parameters().size()){
                                        auto& param_statement = *it;

                                        if(auto* param_ptr = boost::get<std::shared_ptr<mtac::Param>>(&param_statement)){
                                            if(discovered == parameter.first){
                                                param_block->statements.erase(--(it.base()));
                                                --it;
                                                end = param_block->statements.rend();
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

            //Replace the parameter by the constant in each use of the parameter
            for(auto& mtac_function : program.functions){
                if(mtac_function.definition() == function){
                    mtac::VariableClones clones;
                    
                    for(auto& parameter : constant_parameters){
                        clones[mtac_function.context->getVariable(function.parameters()[parameter.first].name)] = parameter.second;
                    }

                    VariableReplace replacer(clones);
                    mtac::visit_all_statements(replacer, mtac_function);
                }
            }
        }
    }

    return optimized;
}
