//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_DESTRUCTOR_H
#define AST_DESTRUCTOR_H

#include <string>
#include <memory>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

#include "ast/Instruction.hpp"
#include "ast/FunctionParameter.hpp"

namespace eddic {

class FunctionContext;

namespace ast {

/*!
 * \class ASTDestructor
 * \brief The AST node for a destructor declaration.
 */
struct Destructor : x3::file_position_tagged {
    FunctionContext * context = nullptr;

    std::string mangledName;
    std::shared_ptr<const eddic::Type> struct_type = nullptr;

    std::vector<FunctionParameter> parameters;

    std::vector<Instruction> instructions;
    x3::unused_type fake_;
};

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::Destructor,
    (std::vector<eddic::ast::Instruction>, instructions)
    (x3::unused_type, fake_)
)

#endif
