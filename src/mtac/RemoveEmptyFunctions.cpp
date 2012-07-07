//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <vector>

#include "mtac/RemoveEmptyFunctions.hpp"

using namespace eddic;

bool mtac::remove_empty_functions(std::shared_ptr<mtac::Program> program){
    bool optimized = false;

    std::vector<std::string> removed_functions;

    auto it = program->functions.begin();

    while(it != program->functions.end()){
        auto function = *it;

        unsigned int statements = 0;

        for(auto& block : function->getBasicBlocks()){
            statements += block->statements.size();
        }

        if(statements == 0){
            removed_functions.push_back(function->getName());
            it = program->functions.erase(it);
        } else {
            ++it;
        }
    }

    if(!removed_functions.empty()){
        for(auto& function : program->functions){
            auto& blocks = function->getBasicBlocks();

            auto bit = blocks.begin();

            while(bit != blocks.end()){
                auto block = *bit;

                auto fit = block->statements.begin();

                while(fit != block->statements.end()){
                    auto statement = *fit;

                    if(auto* ptr = boost::get<std::shared_ptr<mtac::Call>>(&statement)){
                        auto function = (*ptr)->function;

                        if(std::find(removed_functions.begin(), removed_functions.end(), function) != removed_functions.end()){
                            int parameters = (*ptr)->functionDefinition->parameters.size();

                            if(parameters > 0){
                                //The parameters are in the previous block
                                if(fit == block->statements.begin()){
                                    auto pit = bit;
                                    --pit;
                                    auto previous = *pit;

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

                ++bit;
            }
        }
    }

    return optimized;
}

