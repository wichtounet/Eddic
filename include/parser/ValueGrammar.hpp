//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef VALUE_GRAMMAR_H
#define VALUE_GRAMMAR_H

#include "boost_cfg.hpp"
#include <boost/spirit/include/qi.hpp>
#include "lexer/SpiritLexer.hpp"

#include "ast/Value.hpp"
#include "parser/TypeGrammar.hpp"

namespace qi = boost::spirit::qi;

namespace eddic {

namespace parser {

/*!
 * \class ValueGrammar
 * \brief Grammar representing values in EDDI language.
 */
struct ValueGrammar : 
    qi::grammar<lexer::Iterator, 
    ast::Value(lexer::pos_iterator_type const&),
    qi::locals<lexer::pos_iterator_type> > 
{
    ValueGrammar(const lexer::Lexer& lexer);

    template <typename A, typename... Inherited> using Rule = qi::rule<lexer::Iterator, A(Inherited...), qi::locals<lexer::pos_iterator_type> >;
    
    Rule<ast::Assignment,      lexer::pos_iterator_type const&> assignment;
    Rule<ast::Expression,      lexer::pos_iterator_type const&> postfix_expression;
    Rule<ast::FunctionCall,    lexer::pos_iterator_type const&> function_call;
    Rule<ast::PrefixOperation, lexer::pos_iterator_type const&> prefix_operation;

  private:
    Rule<ast::Value, lexer::pos_iterator_type const&> start;

    Rule<ast::Ternary>            ternary;
    Rule<ast::Value>              value;
    Rule<ast::Value>              primary_value;
    Rule<ast::Cast>               cast_value;
    Rule<ast::Value>              conditional_expression;
    Rule<ast::CallOperationValue> call_value;

    Rule<ast::Expression>         postfix_expression_helper;

    Rule<ast::Expression>         additive_value;
    Rule<ast::Expression>         multiplicative_value;
    Rule<ast::Expression>         relational_value;
    Rule<ast::Expression>         logicalAnd_value;
    Rule<ast::Expression>         logicalOr_value;

    Rule<ast::Integer>            integer;
    Rule<ast::IntegerSuffix>      integer_suffix;
    Rule<ast::Float>              float_;
    Rule<ast::Literal>            string_literal;
    Rule<ast::CharLiteral>        char_literal;
    Rule<ast::VariableValue>      variable_value;
    Rule<ast::BuiltinOperator>    builtin_operator;
    Rule<ast::True>               true_;
    Rule<ast::False>              false_;
    Rule<ast::Null>               null;
    Rule<ast::New>                new_;
    Rule<ast::NewArray>           new_array;
    Rule<ast::PrefixOperation>    unary_operation;

    Rule<ast::Value>              assignment_expression;
    Rule<ast::Value>              unary_expression;
    Rule<ast::Value>              cast_expression;
    
    /* Operators */

    qi::symbols<char, ast::Operator> multiplicative_op;
    qi::symbols<char, ast::Operator> additive_op;
    qi::symbols<char, ast::Operator> relational_op;
    qi::symbols<char, ast::Operator> logical_and_op;
    qi::symbols<char, ast::Operator> logical_or_op;
    qi::symbols<char, ast::Operator> postfix_op;
    qi::symbols<char, ast::Operator> prefix_op;
    qi::symbols<char, ast::Operator> assign_op;
    qi::symbols<char, ast::Operator> unary_op;

    qi::symbols<char, ast::BuiltinType> builtin_op;
    
    TypeGrammar type;
};

} //end of parser

} //end of eddic

#endif
