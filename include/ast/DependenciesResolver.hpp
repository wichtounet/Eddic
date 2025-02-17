//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef DEPENDENCIES_RESOLVER_H
#define DEPENDENCIES_RESOLVER_H

#include "ast/source_def.hpp"

namespace eddic {

namespace parser_x3 {
    struct SpiritParser;
}

namespace ast {

void resolveDependencies(ast::SourceFile& program, parser_x3::SpiritParser& parser);

} //end of ast

} //end of eddic

#endif
