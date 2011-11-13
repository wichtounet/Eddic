//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "mangling.hpp"

using namespace eddic;

std::string eddic::mangle(Type type){
    if(type.base() == BaseType::INT){
        return type.isArray() ? "AI" : "I";
    } else if(type.base() == BaseType::INT){
        return type.isArray() ? "AS" : "S";
    }

    assert(false); //Type not handled
}

std::string eddic::mangle(const std::string& functionName, const std::vector<ast::Value>& values){
    if(functionName == "main"){
        return functionName;
    }

    std::ostringstream ss;

    ss << "_F";
    ss << functionName.length();
    ss << functionName;

    GetTypeVisitor visitor;

    for(auto& value : values){
        Type type = boost::apply_visitor(visitor, value);
        ss << mangle(type);
    }

    return ss.str();
}
