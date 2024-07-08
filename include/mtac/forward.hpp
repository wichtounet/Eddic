//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_FORWARD_DECLARATIONS_H
#define MTAC_FORWARD_DECLARATIONS_H

#include <memory>

#include "variant.hpp"

namespace eddic::mtac {

struct Program;
class Function;

class basic_block;
using basic_block_p  = std::shared_ptr<mtac::basic_block>;
using basic_block_cp = std::shared_ptr<const mtac::basic_block>;

struct Quadruple;

} // namespace eddic::mtac

#endif
