//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_FUNCTION_CHECK_PASS_H
#define AST_FUNCTION_CHECK_PASS_H

#include "ast/ContextAwarePass.hpp"

namespace eddic {

namespace ast {

struct FunctionCheckPass : ContextAwarePass {
    void apply_function(ast::FunctionDeclaration& function) override;
    void apply_struct(ast::Struct& struct_, bool indicator) override;
    void apply_struct_function(ast::FunctionDeclaration& function) override;
    void apply_struct_constructor(ast::Constructor& constructor) override;
    void apply_struct_destructor(ast::Destructor& destructor) override;
    void apply_program(ast::SourceFile& program, bool indicator) override;

    std::shared_ptr<GlobalContext> context;
};

} //end of ast

} //end of eddic

#endif
