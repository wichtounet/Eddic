//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef STRING_POOL_H
#define STRING_POOL_H

#include <string>
#include <unordered_map>
#include <mutex>

namespace eddic {

/*!
 * \struct StringPool
 * \brief The string pool of the program. 
 * All the strings are stored and referred only by an index.  
 */
struct StringPool {
    private:
        std::unordered_map<std::string, std::string> pool;
        unsigned int currentString;

//        mutable std::mutex mutex;

    public:
        StringPool();
        
        StringPool(const StringPool& rhs) = delete;
        StringPool& operator=(const StringPool& rhs) = delete;

        /*!
         * \brief Return the label for the given value. 
         * If the given value is not in the pool, it will be inserted with a new label. 
         * \n\n \b Complexity : O(1)
         * \param value The string we want to search in the pool. 
         * \return The label associated with the given value. 
         */
        std::string label(const std::string& value);
        
        /*!
         * \brief Return the value for the given label. 
         * This function should only be used for existing labels.
         * \n\n \b Complexity : O(n)
         * \param label The label we want to search in the pool. 
         * \return The value associated with the given label. 
         */
        std::string value(const std::string& label) const ;

        std::unordered_map<std::string, std::string> getPool() const;
};

} //end of eddic

#endif
