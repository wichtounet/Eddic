//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef SPIRIT_PARSER_X3_H
#define SPIRIT_PARSER_X3_H

#include "parser_x3/error_handling.hpp"
#include "ast/SourceFile.hpp"

namespace eddic {

struct GlobalContext;

namespace parser_x3 {

/*!
 * \struct SpiritParser
 * \brief The EDDI source file parser.
 *
 * This class takes an EDDI source file as input and produces an Abstract Syntax Tree as its output. The output syntax is created
 * using a Boost Spirit parser and a set of grammars describing the EDDI language.
 */
struct SpiritParser {
    /*!
     * \brief Parse the given source file and fills the given Abstract Syntax Tree.
     * \param file The path to the file to parse.
     * \param program The Abstract Syntax Tree root to fill.
     * \return true if the file was valid, false otherwise
     */
    bool parse(const std::string& file, ast::SourceFile& program, GlobalContext & context);
};

}

} //end of eddic

#endif
