//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_FLOAT_REGISTER_H
#define LTAC_FLOAT_REGISTER_H

#include <ostream>

namespace eddic::ltac {

/*!
 * \struct FloatRegister
 * Represents a symbolic hard float register in the LTAC Representation. 
 */
struct FloatRegister {
    unsigned short reg;

    FloatRegister();
    FloatRegister(unsigned short);

    operator int();

    auto operator<=>(const FloatRegister & rhs) const = default;
};

std::ostream& operator<<(std::ostream& out, const FloatRegister& reg);

} // namespace eddic::ltac

namespace std {
    template<>
    class hash<eddic::ltac::FloatRegister> {
    public:
        size_t operator()(const eddic::ltac::FloatRegister& val) const {
            return val.reg;
        }
    };
} // namespace std

#endif
