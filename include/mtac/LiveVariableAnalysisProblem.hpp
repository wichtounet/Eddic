//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_LIVE_VARIABLE_ANALYSIS_PROBLEM_H
#define MTAC_LIVE_VARIABLE_ANALYSIS_PROBLEM_H

#include <unordered_set>
#include <memory>

#include "mtac/DataFlowProblem.hpp"

namespace eddic {

class Variable;

namespace mtac {
    
typedef std::unordered_set<std::shared_ptr<Variable>> Values;

struct LiveVariableValues {
    Values values;
    std::shared_ptr<Values> pointer_escaped;

    void insert(std::shared_ptr<Variable> var){
        values.insert(var);
    }
    
    void erase(std::shared_ptr<Variable> var){
        values.erase(var);
    }

    Values::iterator find(std::shared_ptr<Variable> var){
        return values.find(var);
    }
    
    Values::iterator begin(){
        return values.begin();
    }
    
    Values::iterator end(){
        return values.end();
    }

    std::size_t size(){
        return values.size();
    }
};

std::ostream& operator<<(std::ostream& stream, LiveVariableValues& expression);

struct LiveVariableAnalysisProblem : public DataFlowProblem<DataFlowType::Backward, LiveVariableValues> {
    std::shared_ptr<Values> pointer_escaped;
    Values escaped_variables;
    
    LiveVariableAnalysisProblem();

    void Gather(std::shared_ptr<mtac::Function> function) override;
    
    ProblemDomain Boundary(std::shared_ptr<mtac::Function> function) override;
    ProblemDomain Init(std::shared_ptr<mtac::Function> function) override;
   
    ProblemDomain meet(ProblemDomain& in, ProblemDomain& out) override;
    ProblemDomain transfer(std::shared_ptr<mtac::BasicBlock> basic_block, mtac::Statement& statement, ProblemDomain& in) override;
    
    bool optimize(mtac::Statement& statement, std::shared_ptr<DataFlowResults<ProblemDomain>>& results) override;
};

} //end of mtac

} //end of eddic

#endif
