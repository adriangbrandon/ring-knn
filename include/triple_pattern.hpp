/*
 * triple_pattern.hpp
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



#ifndef RING_TRIPLE_PATTERN_HPP
#define RING_TRIPLE_PATTERN_HPP

#include <unordered_map>

namespace ring_ltj {

    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    struct term_pattern {
        uint64_t value;
        bool is_variable;
    };

    struct pair_term_pattern {
        term_pattern term_s;
        term_pattern term_o;

        bool operator==(const pair_term_pattern &p) const {
            return p.term_s.is_variable == term_s.is_variable && p.term_s.value == term_s.value &&
                   p.term_o.is_variable == term_o.is_variable && p.term_o.value == term_o.value;
        }
    };

    struct hash_pair_term_pattern {

        std::size_t operator() (const pair_term_pattern &pair_term) const {
            std::size_t seed = 0;
            hash_combine(seed, pair_term.term_s.is_variable);
            hash_combine(seed, pair_term.term_s.value);
            hash_combine(seed, pair_term.term_o.is_variable);
            hash_combine(seed, pair_term.term_o.value);
            return seed;
        }

    };

    struct triple_pattern {
        term_pattern term_s;
        term_pattern term_p;
        term_pattern term_o;
        uint64_t k_sim = 0;
        uint64_t k_best = 0;

        void const_s(uint64_t s){
            term_s.is_variable = false;
            term_s.value = s;
        }

        void const_o(uint64_t o){
            term_o.is_variable = false;
            term_o.value = o;
        }

        void const_p(uint64_t p){
            term_p.is_variable = false;
            term_p.value = p;
        }

        void var_s(uint64_t s){
            term_s.is_variable = true;
            term_s.value = s;

        }

        void var_o(uint64_t o){
            term_o.is_variable = true;
            term_o.value = o;
        }

        void var_p(uint64_t p){
            term_p.is_variable = true;
            term_p.value = p;
        }

        void similarity(uint64_t k){
            term_p.is_variable = false;
            k_sim = k;
        }

        void best(uint64_t k){
            term_p.is_variable = false;
            k_sim = 1;
            k_best = k;
        }

        bool s_is_variable() const {
            return term_s.is_variable;
        }

        bool p_is_variable() const {
            return term_p.is_variable;
        }

        bool o_is_variable() const {
            return term_o.is_variable;
        }

        bool is_similarity() const {
            return k_sim > 0;
        }

        bool is_best() const {
            return k_best > 0;
        }


        void print(std::unordered_map<uint8_t, std::string> &ht) const {
            if(s_is_variable()){
                std::cout << "?" << ht[term_s.value] << " ";
            }else{
                std::cout << term_s.value << " ";
            }

            if(!is_similarity() && !is_best()){
                if(p_is_variable()){
                    std::cout << "?" << ht[term_p.value] << " ";
                }else{
                    std::cout << term_p.value << " ";
                }
            }else if (is_best()){
                std::cout << "b" << k_best << " ";
            }else{
                std::cout << "k" << k_sim << " ";
            }

            if(o_is_variable()){
                std::cout << "?" << ht[term_o.value];
            }else{
                std::cout << term_o.value;
            }
        }
    };
}


#endif //RING_TRIPLE_PATTERN_HPP
