//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef AST_TRUE_H
#define AST_TRUE_H

namespace eddic {

namespace ast {

/*!
 * \class True
 * \brief Reprensent a true boolean litteral. 
 */
struct True {
    
};

std::ostream& operator<< (std::ostream& stream, True true_);

} //end of ast

} //end of eddic

#endif
