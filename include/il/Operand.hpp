//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef OPERAND_H
#define OPERAND_H

#include <string>

namespace eddic {

enum class OperandType : unsigned int {
    IMMEDIATE,
    REGISTER,
    STACK,
    GLOBAL
};

class Operand {
    private:
        OperandType m_type;

    public:
        Operand(OperandType type);

        virtual std::string getValue() = 0;
        bool isImmediate();
        bool isRegister();
        bool isStack();
        bool isGlobal();
};

} //end of eddic

#endif
