//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_ELSE_IF_H
#define AST_ELSE_IF_H

#include <vector>
#include <memory>

#include <boost/fusion/include/adapt_struct.hpp>

#include "ast/Value.hpp"

namespace eddic {

class Context;

namespace ast {

/*!
 * \class ElseIf
 * \brief The AST node for an else if construction.
 */
struct ElseIf {
    Context * context = nullptr; 

    Value condition;
    std::vector<Instruction> instructions;
};

} //end of eddic

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::ElseIf, 
    (eddic::ast::Value, condition)
    (std::vector<eddic::ast::Instruction>, instructions)
)

#endif
