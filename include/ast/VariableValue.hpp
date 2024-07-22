//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_VARIABLE_VALUE_H
#define AST_VARIABLE_VALUE_H

#include <memory>
#include <string>

namespace eddic {

class Context;
class Variable;

namespace ast {

/*!
 * \class ASTVariableValue
 * \brief The AST node for a variable value.
 */
struct VariableValue : x3::file_position_tagged {
    Context * context = nullptr;
    std::shared_ptr<Variable> var;

    std::string variableName;

    std::shared_ptr<Variable> variable() const {
        return var;
    }
};

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::VariableValue,
    (std::string, variableName)
)

#endif
