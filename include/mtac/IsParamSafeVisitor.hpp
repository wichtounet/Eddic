//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_IS_PARAM_SAFE_VISITOR_H
#define MTAC_IS_PARAM_SAFE_VISITOR_H

#include "variant.hpp"

#include "ast/values_def.hpp"

namespace eddic {

namespace mtac {

struct IsParamSafeVisitor : public boost::static_visitor<bool> {
    bool operator()(const ast::VariableValue& value) const;
    bool operator()(const ast::Integer& value) const;
    bool operator()(const ast::Float& value) const;
    bool operator()(const ast::True& value) const;
    bool operator()(const ast::False& value) const;
    bool operator()(const ast::Litteral& value) const;
    bool operator()(const ast::ArrayValue& value) const;
    bool operator()(const ast::Expression& value) const;
    bool operator()(const ast::Minus& value) const;
    bool operator()(const ast::Plus& value) const;
    bool operator()(const ast::FunctionCall& value) const;
    bool operator()(const ast::BuiltinOperator& value) const;
    bool operator()(const ast::SuffixOperation& value) const;
    bool operator()(const ast::PrefixOperation& value) const;
    bool operator()(const ast::Assignment& value) const;
};

} //end of mtac

} //end of eddic

#endif
