//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef CONTEXT_ANNOTATOR_H
#define CONTEXT_ANNOTATOR_H

#include <memory>

#include "ast/Pass.hpp"

namespace eddic {

class Context;
class GlobalContext;

namespace ast {

class ContextAnnotationPass : public Pass {
    public:
        void apply_program(ast::SourceFile& program, bool indicator) override;
        void apply_function(ast::TemplateFunctionDeclaration& function) override;
        void apply_struct_function(ast::TemplateFunctionDeclaration& function) override;
        void apply_struct_constructor(ast::Constructor& constructor) override;
        void apply_struct_destructor(ast::Destructor& destructor) override;

        ContextAnnotationPass(GlobalContext & context) : globalContext(context) {}

    private:
        GlobalContext & globalContext;
        FunctionContext * functionContext = nullptr;
        Context * currentContext = nullptr;
};

} //end of ast

} //end of eddic

#endif
