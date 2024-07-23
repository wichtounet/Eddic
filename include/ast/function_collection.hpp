//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_FUNCTION_COLLECTION_PASS_H
#define AST_FUNCTION_COLLECTION_PASS_H

#include "ast/Pass.hpp"

namespace eddic {

namespace ast {

struct FunctionCollectionPass : Pass {
    void apply_function(ast::TemplateFunctionDeclaration& function) override;
    void apply_struct_function(ast::TemplateFunctionDeclaration& function) override;
    void apply_struct_constructor(ast::Constructor& constructor) override;
    void apply_struct_destructor(ast::Destructor& destructor) override;

    GlobalContext & context;
    FunctionCollectionPass(GlobalContext & context) : context(context) {}
};

} //end of ast

} //end of eddic

#endif
