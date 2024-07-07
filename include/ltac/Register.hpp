//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_REGISTER_H
#define LTAC_REGISTER_H

#include <ostream>

namespace eddic::ltac {

/*!
 * \struct Register
 * Represents a symbolic hard register in the LTAC Representation. 
 */
struct Register {
    unsigned short reg = 0;

    Register() = default;
    explicit Register(unsigned short reg) : reg(reg) {}

    explicit operator int() const {
        return reg;
    }

    auto operator<=>(const Register & rhs) const = default;
};

/*!
 * Represent the stack pointer. 
 */
static const Register SP(1000);

/*!
 * Represent the base pointer. 
 */
static const Register BP(1001);

inline std::ostream & operator<<(std::ostream & out, const Register & reg) {
    if (reg == BP) {
        return out << "bp";
    }

    if (reg == SP) {
        return out << "sp";
    }

    return out << "r" << reg.reg;
}

} // namespace eddic::ltac

namespace std {
    template<>
    class hash<eddic::ltac::Register> {
    public:
        size_t operator()(const eddic::ltac::Register& val) const {
            return val.reg;
        }
    };
} // namespace std

#endif
