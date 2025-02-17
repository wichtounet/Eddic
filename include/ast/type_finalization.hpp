//=======================================================================
// Copyright Baptiste Wicht 2011-2024.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_TYPE_FINALIZATION_PASS_H
#define AST_TYPE_FINALIZATION_PASS_H

#include <unordered_map>
#include <unordered_set>

#include "ast/Pass.hpp"
#include "Type.hpp"

namespace eddic::ast {

struct TypeFinalizationPass : Pass {
    void apply_struct(ast::struct_definition & struct_, bool indicator) override;

    GlobalContext & context;
    TypeFinalizationPass(GlobalContext & context) : context(context) {}
};

} // namespace eddic::ast

#endif
