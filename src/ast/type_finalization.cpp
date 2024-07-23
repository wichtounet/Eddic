//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>

#include "SemanticalException.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "mangling.hpp"
#include "VisitorUtils.hpp"

#include "ast/type_finalization.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

void ast::TypeFinalizationPass::apply_struct(ast::struct_definition & struct_, bool) {
    if (!struct_.struct_type) {
        context.error_handler.semantical_exception("The structure " + struct_.name + " cannot be fully resolved", struct_);
    }

    auto signature = context.get_struct_safe(struct_.mangled_name);

    // Finalize all member pointers

    for (auto & block : struct_.blocks) {
        if (auto * member = boost::get<ast::MemberDeclaration>(&block)) {
            auto & struct_member = (*signature)[member->name];

            if (struct_member.type->is_incomplete()) {
                struct_member.type = visit(ast::TypeTransformer(context), member->type);
            }
        }
    }
}
