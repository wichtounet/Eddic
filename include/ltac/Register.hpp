//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_REGISTER_H
#define LTAC_REGISTER_H

#include <ostream>

namespace eddic {

namespace ltac {

/*!
 * \struct Register
 * Represents a symbolic hard register in the LTAC Representation. 
 */
struct Register {
    unsigned short reg;

    Register();
    explicit Register(unsigned short);
    
    explicit operator int();

    bool operator<(const Register& rhs) const;
    bool operator>(const Register& rhs) const;

    bool operator==(const Register& rhs) const;
    bool operator!=(const Register& rhs) const;
};

std::ostream& operator<<(std::ostream& out, const Register& reg);

/*!
 * Represent the stack pointer. 
 */
static const Register SP(1000);

/*!
 * Represent the base pointer. 
 */
static const Register BP(1001);

} //end of ltac

} //end of eddic

namespace std {
    template<>
    class hash<eddic::ltac::Register> {
    public:
        size_t operator()(const eddic::ltac::Register& val) const {
            return val.reg;
        }
    };
}

#endif
