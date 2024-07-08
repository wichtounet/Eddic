//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_CONSTANT_PROPAGATION_PROBLEM_H
#define MTAC_CONSTANT_PROPAGATION_PROBLEM_H

#include <unordered_map>
#include <memory>

#include "variant.hpp"

#include "mtac/pass_traits.hpp"
#include "mtac/DataFlowProblem.hpp"
#include "mtac/EscapeAnalysis.hpp"

namespace eddic {

class Variable;

namespace mtac {

typedef boost::variant<std::string, double, int, std::shared_ptr<Variable>> ConstantValue;

class ConstantPropagationLattice {
    public:
        ConstantPropagationLattice(){}; //NAC
        ConstantPropagationLattice(ConstantValue value) : m_value(value) {}

        ConstantPropagationLattice(const ConstantPropagationLattice& rhs) : m_value(rhs.m_value) {}
        ConstantPropagationLattice& operator=(const ConstantPropagationLattice& rhs){
            m_value = rhs.m_value;

            return *this;
        }

        ConstantPropagationLattice(ConstantPropagationLattice&& rhs) : m_value(std::move(rhs.m_value)) {}
        ConstantPropagationLattice& operator=(ConstantPropagationLattice&& rhs){
            m_value = std::move(rhs.m_value);

            return *this;
        }

        ConstantValue& value(){
            return *m_value;
        }

        ConstantValue value() const {
            return *m_value;
        }

        bool constant() const {
            return static_cast<bool>(m_value);
        }

        bool nac() const {
            return !constant();
        }

        void set_nac(){
            m_value = boost::none;
        }

    private:
        boost::optional<ConstantValue> m_value;
};

typedef std::unordered_map<std::shared_ptr<Variable>, ConstantPropagationLattice> ConstantPropagationValues;

class ConstantPropagationProblem {
    public:
        //The type of data managed
        typedef Domain<ConstantPropagationValues> ProblemDomain;

        //The direction
        STATIC_CONSTANT(DataFlowType, Type, DataFlowType::Fast_Forward);
        STATIC_CONSTANT(bool, Low, false);

        ProblemDomain Init(mtac::Function& function);
        ProblemDomain Boundary(mtac::Function& function);

        void meet(ProblemDomain& in, const ProblemDomain& out);
        void transfer(const mtac::basic_block_p & basic_block, mtac::Quadruple& quadruple, ProblemDomain& in);
        bool optimize(mtac::Function& function, std::shared_ptr<DataFlowResults<ProblemDomain>> global_results);

    private:
        ProblemDomain top_element();
        ProblemDomain default_element();

        mtac::escaped_variables_ptr pointer_escaped;
};

bool operator==(const mtac::Domain<ConstantPropagationValues>& lhs, const mtac::Domain<ConstantPropagationValues>& rhs);
bool operator!=(const mtac::Domain<ConstantPropagationValues>& lhs, const mtac::Domain<ConstantPropagationValues>& rhs);

template<>
struct pass_traits<ConstantPropagationProblem> {
    STATIC_CONSTANT(pass_type, type, pass_type::DATA_FLOW);
    STATIC_STRING(name, "constant_propagation");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);
};

std::ostream& operator<<(std::ostream& stream, const ConstantPropagationLattice& lattice);

} //end of mtac

} //end of eddic

#endif
