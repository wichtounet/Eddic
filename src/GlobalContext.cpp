//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <algorithm>
#include <functional>
#include <utility>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>

#include "GlobalContext.hpp"
#include "Variable.hpp"
#include "Utils.hpp"

#include "GetConstantValue.hpp"

using namespace eddic;

std::unordered_map<std::string, std::shared_ptr<Variable>> GlobalContext::getVariables(){
    return variables;
}
        
GlobalContext::GlobalContext() : Context(NULL) {
    Val zero = 0;
    
    variables["eddi_remaining"] = std::make_shared<Variable>("eddi_remaining", stringToType("int"), Position(GLOBAL, "eddi_remaining"), zero);
    variables["eddi_current"] = std::make_shared<Variable>("eddi_current", stringToType("int"), Position(GLOBAL, "eddi_current"), zero);
}

std::shared_ptr<Variable> GlobalContext::addVariable(const std::string& variable, Type type){
    //A global variable must have a value
    assert(type.isArray());
    
    Position position(GLOBAL, variable);
    
    return variables[variable] = std::make_shared<Variable>(variable, type, position);
}

std::shared_ptr<Variable> GlobalContext::addVariable(const std::string& variable, Type type, ast::Value& value){
    Position position(GLOBAL, variable);

    auto val = boost::apply_visitor(GetConstantValue(), value);

    return variables[variable] = std::make_shared<Variable>(variable, type, position, val);
}
