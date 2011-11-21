//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_FOR_H
#define AST_FOR_H

#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

#include "ast/Deferred.hpp"
#include "ast/Condition.hpp"

namespace eddic {

namespace ast {

struct ASTFor {
    boost::optional<Instruction> start;
    boost::optional<Condition> condition;
    boost::optional<Instruction> repeat;
    std::vector<Instruction> instructions;
};

typedef Deferred<ASTFor> For;

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::For, 
    (boost::optional<eddic::ast::Instruction>, Content->start)
    (boost::optional<eddic::ast::Condition>, Content->condition)
    (boost::optional<eddic::ast::Instruction>, Content->repeat)
    (std::vector<eddic::ast::Instruction>, Content->instructions)
)

#endif
