//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BACK_END_H
#define BACK_END_H

#include <memory>
#include <string>

#include "Platform.hpp"
#include "Options.hpp"

#include "mtac/Program.hpp"

namespace eddic {

struct StringPool;

/*!
 * \class BackEnd
 * \brief Represent the back end part of the compilation process. 
 */
class BackEnd {
    public:
        /*!
         * Generates the backend output from the MTAC Program. 
         * \param mtacProgram The MTAC Program that has been generated by the front end. 
         * \param platform The target platform.
         */
        virtual void generate(std::shared_ptr<mtac::Program> mtacProgram, Platform platform) = 0;

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
