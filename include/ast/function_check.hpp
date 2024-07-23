//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_FUNCTION_CHECK_PASS_H
#define AST_FUNCTION_CHECK_PASS_H

#include "ast/Pass.hpp"

namespace eddic {

struct GlobalContext;

namespace ast {

struct FunctionCheckPass : Pass {
    void apply_function(ast::TemplateFunctionDeclaration& function) override;
    void apply_struct(ast::struct_definition& struct_, bool indicator) override;
    void apply_struct_function(ast::TemplateFunctionDeclaration& function) override;
    void apply_struct_constructor(ast::Constructor& constructor) override;
    void apply_struct_destructor(ast::Destructor& destructor) override;
    void apply_program(ast::SourceFile& program, bool indicator) override;

    GlobalContext & context;
    FunctionCheckPass(GlobalContext & context) : context(context) {}
};

} //end of ast

} //end of eddic

#endif
