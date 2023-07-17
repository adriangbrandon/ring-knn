/*
 * utils.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include "ring.hpp"


namespace ring_ltj {

    namespace utils {

        struct trait_size {

                template<class Iterator, class Ring>
                static uint64_t get(Ring* ptr_ring, const Iterator &iter){
                    return iter.interval_length();
                }

                template<class Iterator, class Ring>
                static uint64_t subject(Ring* ptr_ring, const Iterator &iter){
                    return iter.interval_length();
                }

                template<class Iterator, class Ring>
                static uint64_t predicate(Ring* ptr_ring, const Iterator &iter){
                    return iter.interval_length();
                }

                template<class Iterator, class Ring>
                static uint64_t object(Ring* ptr_ring, const Iterator &iter) {
                    return iter.interval_length();
                }

                template<class Iterator, class Ring>
                static uint64_t subject_sim(Ring* ptr_ring, const Iterator &iter) {
                    if (iter.in_last_level()) {
                        //return iter.distinct();
                        return iter.distinct();
                    }
                    return ptr_ring->max_s;
                }

                template<class Iterator, class Ring>
                static uint64_t object_sim(Ring* ptr_ring, const Iterator &iter){
                    if (iter.in_last_level()) {
                        return iter.distinct();
                        //return 0;
                    }
                    return ptr_ring->max_o;
                }

        };

    }

}



#endif 
