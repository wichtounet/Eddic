//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_CONSTANT_PROPAGATION_PROBLEM_H
#define MTAC_CONSTANT_PROPAGATION_PROBLEM_H

#include <unordered_map>
#include <memory>

#include <boost/variant.hpp>

#include "mtac/DataFlowProblem.hpp"

namespace eddic {

class Variable;

namespace mtac {

typedef boost::variant<int, double, std::string> ConstantValue;
typedef std::unordered_map<std::shared_ptr<Variable>, ConstantValue> ConstantPropagationValues;

struct ConstantPropagationProblem : public DataFlowProblem<true, ConstantPropagationValues> {
    ProblemDomain meet(ProblemDomain& in, ProblemDomain& out) override;
    ProblemDomain transfer(mtac::Statement& statement, ProblemDomain& in) override;

    ProblemDomain Boundary() override;
    ProblemDomain Init() override;
};

} //end of mtac

} //end of eddic

#endif
