//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "logging.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"

#include "mtac/Function.hpp"
#include "mtac/loop.hpp"
#include "mtac/loop_unrolling.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

bool mtac::loop_unrolling::gate(std::shared_ptr<Configuration> configuration){
    return configuration->option_defined("funroll-loops");
}

bool mtac::loop_unrolling::operator()(mtac::Function& function){
    if(function.loops().empty()){
        return false;
    }

    bool optimized = false;

    for(auto& loop : function.loops()){
        if(loop.has_estimate() && loop.blocks().size() == 1){
            auto it = loop.estimate();

            if(it > 100){
                auto bb = *loop.begin();

                //Do not increase too much the size of the body
                if(bb->statements.size() < 20){
                    unsigned int factor = 0;
                    if((it % 8) == 0){
                        factor = 8;
                    } else if((it % 4) == 0){
                        factor = 4;
                    } else if((it % 2) == 0){
                        factor = 2;
                    }

                    if(factor == 0){
                        continue;
                    }

                    LOG<Trace>("loops") << "Unroll the loop with a factor " << factor << log::endl;
                    function.context->global().stats().inc_counter("loop_unrolled");

                    optimized = true;

                    auto& statements = bb->statements;

                    //The comparison is not necessary here anymore
                    auto comparison = statements.back();
                    statements.pop_back();

                    int limit = statements.size();

                    //There are perhaps new references to functions
                    for(auto& statement : statements){
                        if(statement.op == mtac::Operator::CALL){
                            program.cg.edge(function.definition(), statement.function())->count += (factor - 1);
                        }
                    }

                    //Save enough space for the new statements
                    statements.reserve(limit * factor + 1);

                    //Start at 1 because there is already the original body
                    for(unsigned int i = 1; i < factor; ++i){
                        for(int j = 0; j < limit; ++j){
                            statements.push_back(statements[j]);
                        }
                    }

                    //Put the comparison again at the end
                    statements.push_back(comparison);
                }
            }
        }
    }

    return optimized;
}
