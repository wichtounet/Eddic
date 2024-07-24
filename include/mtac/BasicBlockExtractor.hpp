//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_BASIC_BLOCK_EXTRACTOR_H
#define MTAC_BASIC_BLOCK_EXTRACTOR_H

#include <memory>

#include "mtac/forward.hpp"

namespace eddic::mtac {

void extract_basic_blocks(mtac::Program & program);
void clear_basic_blocks(mtac::Program & program);

} // namespace eddic::mtac

#endif
