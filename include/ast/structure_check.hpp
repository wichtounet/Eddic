//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_STRUCTURE_CHECK_PASS_H
#define AST_STRUCTURE_CHECK_PASS_H

#include "ast/Pass.hpp"

namespace eddic {

namespace ast {

struct StructureCheckPass : Pass {
    void apply_struct(ast::struct_definition& struct_, bool indicator) override;

    GlobalContext & context;
    StructureCheckPass(GlobalContext & context) : context(context) {}
};

} //end of ast

} //end of eddic

#endif
