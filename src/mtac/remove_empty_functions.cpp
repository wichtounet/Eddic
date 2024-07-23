//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <vector>

#include "iterators.hpp"
#include "logging.hpp"
#include "GlobalContext.hpp"

#include "mtac/remove_empty_functions.hpp"
#include "mtac/Utils.hpp"
#include "mtac/Quadruple.hpp"

using namespace eddic;

bool mtac::remove_empty_functions::operator()(mtac::Program& program){
    std::vector<std::string> removed_functions;

    program.functions.erase(std::remove_if(program.functions.begin(), program.functions.end(), 
        [&program,&removed_functions](auto& function){
            if(!function.is_main() && function.size_no_nop() == 0){
                program.context.stats().inc_counter("empty_function_removed");
                LOG<Debug>("Optimizer") << "Remove empty function " << function.get_name() << log::endl;
                
                removed_functions.push_back(function.get_name());

                return true;
            }

            return false;
        }), program.functions.end());

    if(!removed_functions.empty()){
        for(auto& function : program.functions){
            for(auto& block : function){
                auto fit = block->statements.begin();

                while(fit != block->statements.end()){
                    auto& quadruple = *fit;

                    if(quadruple.op == mtac::Operator::CALL){
                        auto function_name = quadruple.function().mangled_name();

                        if(std::find(removed_functions.begin(), removed_functions.end(), function_name) != removed_functions.end()){
                            //Update the call graph
                            --program.cg.edge(function.definition(), quadruple.function())->count;

                            int parameters = quadruple.function().parameters().size();

                            if(parameters > 0){
                                //The parameters are in the previous block
                                if(fit == block->statements.begin()){
                                    auto previous = block->prev;

                                    auto fend = previous->statements.end();
                                    --fend;

                                    while(parameters > 0){
                                        fend = previous->statements.erase(fend);
                                        --fend;

                                        --parameters;
                                    }

                                    fit = block->statements.erase(fit);
                                }
                                //The parameters are in the same block
                                else {
                                    while(parameters >= 0){
                                        fit = block->statements.erase(fit);
                                        --fit;

                                        --parameters;
                                    }
                                }

                            } else {
                                fit = block->statements.erase(fit);
                            }

                            continue;
                        }
                    }

                    ++fit;
                }
            }
        }
    }

    return !removed_functions.empty();
}
