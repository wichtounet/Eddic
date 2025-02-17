//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the Boost Software License: Version 1.0.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <sstream>

#include "cpp_utils/assert.hpp"

#include "ast/VariableType.hpp"

using namespace eddic;

std::string ast::to_string(const ast::Type& type){
    if(auto* ptr = boost::smart_get<ast::SimpleType>(&type)){
        return ptr->type;
    } else if(auto* ptr = boost::smart_get<ast::ArrayType>(&type)){
        return to_string(ptr->type) + "[]";
    } else if(auto* ptr = boost::smart_get<ast::PointerType>(&type)){
        return to_string(ptr->type) + "*";
    } else if(auto* ptr = boost::smart_get<ast::TemplateType>(&type)){
        std::stringstream printed;

        printed << ptr->type << "<";

        for(auto& tmp_type : ptr->template_types){
            printed << to_string(tmp_type) << ",";
        }

        printed << ">";

        return printed.str();
    } else {
        cpp_unreachable("Unhandled type");
    }
}
