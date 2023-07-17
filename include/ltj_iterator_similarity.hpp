/*
 * ltj_iterator.hpp
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

#ifndef RING_LTJ_ITERATOR_SIMILARITY_HPP
#define RING_LTJ_ITERATOR_SIMILARITY_HPP

#define VERBOSE 0


#include <wt_intersection_iterator.hpp>

namespace ring_ltj {

    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_similarity : public ltj_iterator_base<var_t, cons_t> {

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef typename ring_type::knn_intersection_iterator_type knn_iterator_type;
        typedef typename ring_type::knn_intersection_helper_type knn_helper_type;
        typedef uint64_t size_type;
        //enum state_type {s, p, o};
        //std::vector<value_type> leap_result_type;

    private:
        ring_type *m_ptr_ring; //TODO: should be const
        bwt_interval m_i;
        std::array<value_type, 2> m_consts;
        std::array<descriptor_type, 2> m_state;
        size_type m_level = 0;
        knn_iterator_type m_knn_iter;
        knn_helper_type m_knn_help;
        bool m_is_empty = false;
        //std::stack<state_type> m_states;


        void copy(const ltj_iterator_similarity &o) {
            ptr_triple_patterns = o.ptr_triple_patterns;
            m_ptr_ring = o.m_ptr_ring;
            m_i = o.m_i;
            m_consts = o.m_consts;
            m_state = o.m_state;
            m_level = o.m_level;
            m_knn_iter = o.m_knn_iter;
            m_knn_help = o.m_knn_help;
            m_is_empty = o.m_is_empty;
        }

    public:
        //const value_type &cur_s = m_cur_s;
        //const value_type &cur_o = m_cur_o;
        std::array<const triple_pattern *, 2> ptr_triple_patterns;

        ltj_iterator_similarity() = default;

        ltj_iterator_similarity(const triple_pattern *triple1, const triple_pattern *triple2, ring_type *ring) {
            ptr_triple_patterns[0] = triple1;
            ptr_triple_patterns[1] = triple2;
            m_ptr_ring = ring;
            m_i = m_ptr_ring->open_POS();
            //Init current values and intervals according to the triple
            if (!ptr_triple_patterns[0]->s_is_variable() && !ptr_triple_patterns[0]->o_is_variable()) {
                //S->O to avoid forward steps

                //Interval in S
                //auto s_aux = m_ptr_ring->next_S(m_i, m_ptr_triple_pattern->term_s.value);
                //Is the constant of S in m_i_s?
                /*if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }*/
                m_consts[0] = ptr_triple_patterns[0]->term_s.value;
                m_state[0] = s;

                m_ptr_ring->knn_intersection_helper(m_consts[0], ptr_triple_patterns[0]->k_sim,
                                                  ptr_triple_patterns[1]->k_sim, m_knn_help);
                if(m_knn_help.is_empty()){
                    m_is_empty = true;
                    return;
                }
                auto o_aux = m_knn_help.next(ptr_triple_patterns[0]->term_o.value);
                //Is the constant of O in m_i_o?
                if (o_aux != ptr_triple_patterns[0]->term_o.value) {
                    m_is_empty = true;
                    return;
                }
                m_consts[1] = o_aux;
                m_state[1] = o;
                m_level = 2;

            } else if (!ptr_triple_patterns[0]->s_is_variable()) {

                //Interval in S
                //auto s_aux = m_ptr_ring->next_S(m_i, m_ptr_triple_pattern->term_s.value);
                //Is the constant of S in m_i_s?
                /*if (s_aux != m_ptr_triple_pattern->term_s.value) {
                    m_is_empty = true;
                    return;
                }*/

                m_consts[0] = ptr_triple_patterns[0]->term_s.value;
                m_ptr_ring->knn_intersection_helper(m_consts[0], ptr_triple_patterns[0]->k_sim,
                                                  ptr_triple_patterns[1]->k_sim, m_knn_help);
                if(m_knn_help.is_empty()){
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_level = 1;

            } else if (!ptr_triple_patterns[0]->o_is_variable()) {

                //Interval in O
                //auto o_aux = m_ptr_ring->next_O(m_i, m_ptr_triple_pattern->term_o.value);
                //Is the constant of P in m_i_p?
                /*if (o_aux != m_ptr_triple_pattern->term_o.value) {
                    m_is_empty = true;
                    return;
                }*/
                m_consts[0] = ptr_triple_patterns[0]->term_o.value;
                //m_ptr_ring->knn_intersection_iter(m_consts[0], ptr_triple_patterns[0]->k_sim,
                //                                  ptr_triple_patterns[1]->k_sim, m_knn_iter);
                m_ptr_ring->knn_intersection_helper(m_consts[0], ptr_triple_patterns[1]->k_sim,
                                                  ptr_triple_patterns[0]->k_sim, m_knn_help);
                if(m_knn_help.is_empty()){
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_level = 1;
            }
        }

        //! Copy constructor
        ltj_iterator_similarity(const ltj_iterator_similarity &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_similarity(ltj_iterator_similarity &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_similarity &operator=(const ltj_iterator_similarity &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_similarity &operator=(ltj_iterator_similarity &&o) {
            if (this != &o) {
                ptr_triple_patterns = std::move(o.ptr_triple_patterns);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_i = std::move(o.m_i);
                m_consts = std::move(o.m_consts);
                m_state = std::move(o.m_state);
                m_level = o.m_level;
                m_knn_iter = std::move(o.m_knn_iter);
                m_knn_help = std::move(o.m_knn_help);
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_iterator_similarity &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(ptr_triple_patterns, o.ptr_triple_patterns);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            m_i.swap(o.m_i);
            std::swap(m_consts, o.m_consts);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_knn_iter, o.m_knn_iter);
            std::swap(m_knn_help, o.m_knn_help);
            std::swap(m_is_empty, o.m_is_empty);
        }



        inline bool is_variable_subject(var_type var) {
            return ptr_triple_patterns[0]->term_s.is_variable && var == ptr_triple_patterns[0]->term_s.value;
        }

        inline bool is_variable_predicate(var_type var) {
            return false;
        }

        inline bool is_variable_object(var_type var) {
            return ptr_triple_patterns[0]->term_o.is_variable && var == ptr_triple_patterns[0]->term_o.value;
        }

        void down(var_type var, size_type c) { //Go down in the trie
            if (m_level > 1) return;
            if (m_level == 0) {
                //m_ptr_ring->knn_intersection_iter(c, ptr_triple_patterns[0]->k_sim,
                //                                  ptr_triple_patterns[1]->k_sim, m_knn_iter);
                if(is_variable_subject(var)){
                    m_state[m_level] = s;
                    m_ptr_ring->knn_intersection_helper(c, ptr_triple_patterns[0]->k_sim,
                                                      ptr_triple_patterns[1]->k_sim, m_knn_help);
                }else{
                    m_state[m_level] = o;
                    m_ptr_ring->knn_intersection_helper(c, ptr_triple_patterns[1]->k_sim,
                                                      ptr_triple_patterns[0]->k_sim, m_knn_help);
                }
            }else{
                if(is_variable_subject(var)){
                    m_state[m_level] = s;
                }else{
                    m_state[m_level] = o;
                }
            }
            m_consts[m_level] = c;
            ++m_level;
        };

        void down(var_type var, size_type c, size_type k) { //Go down in the trie
            if (m_level > 1) return;
            if (m_level == 0) {
                m_ptr_ring->knn_intersection_helper(c, k, k, m_knn_help);
            }
            if(is_variable_subject(var)){
                m_state[m_level] = s;
            }else{
                m_state[m_level] = o;
            }
            m_consts[m_level] = c;
            ++m_level;
            //std::cout << "DOWN" << std::endl;
        };

        void up(var_type var) { //Go up in the trie
            if(m_level == 0) return;
            --m_level;
            //std::cout << "UP" << std::endl;
        };

        value_type leap(var_type var) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
            /*if(m_level == 0){

                value_type c =  m_ptr_ring->min_S(m_i);
                size_type n_ok = 1;
                value_type c_i, i = 1, c_prev = c;
                while (true) {
                    if (i == 0) {
                        c_i = m_ptr_ring->next_S(m_i, c);
                    } else {
                        c_i = m_ptr_ring->next_O(m_i, c);
                    }
                    if (c_i == 0) return 0; //Empty intersection
                    n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
                    if (n_ok == 2) return c_i;
                    c = c_prev = c_i;
                    i = (i + 1 == 2) ? 0 : i + 1;
                }
            }*/
            if(m_level == 0) return 1;
            return m_knn_help.next();
        };

        value_type leap(var_type var, size_type c) { //Return the next value greater or equal than c in the range
            /*if(m_level == 0){
                size_type n_ok = 0;
                value_type c_i, i = 0, c_prev = c;
                while (true) {
                    if (i == 0) {
                        c_i = m_ptr_ring->next_S(m_i, c);
                    } else {
                        c_i = m_ptr_ring->next_O(m_i, c);
                    }
                    if (c_i == 0) return 0; //Empty intersection
                    n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
                    if (n_ok == 2) return c_i;
                    c = c_prev = c_i;
                    i = (i + 1 == 2) ? 0 : i + 1;
                }
            }*/
            if(m_level == 0) {
                if(c > m_ptr_ring->knn_nodes) return 0;
                return c;
            }
            //std::cout << "LEAP" << std::endl;
            return m_knn_help.next(c);
        }

        bool in_last_level() const{
            return m_level == 1;
        }

        size_type distinct() const{
            return m_knn_help.distinct();
        }


        value_type seek_last(var_type var){
            m_ptr_ring->knn_intersection_iter(m_consts[0], ptr_triple_patterns[0]->k_sim,
                                                ptr_triple_patterns[1]->k_sim, m_knn_iter);
            return m_knn_iter.next();
        }

        value_type seek_last_next(var_type var){
            return m_knn_iter.next();
        }

        inline descriptor get_descriptor(var_type var){
            descriptor desc;
            return desc;
        }

        bool is_empty(){
            return m_is_empty;
        }

        size_type is_similarity(){
            return 2;
        }
    };

}

#endif //RING_LTJ_ITERATOR_HPP
