//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <vector>

#include "FunctionContext.hpp"
#include "Variable.hpp"
#include "Type.hpp"
#include "VisitorUtils.hpp"

#include "mtac/remove_aliases.hpp"
#include "mtac/Utils.hpp"
#include "mtac/EscapeAnalysis.hpp"
#include "mtac/Quadruple.hpp"

using namespace eddic;

namespace {

bool is_written_once(std::shared_ptr<Variable> variable, mtac::Function& function){
    bool written = false;

    for(auto& block : function){
        for(auto& quadruple : block->statements){
            if(quadruple.op == mtac::Operator::CALL){
                if(quadruple.return1() == variable || quadruple.return2() == variable){
                    if(written){
                        return false;
                    }

                    written = true;
                }
            } else if(mtac::erase_result(quadruple.op) && quadruple.result == variable){
                if(written){
                    return false;
                }

                written = true;
            }
        }
    }

    return true;
}

bool is_not_direct_alias(std::shared_ptr<Variable> source, std::shared_ptr<Variable> target, mtac::Function& function){
    for(auto& block : function){
        for(auto& quadruple : block){
            if(quadruple.op == mtac::Operator::PASSIGN && quadruple.result == source){
                if(auto* var_ptr = boost::get<std::shared_ptr<Variable>>(&*quadruple.arg1)){
                    if(*var_ptr == target){
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

std::vector<std::shared_ptr<Variable>> get_targets(std::shared_ptr<Variable> variable, mtac::Function& function){
    std::vector<std::shared_ptr<Variable>> targets;
    
    for(auto& block : function){
        for(auto& quadruple : block->statements){
            if(quadruple.op == mtac::Operator::ASSIGN || quadruple.op == mtac::Operator::FASSIGN || quadruple.op == mtac::Operator::PASSIGN){
                if(auto* var_ptr = boost::get<std::shared_ptr<Variable>>(&*quadruple.arg1)){
                    if(*var_ptr == variable){
                        targets.push_back(quadruple.result); 
                    }
                }
            }
        }
    }

    return targets;
}

struct VariableReplace {
    std::shared_ptr<Variable> source;
    std::shared_ptr<Variable> target;

    bool find_first = false;
    bool invalid = false;
    bool reverse = false;

    VariableReplace(std::shared_ptr<Variable> source, std::shared_ptr<Variable> target) : source(source), target(target) {}
    VariableReplace(const VariableReplace& rhs) = delete;

    void guard(mtac::Quadruple& quadruple){
        if(!reverse){
            if(quadruple.result == source || quadruple.secondary == source){
                find_first = true;
                return;
            } 

            if(find_first && (quadruple.result == target || quadruple.secondary == target)){
                invalid = true;
            }
        }
    }

    inline bool optimize_optional(boost::optional<mtac::Argument>& arg){
        if(arg){
            if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*arg)){
                if(*ptr == source){
                    arg = target;
                    return true;
                }
            }
        }

        return false;
    }

    bool optimize(mtac::Quadruple& quadruple){
        guard(quadruple);
        
        if(invalid){
            return false;
        }

        bool optimized = false;

        //Do not replace source by target in the lhs of an assign
        //because it can be invalidated lated
        //and it will removed by other passes if still there
        
        if(quadruple.op == mtac::Operator::CALL){
            if(quadruple.result == source){
                quadruple.result = target;
                optimized = true;
            }
            
            if(quadruple.secondary == source){
                quadruple.secondary = target;
                optimized = true;
            }
        } else if(reverse || !mtac::erase_result(quadruple.op)){
            if(quadruple.result == source){
                quadruple.result = target;
                optimized = true;
            }
        }

        return optimized | optimize_optional(quadruple.arg1) | optimize_optional(quadruple.arg2);
    }
};

}

bool mtac::remove_aliases::operator()(mtac::Function& function){
    bool optimized = false;

    auto pointer_escaped = mtac::escape_analysis(function);

    for(auto& var : function.context->stored_variables()){
        auto position = var->position();
        auto type = var->type();

        if((position.is_temporary() || position.is_variable() || position.isStack()) && (type->is_standard_type() || type->is_pointer()) && type != STRING){
            if(is_written_once(var, function)){
                auto targets = get_targets(var, function);

                if(targets.size() == 1){
                    if(pointer_escaped->find(var) == pointer_escaped->end()){
                        if(is_not_direct_alias(var, targets.front(), function) && is_written_once(targets.front(), function) && targets.front()->type() != STRING){
                            VariableReplace replacer(var, targets.front());
                            replacer.reverse = true;

                            for(auto& block : function){
                                for(auto& statement : block->statements){
                                    optimized |= replacer.optimize(statement);
                                }
                            }

                            continue;
                        }
                    }
                } 
            }
        }
    }

    return optimized;
}
