//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_ELSE_H
#define AST_ELSE_H

#include <vector>
#include <memory>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

namespace eddic {

class Context;

namespace ast {

/*!
 * \class Else
 * \brief The AST node for an else construction.
 */
struct Else {
    Context * context = nullptr;
    std::vector<Instruction> instructions;
    x3::unused_type fake_;
};

} //end of ast

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::ast::Else,
    (std::vector<eddic::ast::Instruction>, instructions)
    (x3::unused_type, fake_)
)

#endif
