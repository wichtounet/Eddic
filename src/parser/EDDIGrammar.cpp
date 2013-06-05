//=======================================================================
// Copyright Baptiste Wicht 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "parser/EDDIGrammar.hpp"
#include "parser/Utils.hpp"

#include "lexer/adapttokens.hpp"
#include "lexer/position.hpp"

using namespace eddic;

parser::EddiGrammar::EddiGrammar(const lexer::Lexer& lexer) : 
        EddiGrammar::base_type(start, "EDDI Grammar"), 
        value_grammar(lexer), 
        type_grammar(lexer)
{
    auto const value = value_grammar(qi::_a);
    auto const& type = type_grammar;
    auto local_begin = qi::lazy(boost::phoenix::construct<qi::position>(qi::_a));

    start %= qi::eps [ qi::_a = qi::_r1 ] >> program;

    delete_ %=
            local_begin
        >>  lexer.delete_
        >>  value;
    
    default_case %=
            lexer.default_
        >>  lexer.double_dot
        >>  *(instruction);

    switch_case %=
            local_begin
        >>  lexer.case_
        >>  value
        >>  lexer.double_dot
        >>  (*instruction);

    switch_ %=
            local_begin
        >>  lexer.switch_
        >>  lexer.left_parenth
        >>  value
        >>  lexer.right_parenth
        >>  lexer.left_brace
        >>  *(switch_case)
        >>  -(default_case)
        >>  lexer.right_brace
            ;

    else_if_ %= 
            lexer.else_ 
        >>  lexer.if_ 
        >>  lexer.left_parenth 
        >>  value 
        >>  lexer.right_parenth 
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    else_ %= 
            lexer.else_ 
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    if_ %= 
            lexer.if_ 
        >>  lexer.left_parenth 
        >>  value 
        >>  lexer.right_parenth 
        >>  lexer.left_brace 
        >>  *(instruction) 
        >>  lexer.right_brace
        >>  *(else_if_)
        >>  -(else_);

    for_ %= 
            lexer.for_ 
        >   lexer.left_parenth 
        >   -declaration 
        >   lexer.stop 
        >   -value 
        >   lexer.stop 
        >   -repeatable_instruction 
        >   lexer.right_parenth 
        >   lexer.left_brace
        >   (*instruction)
        >   lexer.right_brace;
    
    foreach_ = 
            local_begin
        >>  lexer.foreach_ 
        >>  lexer.left_parenth 
        >>  type 
        >>  lexer.identifier 
        >>  lexer.from_ 
        >>  lexer.integer 
        >>  lexer.to_ 
        >>  lexer.integer 
        >>  lexer.right_parenth 
        >>  lexer.left_brace 
        >>  *(instruction)
        >>  lexer.right_brace;

    foreachin_ = 
            local_begin
        >>  lexer.foreach_ 
        >>  lexer.left_parenth 
        >>  type 
        >>  lexer.identifier 
        >>  lexer.in_ 
        >>  lexer.identifier 
        >>  lexer.right_parenth 
        >>  lexer.left_brace 
        >>  *(instruction)
        >>  lexer.right_brace;
    
    while_ %=
            lexer.while_ 
        >   lexer.left_parenth 
        >   value 
        >   lexer.right_parenth 
        >   lexer.left_brace 
        >   *(instruction)
        >   lexer.right_brace;

    do_while_ %=
            lexer.do_ 
        >   lexer.left_brace 
        >   *(instruction)
        >   lexer.right_brace
        >   lexer.while_  
        >   lexer.left_parenth 
        >   value 
        >   lexer.right_parenth
        >   lexer.stop;

    struct_declaration %= 
            local_begin
        >>  type 
        >>  lexer.identifier 
        >>  lexer.left_parenth
        >>  -(value >> *( lexer.comma > value))
        >>  lexer.right_parenth;

    declaration %= 
            local_begin
        >>  type 
        >>  lexer.identifier 
        >>  -(lexer.assign >> value);
    
    arrayDeclaration %= 
            local_begin
        >>  type
        >>  lexer.identifier 
        >>  lexer.left_bracket
        >>  value
        >>  lexer.right_bracket;

    return_ %=
            local_begin
        >>  lexer.return_
        >>  value
        >>  lexer.stop;
    
    globalDeclaration %= 
            local_begin
        >>  type 
        >>  lexer.identifier 
        >>  -(lexer.assign >> value)
        >>  lexer.stop;

    globalArrayDeclaration %= 
            local_begin
        >>  type 
        >>  lexer.identifier 
        >>  lexer.left_bracket
        >>  value
        >>  lexer.right_bracket
        >>  lexer.stop;

    swap %= 
            local_begin
        >>  lexer.identifier 
        >>  lexer.swap 
        >>  lexer.identifier;
    
    instruction %= 
            switch_
        |   (value_grammar.assignment(qi::_a) > lexer.stop)
        |   (value_grammar.postfix_expression(qi::_a) > lexer.stop)
        |   (value_grammar.function_call(qi::_a) > lexer.stop)
        |   (struct_declaration >> lexer.stop)
        |   (declaration >> lexer.stop)
        |   (value_grammar.prefix_operation(qi::_a) > lexer.stop)
        |   (arrayDeclaration >> lexer.stop)
        |   if_
        |   for_
        |   while_
        |   do_while_
        |   foreach_
        |   foreachin_
        |   return_
        |   (swap > lexer.stop)
        |   (delete_ > lexer.stop)
        ;

    repeatable_instruction = 
            value_grammar.assignment(qi::_a) 
        |   swap 
        |   value_grammar.postfix_expression(qi::_a)
        |   value_grammar.prefix_operation(qi::_a)
        |   value_grammar.function_call(qi::_a);

    arg %= 
            type 
        >>  lexer.identifier;

    template_function %= 
            local_begin
        >>  lexer.template_
        >>  qi::omit[lexer.less]
        >>  +(
                    lexer.type
               >>   lexer.identifier
               >>  *(
                            lexer.comma
                       >>   lexer.type
                       >>   lexer.identifier
                   )
            )
        >>  qi::omit[lexer.greater]
        >>  type 
        >>  lexer.identifier
        >>  lexer.left_parenth
        >>  -( arg >> *( lexer.comma > arg))
        >>  lexer.right_parenth
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    function %= 
            local_begin
        >>  type 
        >>  lexer.identifier
        >>  lexer.left_parenth
        >>  -( arg >> *( lexer.comma > arg))
        >>  lexer.right_parenth
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    member_declaration %=
            local_begin
        >>  type
        >>  lexer.identifier
        >>  lexer.stop;

    constructor %= 
            local_begin
        >>  qi::omit[lexer.this_]
        >>  lexer.left_parenth
        >>  -( arg >> *( lexer.comma > arg))
        >>  lexer.right_parenth
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    destructor %= 
            local_begin
        >>  lexer.tilde     
        >>  qi::omit[lexer.this_]
        >>  lexer.left_parenth
        >>  lexer.right_parenth
        >>  lexer.left_brace
        >>  *(instruction)
        >>  lexer.right_brace;

    template_struct %=
            local_begin
        >>  lexer.template_
        >>  qi::omit[lexer.less]
        >>  +(
                    lexer.type
               >>   lexer.identifier
               >>  *(
                            lexer.comma
                       >>   lexer.type
                       >>   lexer.identifier
                   )
            )
        >>  qi::omit[lexer.greater]
        >>  lexer.struct_
        >>  lexer.identifier
        >>  -(
                    lexer.extends
                >>  type 
             )
        >>  lexer.left_brace
        >>  *(

                    member_declaration
                |   (arrayDeclaration >> lexer.stop)
                |   constructor
                |   destructor
                |   function
                |   template_function
             )
        >>  lexer.right_brace;

    struct_ %=
            local_begin
        >>  lexer.struct_
        >>  lexer.identifier
        >>  -(
                    lexer.extends
                >>  type 
             )
        >>  lexer.left_brace
        >>  *(

                    member_declaration
                |   (arrayDeclaration >> lexer.stop)
                |   constructor
                |   destructor
                |   function
                |   template_function
             )
        >>  lexer.right_brace;

    standardImport %= 
            local_begin
        >>  lexer.include
        >>  qi::omit[lexer.less]
        >>  lexer.identifier
        >>  qi::omit[lexer.greater];

    import %=
            local_begin
        >>  lexer.include
        >>  lexer.string_literal;

    program %=
            local_begin
        >>  *(
                    function 
                |   template_function 
                |   globalDeclaration 
                |   globalArrayDeclaration 
                |   standardImport 
                |   import 
                |   struct_
                |   template_struct
            );

    /* Debugging rules */
    //DEBUG_RULE(program);
    //DEBUG_RULE(function);
    //DEBUG_RULE(template_function);
    //DEBUG_RULE(instruction);
    //DEBUG_RULE(struct_);
    //DEBUG_RULE(globalDeclaration);
    //DEBUG_RULE(constructor);
    //DEBUG_RULE(destructor);
}
