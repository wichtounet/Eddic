//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "assert.hpp"
#include "Variable.hpp"

#include "mtac/CopyPropagationProblem.hpp"

using namespace eddic;

typedef mtac::CopyPropagationProblem::ProblemDomain ProblemDomain;

ProblemDomain mtac::CopyPropagationProblem::meet(ProblemDomain& in, ProblemDomain& out){
    auto result = mtac::union_meet(in, out);

    //Remove all the temporary
    for(auto it = std::begin(result.values()); it != std::end(result.values());){
        if (it->second->position().isTemporary()){
            it = result.values().erase(it);
        } else {
            ++it;
        }
    }

    return result;
}

ProblemDomain mtac::CopyPropagationProblem::transfer(mtac::Statement& statement, ProblemDomain& in){
    auto out = in;

    //Only quadruple affects variable
    if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
        auto quadruple = *ptr;

        if(quadruple->op == mtac::Operator::ASSIGN || quadruple->op == mtac::Operator::FASSIGN){
            if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*quadruple->arg1)){
                out[quadruple->result] = *ptr;
            } else {
                //The result is not constant at this point
                out.erase(quadruple->result);
            }
        } else {
            auto op = quadruple->op;

            //Check if the operator erase the contents of the result variable
            if(op != mtac::Operator::ARRAY_ASSIGN && op != mtac::Operator::DOT_ASSIGN && op != mtac::Operator::RETURN){
                //The result is not constant at this point
                out.erase(quadruple->result);
            }
        }
    }

    return out;
}

bool optimize_arg(mtac::Argument* arg, ProblemDomain& results){
    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(arg)){
        if(results.find(*ptr) != results.end()){
            *arg = results[*ptr];
            return true;
        }
    }

    return false;
}

bool optimize_optional(boost::optional<mtac::Argument>& arg, ProblemDomain& results){
    if(arg){
        return optimize_arg(&*arg, results);
    }

    return false;
}

bool mtac::CopyPropagationProblem::optimize(mtac::Statement& statement, std::shared_ptr<mtac::DataFlowResults<ProblemDomain>>& global_results){
    auto& results = global_results->IN_S[statement];

    bool changes = false;

    if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
        auto& quadruple = *ptr;

        //Do not replace a variable by a constant when used in offset
        if(quadruple->op != mtac::Operator::ARRAY && quadruple->op != mtac::Operator::DOT){
            changes |= optimize_optional(quadruple->arg1, results);
        }

        changes |= optimize_optional(quadruple->arg2, results);
    } else if(auto* ptr = boost::get<std::shared_ptr<mtac::Param>>(&statement)){
        auto& param = *ptr;

        changes |= optimize_arg(&param->arg, results);
    } else if(auto* ptr = boost::get<std::shared_ptr<mtac::IfFalse>>(&statement)){
        auto& ifFalse = *ptr;
        
        changes |= optimize_arg(&ifFalse->arg1, results);
        changes |= optimize_optional(ifFalse->arg2, results);
    } else if(auto* ptr = boost::get<std::shared_ptr<mtac::If>>(&statement)){
        auto& if_ = *ptr;
        
        changes |= optimize_arg(&if_->arg1, results);
        changes |= optimize_optional(if_->arg2, results);
    }

    return changes;
}
