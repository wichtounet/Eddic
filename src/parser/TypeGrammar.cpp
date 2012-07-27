//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "parser/TypeGrammar.hpp"
#include "lexer/position.hpp"

using namespace eddic;

parser::TypeGrammar::TypeGrammar(const lexer::Lexer& lexer, const lexer::pos_iterator_type& position_begin) : 
        TypeGrammar::base_type(type, "Type Grammar"),
        position_begin(position_begin){
   
    const_ %=
            (lexer.const_ > boost::spirit::attr(true))
        |   boost::spirit::attr(false);

    array_type %=
            lexer.identifier
        >>  lexer.left_bracket
        >>  lexer.right_bracket;
    
    pointer_type %=
            lexer.identifier
        >>  lexer.multiplication;

    simple_type %=
            const_
        >>  lexer.identifier;

    type %=
            array_type
        |   pointer_type
        |   simple_type;
}
