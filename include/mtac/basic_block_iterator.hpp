//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_BASIC_BLOCK_ITERATOR_H
#define MTAC_BASIC_BLOCK_ITERATOR_H

#include <iterator>

#include "mtac/forward.hpp"

namespace eddic {

namespace mtac {

template<typename BB>
struct basic_block_base_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = BB;
    using difference_type = std::ptrdiff_t;
    using pointer= BB*;
    using reference = BB&;

        basic_block_base_iterator(BB current, BB prev) : current(current), prev(prev) {}

        basic_block_base_iterator<BB>& operator++() {
            prev = current;
            current = current->next;
            return *this;
        }

        basic_block_base_iterator<BB> operator++(int) {
            basic_block_base_iterator<BB> tmp(*this); 
            operator++(); 
            return tmp;
        }
        
        basic_block_base_iterator<BB>& operator--() {
            current = prev;
            if(current){
                prev = current->prev;
            }
            return *this;
        }

        basic_block_base_iterator<BB> operator--(int) {
            basic_block_base_iterator tmp(*this); 
            operator--(); 
            return tmp;
        }

        bool operator==(const basic_block_base_iterator<BB>& rhs) const {
            return current == rhs.current;
        }

        bool operator!=(const basic_block_base_iterator<BB>& rhs) const {
            return current != rhs.current;
        }

        BB& operator*() {
            return current;
        }
            
    private:
        BB current;
        BB prev;
};

typedef basic_block_base_iterator<mtac::basic_block_p> basic_block_iterator;
typedef basic_block_base_iterator<mtac::basic_block_cp> basic_block_const_iterator;

} //end of mtac

} //end of eddic

#endif
