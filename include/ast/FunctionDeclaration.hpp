//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_FUNCTION_DECLARATION_H
#define AST_FUNCTION_DECLARATION_H

#include <boost/variant/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace eddic {

struct FunctionDeclaration {
    std::string returnType;
    std::string functionName;
};

} //end of eddic

//Adapt the struct for the AST
BOOST_FUSION_ADAPT_STRUCT(
    eddic::FunctionDeclaration, 
    (std::string, returnType)
    (std::string, functionName)
)

#endif
