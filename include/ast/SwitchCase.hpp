//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_SWITCH_CASE_H
#define AST_SWITCH_CASE_H

#include <vector>
#include <memory>

#include <boost/fusion/include/adapt_struct.hpp>

#include "ast/Value.hpp"

namespace eddic {

class Context;

namespace ast {

/*!
 * \class SwitchCase
 * \brief The AST node for a switch case.
 */
struct SwitchCase : x3::file_position_tagged {
    Context * context = nullptr;

    Value value;
    std::vector<Instruction> instructions;
};

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::SwitchCase,
    (eddic::ast::Value, value)
    (std::vector<eddic::ast::Instruction>, instructions)
)

#endif
