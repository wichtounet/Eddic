//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <algorithm>
#include <functional>
#include <utility>

#include "cpp_utils/assert.hpp"

#include "Context.hpp"
#include "GlobalContext.hpp"
#include "Utils.hpp"
#include "Type.hpp"
#include "Variable.hpp"

using namespace eddic;

Context::Context(Context * parent, GlobalContext & global_context) : parent_(parent), global_context_(global_context) {}

Context * Context::parent() const  {
    return parent_;
}

bool Context::exists(const std::string& variable) const {
    bool found = variables.find(variable) != variables.end();

    auto parent = parent_;

    while(!found){
        if(parent){
            found = parent->variables.find(variable) != parent->variables.end();
            parent = parent->parent_;
        } else {
            return false;
        }
    }

    return true;
}

std::shared_ptr<Variable> Context::new_temporary(std::shared_ptr<const Type>){
    cpp_unreachable("Not implemented");
}

std::shared_ptr<Variable> Context::operator[](const std::string& variable) const {
    return getVariable(variable);
}

std::shared_ptr<Variable> Context::getVariable(const std::string& variable) const {
    cpp_assert(exists(variable), "The variable must exists");

    auto iter = variables.find(variable);
    auto end = variables.end();

    auto * parent = parent_;

    while(iter == end){
        iter = parent->variables.find(variable);
        end = parent->variables.end();
        parent = parent->parent_;
    }
    
    return iter->second;
}

void Context::removeVariable(std::shared_ptr<Variable> variable){
    auto iter = variables.find(variable->name());
    auto end = variables.end();

    auto * parent = parent_;
    
    while(iter == end){
        iter = parent->variables.find(variable->name());
        end = parent->variables.end();
        parent = parent->parent_;
    }

    variables.erase(iter);
}
 
Context::Variables::const_iterator Context::begin() const {
    return variables.cbegin();
}

Context::Variables::const_iterator Context::end() const {
    return variables.cend();
}

GlobalContext & Context::global() const {
    return global_context_;
}

FunctionContext * Context::function(){
    return nullptr;
}
