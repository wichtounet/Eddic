//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "parser/ValueGrammar.hpp"
#include "parser/Utils.hpp"

#include "lexer/adapttokens.hpp"
#include "lexer/position.hpp"
#include <boost/spirit/include/phoenix.hpp>

using namespace eddic;

parser::ValueGrammar::ValueGrammar(const lexer::Lexer& lexer) : 
        ValueGrammar::base_type(start, "_value Grammar"),
        type(lexer)
{
    auto local_begin     = qi::lazy ( boost::phoenix::construct<qi::position>(qi::_a) );
    auto inherited_begin = qi::lazy ( boost::phoenix::construct<qi::position>(qi::_r1) );

    start %= qi::eps [ qi::_a = qi::_r1 ] >> value;

    /* Match operators into symbols */
    
    unary_op.add
        ("+", ast::Operator::ADD)
        ("-", ast::Operator::SUB)
        ("!", ast::Operator::NOT)
        ("*", ast::Operator::STAR)
        ("&", ast::Operator::ADDRESS)
        ;

    additive_op.add
        ("+", ast::Operator::ADD)
        ("-", ast::Operator::SUB)
        ;

    multiplicative_op.add
        ("/", ast::Operator::DIV)
        ("*", ast::Operator::MUL)
        ("%", ast::Operator::MOD)
        ;
    
    relational_op.add
        (">=", ast::Operator::GREATER_EQUALS) 
        (">", ast::Operator::GREATER) 
        ("<=", ast::Operator::LESS_EQUALS) 
        ("<", ast::Operator::LESS) 
        ("!=", ast::Operator::NOT_EQUALS) 
        ("==", ast::Operator::EQUALS) 
        ;
    
    logical_and_op.add
        ("&&", ast::Operator::AND) 
        ;
    
    logical_or_op.add
        ("||", ast::Operator::OR) 
        ;
    
    postfix_op.add
        ("++", ast::Operator::INC)
        ("--", ast::Operator::DEC)
        ;
    
    prefix_op.add
        ("++", ast::Operator::INC)
        ("--", ast::Operator::DEC)
        ;
    
    builtin_op.add
        ("size", ast::BuiltinType::SIZE)
        ("length", ast::BuiltinType::LENGTH)
        ;
    
    assign_op.add
        ("=",  ast::Operator::ASSIGN)
        ("+=", ast::Operator::ADD)
        ("-=", ast::Operator::SUB)
        ("/=", ast::Operator::DIV)
        ("*=", ast::Operator::MUL)
        ("%=", ast::Operator::MOD)
        ;

    /* Terminal values */
    
    new_array %=
            local_begin
        >>  lexer.new_
        >>  type.type
        >>  lexer.left_bracket
        >>  value
        >>  lexer.right_bracket;

    new_ %=
            local_begin
        >>  lexer.new_
        >>  type.type
        >>  lexer.left_parenth
        >>  -( value >> *( lexer.comma > value))
        >>  lexer.right_parenth;
   
    variable_value %= 
            local_begin
        >>  
            (
                    lexer.this_
                |   lexer.identifier
            );
    
    null %= 
            qi::eps
        >>  lexer.null;
    
    true_ %= 
            qi::eps
        >>  lexer.true_;
    
    false_ %= 
            qi::eps
        >>  lexer.false_;

    integer %= 
            qi::eps 
        >>  lexer.integer;

    integer_suffix %=
            lexer.integer
        >>  lexer.identifier;

    float_ %= 
            qi::eps 
        >>  lexer.float_;
    
    string_literal %= 
            qi::eps 
        >> lexer.string_literal;
    
    char_literal %= 
            qi::eps 
        >> lexer.char_literal;

    /* Define values */ 
    
    primary_value = 
            integer_suffix
        |   integer
        |   float_
        |   string_literal
        |   char_literal
        |   builtin_operator
        |   function_call(qi::_a)
        |   new_array
        |   new_
        |   variable_value
        |   null
        |   true_
        |   false_
        |   (lexer.left_parenth >> value >> lexer.right_parenth);
    
    call_value %=
            lexer.identifier
        >>  -(
                    qi::omit[lexer.less]
                >>  type >> *(lexer.comma > type)
                >>  qi::omit[lexer.greater]
            )
        >>  lexer.left_parenth
        >>  -( value >> *( lexer.comma > value))
        >>  lexer.right_parenth;

    postfix_expression %=
            inherited_begin
        >>  primary_value
        >>  +(
                         lexer.left_bracket 
                     >>  boost::spirit::attr(ast::Operator::BRACKET) 
                     >>  value 
                     >>  lexer.right_bracket
                |
                         lexer.dot
                     >>  boost::spirit::attr(ast::Operator::CALL) 
                     >>  call_value 
                |
                         lexer.dot 
                     >>  boost::spirit::attr(ast::Operator::DOT) 
                     >>  qi::as<std::string>()[((lexer.identifier))]
                |
                    qi::adapttokens[postfix_op]       
            );

    prefix_operation %=
            inherited_begin
        >>  qi::adapttokens[prefix_op]  
        >>  unary_expression;
    
    unary_operation %=
            local_begin
        >>  qi::adapttokens[unary_op]   
        >>  cast_expression;
    
    unary_expression %=
            postfix_expression(qi::_a)
        |   prefix_operation(qi::_a)
        |   unary_operation
        |   primary_value;

    cast_value %=
            local_begin
        >>  lexer.left_parenth
        >>  type.type
        >>  lexer.right_parenth
        >>  cast_expression;

    cast_expression %=
            cast_value
        |   unary_expression;
    
    multiplicative_value %=
            local_begin
        >>  cast_expression
        >>  *(qi::adapttokens[multiplicative_op] >> cast_expression);
    
    additive_value %=
            local_begin
        >>  multiplicative_value
        >>  *(qi::adapttokens[additive_op] >> multiplicative_value);
   
    relational_value %=
            local_begin
        >>  additive_value
        >>  *(qi::adapttokens[relational_op] >> additive_value);  
    
    logicalAnd_value %=
            local_begin
        >>  relational_value
        >>  *(qi::adapttokens[logical_and_op] >> relational_value);  
    
    logicalOr_value %=
            local_begin
        >>  logicalAnd_value
        >>  *(qi::adapttokens[logical_or_op] >> logicalAnd_value);  

    ternary %=
            local_begin
        >>  logicalOr_value 
        >>  lexer.question_mark
        >>  conditional_expression 
        >>  lexer.double_dot
        >>  conditional_expression;

    conditional_expression =
            ternary
         |  logicalOr_value;

    assignment_expression %=
            assignment(qi::_a)
        |   conditional_expression;
    
    assignment %= 
            inherited_begin
        >>  unary_expression
        >>  qi::adapttokens[assign_op]
        >>  assignment_expression;

    value = assignment_expression.alias();
   
    function_call %=
            inherited_begin
        >>  lexer.identifier
        >>  -(
                    qi::omit[lexer.less]
                >>  type >> *(lexer.comma > type)
                >>  qi::omit[lexer.greater]
            )
        >>  lexer.left_parenth
        >>  -( value >> *( lexer.comma > value))
        >>  lexer.right_parenth;
   
    builtin_operator %=
            local_begin
        >>  qi::adapttokens[builtin_op]
        >>  lexer.left_parenth
        >>  value >> *( lexer.comma > value)
        >>  lexer.right_parenth;

    //Configure debugging and rule naming
    
    //DEBUG_RULE(value);
    //DEBUG_RULE(primary_value);
    //DEBUG_RULE(cast_value);
    //DEBUG_RULE(conditional_expression);
    //DEBUG_RULE(additive_value);
    //DEBUG_RULE(multiplicative_value);
    //DEBUG_RULE(relational_value);
    //DEBUG_RULE(logicalAnd_value);
    //DEBUG_RULE(logicalOr_value);
    //DEBUG_RULE(integer);
    //DEBUG_RULE(integer_suffix);
    //DEBUG_RULE(float_);
    //DEBUG_RULE(function_call);
    //DEBUG_RULE(true_);
    //DEBUG_RULE(false_);
    //DEBUG_RULE(null);
    //DEBUG_RULE(new_);
    //DEBUG_RULE(new_array);
    //DEBUG_RULE(unary_operation);
    //DEBUG_RULE(assignment);
    //DEBUG_RULE(prefix_operation);
    //DEBUG_RULE(builtin_operator);
    //DEBUG_RULE(variable_value);
    //DEBUG_RULE(ternary);
    //DEBUG_RULE(string_literal);
    //DEBUG_RULE(char_literal);
    //DEBUG_RULE(call_value);
    
    //DEBUG_RULE(assignment_expression);
    //DEBUG_RULE(unary_expression);
    //DEBUG_RULE(cast_expression);
    //DEBUG_RULE(postfix_expression);
}
