//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/variant/apply_visitor.hpp>

#include "GetConstantValue.hpp"

#include "ast/Value.hpp"

using namespace eddic;

Val GetConstantValue::operator()(const ast::Litteral& litteral) const {
    return make_pair(litteral.value, litteral.value.size() - 2);
}

Val GetConstantValue::operator()(const ast::Integer& integer) const {
    return integer.value;
}
