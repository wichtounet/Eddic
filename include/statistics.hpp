//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef STATISTICS_H
#define STATISTICS_H

#include <unordered_map>
#include <string>

namespace eddic {

class statistics {
    public:
        using Counters = std::unordered_map<std::string, std::size_t>;
        using iterator = Counters::const_iterator;

        void inc_counter(const std::string& a);
        std::size_t counter(const std::string& a) const;
        std::size_t counter_safe(const std::string& a) const;

        iterator begin() const;
        iterator end() const;

    private:
        Counters counters;
};

} //end of eddic

#endif
