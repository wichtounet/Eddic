//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef BLOCK_CONTEXT_H
#define BLOCK_CONTEXT_H

#include "Context.hpp"

namespace eddic {

class FunctionContext;
struct GlobalContext;

/*!
 * \class BlockContext
 * \brief A symbol table for the block level. 
 */
struct BlockContext final : Context {
    BlockContext(Context * parent, FunctionContext & function_context, GlobalContext & global_context);

    std::shared_ptr<Variable> addVariable(const std::string & a, std::shared_ptr<const Type> type);
    std::shared_ptr<Variable> addVariable(const std::string & a, std::shared_ptr<const Type> type, ast::Value & value);

    std::shared_ptr<Variable> generate_variable(const std::string & prefix, std::shared_ptr<const Type> type) override;

    std::shared_ptr<Variable> new_temporary(std::shared_ptr<const Type> type);

    FunctionContext * function() override;

    std::shared_ptr<BlockContext> new_block_context();

private:
    FunctionContext & function_context_;

    std::vector<std::shared_ptr<BlockContext>> block_contexts; // TODO: We can probably avoid the shared_ptr
};

} // namespace eddic

#endif
