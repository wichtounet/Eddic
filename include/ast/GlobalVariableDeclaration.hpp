//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_GLOBAL_VARIABLE_DECLARATION_H
#define AST_GLOBAL_VARIABLE_DECLARATION_H

#include <memory>

#include <boost/fusion/include/adapt_struct.hpp>

#include "ast/Value.hpp"
#include "ast/VariableType.hpp"

namespace eddic {

class Context;

namespace ast {

/*!
 * \class ASTGlobalVariableDeclaration
 * \brief The AST node for a declaration of a global variable.
 */
struct GlobalVariableDeclaration : x3::file_position_tagged {
    Context * context = nullptr;

    Type variableType;
    std::string variableName;
    boost::optional<Value> value;

};

} // namespace ast

} // namespace eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::GlobalVariableDeclaration,
    (eddic::ast::Type, variableType)
    (std::string, variableName)
    (boost::optional<eddic::ast::Value>, value)
)

#endif
