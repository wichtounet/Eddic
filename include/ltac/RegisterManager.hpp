//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_REGISTER_MANAGER_H
#define LTAC_REGISTER_MANAGER_H

#include <memory>
#include <vector>
#include <unordered_set>

#include "Platform.hpp"
#include "FloatPool.hpp"

#include "mtac/EscapeAnalysis.hpp"
#include "mtac/forward.hpp"
#include "mtac/Argument.hpp"

//Forward is not enough for PseudoRegisters
#include "ltac/Argument.hpp"
#include "ltac/Operator.hpp"
#include "ltac/PseudoRegister.hpp"
#include "ltac/PseudoFloatRegister.hpp"
#include "ltac/Instruction.hpp"

#include "asm/PseudoRegisters.hpp"

namespace eddic {

namespace ltac {

class StatementCompiler;

class RegisterManager {
    public:
        std::unordered_set<std::shared_ptr<Variable>> written;
        std::unordered_set<std::shared_ptr<Variable>> local;

        mtac::escaped_variables_ptr pointer_escaped;

        mtac::basic_block_p bb;

        RegisterManager(FloatPool& float_pool);

        /*!
         * Deleted copy constructor
         */
        RegisterManager(const RegisterManager& rhs) = delete;

        /*!
         * Deleted copy assignment operator. 
         */
        RegisterManager& operator=(const RegisterManager& rhs) = delete;

        void reset();

        ltac::PseudoRegister get_pseudo_reg(std::shared_ptr<Variable> var);
        ltac::PseudoRegister get_pseudo_reg_no_move(std::shared_ptr<Variable> var);
        ltac::PseudoFloatRegister get_pseudo_float_reg(std::shared_ptr<Variable> var);
        ltac::PseudoFloatRegister get_pseudo_float_reg_no_move(std::shared_ptr<Variable> var);

        void copy(mtac::Argument argument, ltac::PseudoFloatRegister reg);
        void copy(mtac::Argument argument, ltac::PseudoRegister reg, tac::Size size = tac::Size::DEFAULT);

        void move(mtac::Argument argument, ltac::PseudoRegister reg);
        void move(mtac::Argument argument, ltac::PseudoFloatRegister reg);

        ltac::PseudoRegister get_free_pseudo_reg();
        ltac::PseudoFloatRegister get_free_pseudo_float_reg();
        
        ltac::PseudoRegister get_bound_pseudo_reg(unsigned short hard);
        ltac::PseudoFloatRegister get_bound_pseudo_float_reg(unsigned short hard);

        bool is_written(const std::shared_ptr<Variable> & variable);
        void set_written(const std::shared_ptr<Variable> & variable);
        bool is_escaped(const std::shared_ptr<Variable> & variable);

        void collect_parameters(eddic::Function& definition, const PlatformDescriptor* descriptor);

        int last_pseudo_reg();
        int last_float_pseudo_reg();

        void remove_from_pseudo_reg(const std::shared_ptr<Variable> & variable);
        void remove_from_pseudo_float_reg(const std::shared_ptr<Variable> & variable);
    
    private: 
        FloatPool& float_pool;

        //The pseudo registers
        as::PseudoRegisters<ltac::PseudoRegister> pseudo_registers;
        as::PseudoRegisters<ltac::PseudoFloatRegister> pseudo_float_registers;
};

} //end of ltac

} //end of eddic

#endif
