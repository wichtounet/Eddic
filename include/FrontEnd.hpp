//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef FRONT_END_H
#define FRONT_END_H

#include <memory>
#include <string>

#include "Platform.hpp"
#include "Options.hpp"

#include "mtac/Program.hpp"

namespace eddic {

struct StringPool;

/*!
 * \class FrontEnd
 * \brief Represent the front end part of the compilation process. 
 */
struct FrontEnd {
    /*!
     * Compile the given file into its MTAC Representation. 
     * \param file The file that has to be compiled. 
     * \return The MTAC Program representing the source program. 
     */
    virtual std::unique_ptr<mtac::Program> compile(const std::string& file, Platform platform, GlobalContext & context) = 0;   

    virtual ~FrontEnd() = default;

    /*!
     * Set the string pool. 
     * \param pool The string pool. 
     */
    void set_string_pool(std::shared_ptr<StringPool> pool);
    
    /*!
     * Returns the string pool. 
     * \return The string pool. 
     */
    std::shared_ptr<StringPool> get_string_pool();

    void set_configuration(std::shared_ptr<Configuration> configuration);
    std::shared_ptr<Configuration> get_configuration();

protected:
    std::shared_ptr<StringPool> pool; /**< The string pool that is used during this compilation */
    std::shared_ptr<Configuration> configuration; /**< The configuration of the compilation */
};

}

#endif
