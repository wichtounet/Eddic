//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef LTAC_PROLOGUE_H
#define LTAC_PROLOGUE_H

#include <memory>

#include "Options.hpp"

#include "mtac/forward.hpp"

namespace eddic {

namespace ltac {

void generate_prologue_epilogue(mtac::Program& program, std::shared_ptr<Configuration> configuration);

} //end of ltac

} //end of eddic

#endif
