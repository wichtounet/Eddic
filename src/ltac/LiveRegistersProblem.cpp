//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <ranges>

#include "ltac/Instruction.hpp"
#include "ltac/LiveRegistersProblem.hpp"

using namespace eddic;

using ProblemDomain = ltac::LiveRegistersProblem::ProblemDomain;
using PseudoProblemDomain = ltac::LivePseudoRegistersProblem::ProblemDomain;

namespace {

template <typename Reg, typename FloatReg, typename ProblemDomain>
void collect_jump(ltac::Instruction & instruction, ProblemDomain & in) {
    if constexpr (std::is_same_v<Reg, ltac::PseudoRegister>) {
        std::ranges::for_each(instruction.uses, in.values().inserter());
        std::ranges::for_each(instruction.float_uses, in.values().inserter());
        std::ranges::for_each(instruction.kills, in.values().eraser());
        std::ranges::for_each(instruction.float_kills, in.values().eraser());
    } else {
        std::ranges::for_each(instruction.hard_uses, in.values().inserter());
        std::ranges::for_each(instruction.hard_float_uses, in.values().inserter());
        std::ranges::for_each(instruction.hard_kills, in.values().eraser());
        std::ranges::for_each(instruction.hard_float_kills, in.values().eraser());
    }
}

template <typename Reg, typename FloatReg, typename ProblemDomain>
void collect_instruction(ltac::Instruction & instruction, ProblemDomain & in) {
    if constexpr (std::is_same_v<Reg, ltac::PseudoRegister>) {
        std::ranges::for_each(instruction.uses, in.values().inserter());
        std::ranges::for_each(instruction.float_uses, in.values().inserter());
    } else {
        std::ranges::for_each(instruction.hard_uses, in.values().inserter());
        std::ranges::for_each(instruction.hard_float_uses, in.values().inserter());
    }
}

template<typename Reg, typename FloatReg, typename ProblemDomain>
struct LivenessCollector : boost::static_visitor<> {
    ProblemDomain& in;

    explicit LivenessCollector(ProblemDomain& in) : in(in) {}

    template<typename Arg>
    inline void set_live(Arg& arg){
        if(auto* ptr = boost::relaxed_get<Reg>(&arg)){
            in.values().insert(*ptr);
        } else if(auto* ptr = boost::relaxed_get<FloatReg>(&arg)){
            in.values().insert(*ptr);
        } else if(auto* ptr = boost::relaxed_get<ltac::Address>(&arg)){
            set_live_opt(ptr->base_register);
            set_live_opt(ptr->scaled_register);
        }
    }

    template<typename Arg>
    inline void set_dead(Arg& arg){
        if(auto* ptr = boost::get<Reg>(&arg)){
            in.values().erase(*ptr);
        } else if(auto* ptr = boost::get<FloatReg>(&arg)){
            in.values().erase(*ptr);
        }
    }

    template<typename Arg>
    inline void set_live_opt(Arg& arg){
        if(arg){
            set_live(*arg);
        }
    }

    void collect(ltac::Instruction& instruction){
        if(instruction.is_jump()){
            collect_jump<Reg, FloatReg, ProblemDomain>(instruction, in);
        } else if(!instruction.is_label()) {
            if(instruction.op != ltac::Operator::NOP){
                if(ltac::erase_result_complete(instruction.op)){
                    if(auto* ptr = boost::get<ltac::Address>(&*instruction.arg1)){
                        set_live_opt(ptr->base_register);
                        set_live_opt(ptr->scaled_register);
                    } else {
                        set_dead(*instruction.arg1);
                    }
                } else {
                    set_live_opt(instruction.arg1);
                }

                set_live_opt(instruction.arg2);
                set_live_opt(instruction.arg3);
            }

            collect_instruction<Reg, FloatReg, ProblemDomain>(instruction, in);
        }
    }
};

template<typename Domain>
inline void meet(Domain& in, const Domain& out){
    if(out.top()){
        //in does not change
    } else if(in.top()){
        in = out;
    } else {
        for(auto& value : out.values().registers){
            in.values().insert(value);
        }

        for(auto& value : out.values().float_registers){
            in.values().insert(value);
        }
    }
}

} //End of anonymous namespace

ProblemDomain ltac::LiveRegistersProblem::Boundary(mtac::Function& /*function*/){
    auto value = default_element();
    return value;
}

ProblemDomain ltac::LiveRegistersProblem::Init(mtac::Function& /*function*/){
    auto value = default_element();
    return value;
}

PseudoProblemDomain ltac::LivePseudoRegistersProblem::Boundary(mtac::Function& /*function*/){
    auto value = default_element();
    return value;
}

PseudoProblemDomain ltac::LivePseudoRegistersProblem::Init(mtac::Function& /*function*/){
    auto value = default_element();
    return value;
}

void ltac::LiveRegistersProblem::meet(ProblemDomain& in, const ProblemDomain& out){
    ::meet(in, out);
}

void ltac::LivePseudoRegistersProblem::meet(PseudoProblemDomain& in, const PseudoProblemDomain& out){
    ::meet(in, out);
}

ProblemDomain ltac::LiveRegistersProblem::transfer(const mtac::basic_block_p & /*basic_block*/, ltac::Instruction& statement, ProblemDomain& in){
    auto out = in;

    LivenessCollector<ltac::Register, ltac::FloatRegister, ProblemDomain> collector(out);
    collector.collect(statement);

    return out;
}

PseudoProblemDomain ltac::LivePseudoRegistersProblem::transfer(const mtac::basic_block_p & /*basic_block*/, ltac::Instruction& statement, PseudoProblemDomain& in){
    auto out = in;

    LivenessCollector<ltac::PseudoRegister, ltac::PseudoFloatRegister, PseudoProblemDomain> collector(out);
    collector.collect(statement);

    return out;
}
