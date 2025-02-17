//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the Boost Software License: Version 1.0.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "ast/Boolean.hpp"

using namespace eddic;

std::ostream& ast::operator<< (std::ostream& stream, const ast::Boolean& b){
    return stream << b.value;
}
