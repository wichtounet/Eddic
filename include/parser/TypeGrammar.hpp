//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef TYPE_GRAMMAR_H
#define TYPE_GRAMMAR_H

#include <boost/spirit/include/qi.hpp>
#include "lexer/SpiritLexer.hpp"
#include "ast/Program.hpp"

namespace qi = boost::spirit::qi;

namespace eddic {

typedef lexer_type::iterator_type Iterator;
typedef SimpleLexer<lexer_type> Lexer;

struct TypeGrammar : qi::grammar<Iterator, ast::Type()> {
    TypeGrammar(const Lexer& lexer);

    qi::rule<Iterator, ast::Type()> type;
    qi::rule<Iterator, ast::ArrayType()> arrayType;
    qi::rule<Iterator, ast::SimpleType()> simpleType;
};

} //end of eddic

#endif
