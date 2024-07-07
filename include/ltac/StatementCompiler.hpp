//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_STATEMENT_COMPILER_H
#define LTAC_STATEMENT_COMPILER_H

#include <memory>
#include <vector>

#include "PlatformDescriptor.hpp"
#include "variant.hpp"
#include "FloatPool.hpp"
#include "Options.hpp"

#include "mtac/forward.hpp"

#include "ltac/RegisterManager.hpp"

namespace eddic {

namespace ltac {

class StatementCompiler {
    public:
        StatementCompiler(Platform platform, FloatPool& float_pool, const Configuration & configuration, const PlatformDescriptor * descriptor);

        void collect_parameters(eddic::Function& definition);

        void end_bb();
        void end_bb_no_spill();

        bool ended = false;

        void compile(mtac::Quadruple& quadruple);

        void push(ltac::Argument arg);
        void pop(ltac::Argument arg);

        ltac::RegisterManager manager;

        mtac::basic_block_p bb;

    private:
        const Platform platform;
        FloatPool& float_pool;
        const Configuration & configuration;
        const PlatformDescriptor* descriptor;

        bool first_param = true;

        //Uses for the next call
        std::vector<ltac::PseudoRegister> uses;
        std::vector<ltac::PseudoFloatRegister> float_uses;

        void pass_in_int_register(mtac::Argument& argument, int position);
        void pass_in_float_register(mtac::Argument& argument, int position);

        void compare_binary(mtac::Argument& arg1, mtac::Argument& arg2);
        void compare_float_binary(mtac::Argument& arg1, mtac::Argument& arg2);
        void compare_unary(mtac::Argument arg1);

        void set_if_cc(ltac::Operator set, mtac::Quadruple& quadruple, bool floats);

        ltac::PseudoRegister to_register(const std::shared_ptr<Variable> & var);

        ltac::PseudoRegister get_address_in_pseudo_reg(const std::shared_ptr<Variable> & var, int offset);
        ltac::PseudoRegister get_address_in_pseudo_reg2(const std::shared_ptr<Variable> & var, ltac::PseudoRegister offset);

        ltac::Argument to_arg(mtac::Argument argument);

        ltac::Address address(const std::shared_ptr<Variable> & var, mtac::Argument offset);

        std::tuple<std::shared_ptr<const Type>, bool, unsigned int> common_param(mtac::Quadruple& param);
        int function_stack_size(eddic::Function& function);

        void write_8_bit(const ltac::PseudoRegister& reg, ltac::Argument arg, tac::Size size);
        void write_8_bit_to(const ltac::PseudoRegister& reg, ltac::Argument arg, tac::Size size);

        void perform_div(mtac::Quadruple& quadruple);
        void compile_DIV(mtac::Quadruple& quadruple);
        void compile_MOD(mtac::Quadruple& quadruple);

        void compile_ASSIGN(mtac::Quadruple& quadruple);
        void compile_PASSIGN(mtac::Quadruple& quadruple);
        void compile_FASSIGN(mtac::Quadruple& quadruple);
        void compile_ADD(mtac::Quadruple& quadruple);
        void compile_SUB(mtac::Quadruple& quadruple);
        void compile_MUL(mtac::Quadruple& quadruple);
        void compile_FADD(mtac::Quadruple& quadruple);
        void compile_FSUB(mtac::Quadruple& quadruple);
        void compile_FMUL(mtac::Quadruple& quadruple);
        void compile_FDIV(mtac::Quadruple& quadruple);
        void compile_EQUALS(mtac::Quadruple& quadruple);
        void compile_NOT_EQUALS(mtac::Quadruple& quadruple);
        void compile_GREATER(mtac::Quadruple& quadruple);
        void compile_GREATER_EQUALS(mtac::Quadruple& quadruple);
        void compile_LESS(mtac::Quadruple& quadruple);
        void compile_LESS_EQUALS(mtac::Quadruple& quadruple);
        void compile_FE(mtac::Quadruple& quadruple);
        void compile_FNE(mtac::Quadruple& quadruple);
        void compile_FG(mtac::Quadruple& quadruple);
        void compile_FGE(mtac::Quadruple& quadruple);
        void compile_FLE(mtac::Quadruple& quadruple);
        void compile_FL(mtac::Quadruple& quadruple);
        void compile_MINUS(mtac::Quadruple& quadruple);
        void compile_FMINUS(mtac::Quadruple& quadruple);
        void compile_I2F(mtac::Quadruple& quadruple);
        void compile_F2I(mtac::Quadruple& quadruple);
        void compile_DOT(mtac::Quadruple& quadruple);
        void compile_FDOT(mtac::Quadruple& quadruple);
        void compile_PDOT(mtac::Quadruple& quadruple);
        void compile_DOT_ASSIGN(mtac::Quadruple& quadruple);
        void compile_DOT_FASSIGN(mtac::Quadruple& quadruple);
        void compile_DOT_PASSIGN(mtac::Quadruple& quadruple);
        void compile_RETURN(mtac::Quadruple& quadruple);
        void compile_NOT(mtac::Quadruple& quadruple);
        void compile_AND(mtac::Quadruple& quadruple);
        void compile_GOTO(mtac::Quadruple& quadruple);
        void compile_PARAM(mtac::Quadruple& quadruple);
        void compile_PPARAM(mtac::Quadruple& quadruple);
        void compile_CALL(mtac::Quadruple& quadruple);
};

} //end of ltac

} //end of eddic

#endif
