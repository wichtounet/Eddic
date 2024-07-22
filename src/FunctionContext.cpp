//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <utility>

#include "FunctionContext.hpp"
#include "GlobalContext.hpp"
#include "BlockContext.hpp"
#include "Options.hpp"
#include "Type.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "VisitorUtils.hpp"
#include "ast/GetConstantValue.hpp"
#include "cpp_utils/assert.hpp"
#include "logging.hpp"

using namespace eddic;

FunctionContext::FunctionContext(Context * parent, GlobalContext & global_context, Platform platform, const std::shared_ptr<Configuration> & configuration) :
        Context(parent, global_context), platform(platform) {
    //TODO Should not be done here
    if(configuration->option_defined("fomit-frame-pointer")){
        currentParameter = INT->size();
    } else {
        currentParameter = 2 * INT->size();
    }
}

int FunctionContext::size() const {
    const int size = -currentPosition;

    if(size == static_cast<int>(INT->size())){
        return 0;
    }

    return size;
}

int FunctionContext::stack_position() const { return currentPosition; }

void FunctionContext::set_stack_position(int current){
    currentPosition = current;
}

std::shared_ptr<Variable> FunctionContext::newParameter(const std::string& variable,
                                                        const std::shared_ptr<const Type>& type) {
    const Position position(PositionType::PARAMETER, currentParameter);

    LOG<Info>("Variables") << "New parameter " << variable << " at position " << currentParameter << log::endl;

    currentParameter += type->size();

    return std::make_shared<Variable>(variable, type, position);
}

std::shared_ptr<Variable> FunctionContext::newVariable(const std::string& variable,
                                                       const std::shared_ptr<const Type>& type) {
    auto var = std::make_shared<Variable>(variable, type, Position(PositionType::VARIABLE));

    storage.push_back(var);

    return var;
}

Storage FunctionContext::stored_variables(){
    return storage;
}

std::shared_ptr<Variable> FunctionContext::addVariable(const std::string& variable, std::shared_ptr<const Type> type){
    return variables[variable] = newVariable(variable, type);
}

std::shared_ptr<Variable> FunctionContext::newVariable(const std::shared_ptr<Variable>& source) {
    const std::string name = "g_" + source->name() + "_" + toString(temporary++);

    if(source->position().is_temporary()){
        const Position position(PositionType::TEMPORARY);

        auto var = std::make_shared<Variable>(name, source->type(), position);
        storage.push_back(var);
        return variables[name] = var;
    }
    return addVariable(name, source->type());
}

std::shared_ptr<Variable> FunctionContext::addVariable(const std::string& variable, std::shared_ptr<const Type> type, ast::Value& value){
    assert(type->is_const());

    const Position position(PositionType::CONST);

    auto val = visit(ast::GetConstantValue(), value);

    auto var = std::make_shared<Variable>(variable, type, position, val);
    return variables[variable] = var;
}

std::shared_ptr<Variable> FunctionContext::generate_variable(const std::string& prefix, std::shared_ptr<const Type> type){
    const std::string name = prefix + "_" + toString(generated++);
    return addVariable(name, type);
}

std::shared_ptr<Variable> FunctionContext::addParameter(const std::string& parameter,
                                                        const std::shared_ptr<const Type>& type) {
    return variables[parameter] = newParameter(parameter, type);
}

std::shared_ptr<Variable> FunctionContext::new_temporary(std::shared_ptr<const Type> type){
    cpp_assert((type->is_standard_type() && type != STRING) || type->is_pointer() || type->is_dynamic_array(), "Invalid temporary");

    const Position position(PositionType::TEMPORARY);

    const std::string name = "t_" + toString(temporary++);
    auto var = std::make_shared<Variable>(name, type, position);
    storage.push_back(var);
    return variables[name] = var;
}

std::shared_ptr<Variable> FunctionContext::new_reference(const std::shared_ptr<const Type>& type,
                                                         const std::shared_ptr<Variable>& var, const Offset& offset) {
    const std::string name = "t_" + toString(temporary++);
    auto variable = std::make_shared<Variable>(name, type, var, offset);
    storage.push_back(variable);
    return variables[name] = variable;
}

void FunctionContext::allocate_in_param_register(const std::shared_ptr<Variable>& variable, unsigned int register_) {
    assert(variable->position().isParameter());

    const Position position(PositionType::PARAM_REGISTER, register_);
    variable->setPosition(position);
}

void FunctionContext::removeVariable(std::shared_ptr<Variable> variable){
    auto iter_var = std::find(storage.begin(), storage.end(), variable);

    if(variable->position().isParameter()){
        variables.erase(variable->name());

        for(auto& v : variables){
            if(v.second->position().isParameter()){
                if(v.second->position().offset() > variable->position().offset()){
                    const Position position(PositionType::PARAMETER,
                                            v.second->position().offset() - variable->type()->size());
                    v.second->setPosition(position);
                }
            }
        }

        currentParameter -= variable->type()->size();

        LOG<Info>("Variables") << "Remove parameter " << variable->name() << log::endl;
    } else {
        variables.erase(variable->name());
        storage.erase(iter_var);

        LOG<Info>("Variables") << "Remove variable " << variable->name() << log::endl;
    }
}

FunctionContext * FunctionContext::function(){
    return this;
}

std::shared_ptr<BlockContext> FunctionContext::new_block_context() {
    return block_contexts.emplace_back(std::make_shared<BlockContext>(this, *this, global()));
}
