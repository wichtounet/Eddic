//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "BlockContext.hpp"
#include "Variable.hpp"
#include "FunctionContext.hpp"
#include "Position.hpp"
#include "VisitorUtils.hpp"
#include "Type.hpp"

#include "ast/GetConstantValue.hpp"

using namespace eddic;

BlockContext::BlockContext(Context * parent, FunctionContext & functionContext, GlobalContext & global_context) : 
    Context(parent, global_context), function_context_(functionContext){} 

std::shared_ptr<Variable> BlockContext::addVariable(const std::string& variable, std::shared_ptr<const Type> type){
    return variables[variable] = function_context_.newVariable(variable, type);
}

std::shared_ptr<Variable> BlockContext::generate_variable(const std::string& prefix, std::shared_ptr<const Type> type){
    auto variable = function_context_.generate_variable(prefix, type);

    return variables[variable->name()] = variable;
}

std::shared_ptr<Variable> BlockContext::addVariable(const std::string& variable, std::shared_ptr<const Type> type, ast::Value& value){
    assert(type->is_const());

    Position position(PositionType::CONST);

    auto val = visit(ast::GetConstantValue(), value);

    return variables[variable] = std::make_shared<Variable>(variable, type, position, val);
}

std::shared_ptr<Variable> BlockContext::new_temporary(std::shared_ptr<const Type> type){
    return function_context_.new_temporary(type);
}

FunctionContext * BlockContext::function(){
    return &function_context_;
}

std::shared_ptr<BlockContext> BlockContext::new_block_context() {
    return block_contexts.emplace_back(std::make_shared<BlockContext>(this, function_context_, global()));
}
