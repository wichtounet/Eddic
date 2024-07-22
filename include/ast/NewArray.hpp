//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_NEW_ARRAY_H
#define AST_NEW_ARRAY_H

#include <memory>

#include "ast/VariableType.hpp"
#include "ast/Value.hpp"

namespace eddic {

class Context;

namespace ast {

/*!
 * \class ASTNewArray
 * \brief The AST node for a dynamic allocation of array.
 */
struct NewArray : x3::file_position_tagged {
    Context * context = nullptr;

    Type type;
    Value size;

};

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::NewArray,
    (eddic::ast::Type, type)
    (eddic::ast::Value, size)
)

#endif
