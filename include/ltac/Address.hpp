//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef LTAC_ADDRESS_H
#define LTAC_ADDRESS_H

#include "variant.hpp"

#include <boost/optional.hpp>

#include "ltac/Register.hpp"
#include "ltac/FloatRegister.hpp"
#include "ltac/PseudoRegister.hpp"
#include "ltac/PseudoFloatRegister.hpp"

namespace eddic::ltac {

using AddressRegister = boost::variant<ltac::Register, ltac::PseudoRegister, ltac::PseudoFloatRegister, ltac::FloatRegister>;

struct Address {
    boost::optional<ltac::AddressRegister> base_register;
    boost::optional<ltac::AddressRegister> scaled_register;
    boost::optional<unsigned int> scale;
    boost::optional<int> displacement;

    boost::optional<std::string> absolute;

    Address();
    explicit Address(std::string absolute);
    Address(std::string absolute, const ltac::AddressRegister& reg);
    Address(std::string absolute, int displacement);
    
    explicit Address(int displacement);
    Address(const ltac::AddressRegister& reg, int displacement);
    Address(const ltac::AddressRegister& reg, const ltac::AddressRegister& scaled);
    Address(const ltac::AddressRegister& reg, const ltac::AddressRegister& scaled, unsigned scale, int displacement);
};

bool operator==(ltac::Address& lhs, ltac::Address& rhs);
bool operator!=(ltac::Address& lhs, ltac::Address& rhs);

std::ostream& operator<<(std::ostream& out, const Address& address);

} // namespace eddic::ltac

#endif
