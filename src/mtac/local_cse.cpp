//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "FunctionContext.hpp"
#include "Variable.hpp"

#include "mtac/Argument.hpp"
#include "mtac/local_cse.hpp"
#include "mtac/Function.hpp"
#include "mtac/Quadruple.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

namespace {

struct expression {
    std::size_t uid;
    mtac::Argument arg1;
    mtac::Argument arg2;
    mtac::Operator op;
    std::shared_ptr<Variable> tmp;

    expression(std::size_t uid, mtac::Argument arg1, mtac::Argument arg2, mtac::Operator op, std::shared_ptr<Variable> tmp) : uid(uid), arg1(arg1), arg2(arg2), op(op), tmp(tmp) {
        //Nothing
    }
};

bool are_equivalent(mtac::Quadruple& quadruple, expression& exp){
    if(exp.op == quadruple.op){
        if(exp.arg1 == *quadruple.arg1 && exp.arg2 == *quadruple.arg2){
            return true;
        } else if(mtac::is_distributive(quadruple.op) && exp.arg1 == *quadruple.arg2 && exp.arg2 == *quadruple.arg1){
            return true;
        }
    }

    return false;
}

}

bool mtac::local_cse::operator()(mtac::Function& function){
    bool optimized = false;
    
    for(auto& block : function){
        auto it = block->statements.begin();

        std::vector<expression> expressions;

        while(it != block->statements.end()){
            auto& quadruple = *it;
                
            if(mtac::is_expression(quadruple.op)){
                bool found = false;

                for(auto& exp : expressions){
                    if(are_equivalent(quadruple, exp)){
                        found = true;
                                
                        function.context->global()->stats().inc_counter("local_cse");

                        if(!exp.tmp){
                            auto tmp = function.context->new_temporary(quadruple.result->type() == INT ? INT : FLOAT);
                            exp.tmp = tmp;

                            auto current_uid = quadruple.uid();
                            quadruple.result = tmp;

                            auto old_it = block->statements.begin();
                            while(old_it->uid() != exp.uid){
                                ++old_it;
                            }

                            old_it->result = tmp;
                            block->statements.insert(old_it, mtac::Quadruple(tmp, exp.arg1, exp.op, exp.arg2));

                            it = block->statements.begin();
                            while(it->uid() != current_uid){
                                ++it;
                            }
                        } else {
                            quadruple.result = exp.tmp;
                        }

                        break;
                    }
                }

                if(!found){
                    expressions.emplace_back(quadruple.uid(), *quadruple.arg1, *quadruple.arg2, quadruple.op, nullptr);
                }
            }
            
            if(mtac::erase_result(quadruple.op)){
                auto eit = iterate(expressions);

                while(eit.has_next()){
                    auto& expression = *eit;

                    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&expression.arg1)){
                        if(quadruple.result == *ptr){
                            eit.erase();
                            continue;
                        }
                    }
                    
                    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&expression.arg2)){
                        if(quadruple.result == *ptr){
                            eit.erase();
                            continue;
                        }
                    }

                    ++eit;
                }
            }

            ++it;
        }
    }

    return optimized;
}
