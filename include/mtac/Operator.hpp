//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_OPERATOR_H
#define MTAC_OPERATOR_H

#include "ast/Operator.hpp"

namespace eddic {

namespace mtac {

/*!
 * \brief Type of operator in MTAC
 */
enum class Operator : unsigned int {
  ASSIGN,  ///< Assign
  FASSIGN, ///< Float assign
  PASSIGN, ///< Pointer assign

  /* Integer operators */
  ADD, ///< Add two values and store in result
  SUB, ///< Subtract two values and store in result
  MUL, ///< Multiply two values and store in result
  DIV, ///< Divide two values and store in result
  MOD, ///< Perform modulo of two values and store in result

  /* Float operators  */
  FADD, ///< Add two float values and store in result
  FSUB, ///< Subtract two float values and store in result
  FMUL, ///< Multiply two float values and store in result
  FDIV, ///< Divide two float values and store in result

  /* relational operators for expressions */
  EQUALS,         ///< result = arg1 == arg2
  NOT_EQUALS,     ///< result = arg1 != arg2
  GREATER,        ///< result = arg1 > arg2
  GREATER_EQUALS, ///< result = arg1 >= arg2
  LESS,           ///< result = arg1 < arg2
  LESS_EQUALS,    ///< result = arg1 <= arg2

  /* Operators for If */

  IF_UNARY,

  /* relational operators */
  IF_EQUALS,
  IF_NOT_EQUALS,
  IF_GREATER,
  IF_GREATER_EQUALS,
  IF_LESS,
  IF_LESS_EQUALS,

  /* float relational operators */
  IF_FE,
  IF_FNE,
  IF_FG,
  IF_FGE,
  IF_FLE,
  IF_FL,

  /* Operators for IfFalse */

  IF_FALSE_UNARY,

  /* relational operators */
  IF_FALSE_EQUALS,
  IF_FALSE_NOT_EQUALS,
  IF_FALSE_GREATER,
  IF_FALSE_GREATER_EQUALS,
  IF_FALSE_LESS,
  IF_FALSE_LESS_EQUALS,

  /* float relational operators */
  IF_FALSE_FE,
  IF_FALSE_FNE,
  IF_FALSE_FG,
  IF_FALSE_FGE,
  IF_FALSE_FLE,
  IF_FALSE_FL,

  /* boolean operators */
  NOT,
  AND,

  /* float relational operators */
  FE,
  FNE,
  FG,
  FGE,
  FLE,
  FL,

  MINUS,  // result = -arg1
  FMINUS, // result = -arg1

  I2F, // result = (float) arg1
  F2I, // result = (int) arg1

  DOT,  // result = (arg1)+arg2
  FDOT, // result = (arg1)+arg2
  PDOT, // result = address of arg1 + arg2

  DOT_ASSIGN,  // result+arg1=arg2
  DOT_FASSIGN, // result+arg1=arg2
  DOT_PASSIGN, // result+arg1=arg2

  GOTO, // jump to a basic block (label in arg1)

  RETURN, // return from a function

  NOP, // for optimization purpose

  PARAM,  // for parameter passing
  PPARAM, // for parameter passing by address

  CALL, // call functions

  LABEL // label in arg1
};

mtac::Operator toOperator(ast::Operator op);
mtac::Operator toFloatOperator(ast::Operator op);

mtac::Operator toRelationalOperator(ast::Operator op);
mtac::Operator toFloatRelationalOperator(ast::Operator op);

} //end of mtac

} //end of eddic

#endif
