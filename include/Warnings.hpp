//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef WARNINGS_H
#define WARNINGS_H

#include <string>

namespace eddic {

//TODO Warn functions should be moved into the global error handler

/*!
 * \brief Produces a warning on the command line.
 * \param warning The warning message to produce.
 */
void warn(const std::string& warning);

/*!
 * \brief Produces a warning on the command line and display the source of the warning.
 * \param position The position of the warning in the source file.
 * \param warning The warning message to produce.
 */
void warn(const std::string& position, const std::string& warning);

} //end of eddic

#endif
