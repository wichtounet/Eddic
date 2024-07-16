//=======================================================================
// Copyright Baptiste Wicht 2011-2024.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_TYPE_COLLECTION_PASS_H
#define AST_TYPE_COLLECTION_PASS_H

#include <unordered_map>
#include <unordered_set>

#include "ast/ContextAwarePass.hpp"
#include "Type.hpp"

namespace eddic::ast {

struct TypeCollectionPass : ContextAwarePass {
    void apply_program(ast::SourceFile& program, bool indicator) override;
    void apply_struct(ast::struct_definition & struct_, bool indicator) override;

private:
    std::unordered_map<std::string, std::shared_ptr<const eddic::Type>> fully_resolved;
    std::unordered_set<ast::struct_definition * > pending;
};

} // namespace eddic::ast

#endif
