/*
 * gao.hpp
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


#ifndef RING_GAO_ADAPTIVE_SIMILARITY_HPP
#define RING_GAO_ADAPTIVE_SIMILARITY_HPP

#include <ring_similarity.hpp>
#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>

namespace ring_ltj {

    namespace gao {


        template<class ring_t = ring<>,  class var_t = uint8_t,
                class const_t = uint64_t>
        class gao_adaptive_similarity {

        public:
            typedef var_t var_type;
            typedef const_t const_type;
            typedef uint64_t size_type;
            typedef ring_t ring_type;
            typedef ltj_iterator_base<var_type, const_type> ltj_iter_type;
            typedef ltj_iterator<ring_type, var_type, const_type> ltj_iter_basic_type;
            typedef ltj_iterator_similarity<ring_type, var_type, const_type> ltj_iter_similarity_type;


            /*enum spo_type {subject, predicate, object};
            typedef struct {
                std::vector<spo_type> vec_spo;
                size_type iter_pos;
            } spo_iter_type;*/

            typedef struct {
                var_type name;
                size_type weight;
                size_type pos;
                //std::vector<spo_iter_type> iterators;
                std::unordered_set<var_type> related;
                bool is_bound;
            } info_var_type;

            typedef  std::list<size_type> list_type;
            typedef  typename list_type::iterator list_iterator_type;

            typedef struct {
                size_type pos;
                size_type w;
            } update_type;


            typedef std::vector<update_type> version_type;

            typedef std::pair<size_type, var_type> pair_type;
            typedef utils::trait_size gao_trait_type;
            typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
            typedef std::stack<version_type> versions_type;
            typedef std::stack<size_type> bound_type;

        private:
            const std::vector<triple_pattern> *m_ptr_triple_patterns;
            const std::vector<ltj_iter_basic_type> *m_ptr_basic_iterators;
            const std::vector<ltj_iter_similarity_type> *m_ptr_sim_iterators;
            const var_to_iterators_type *m_ptr_var_iterators;
            ring_type *m_ptr_ring;
            std::unordered_map<var_type, size_type> m_hash_table_position;
            std::vector<info_var_type> m_var_info;
            std::vector<var_type> m_lonely;
            size_type m_index;
            list_type m_not_bound;
            bound_type m_bound;
            versions_type m_versions;



            void copy(const gao_adaptive_similarity &o) {
                m_ptr_triple_patterns = o.m_ptr_triple_patterns;
                m_ptr_basic_iterators = o.m_ptr_basic_iterators;
                m_ptr_sim_iterators = o.m_ptr_sim_iterators;
                m_ptr_var_iterators = o.m_ptr_var_iterators;
                m_ptr_ring = o.m_ptr_ring;
                m_hash_table_position = o.m_hash_table_position;
                m_not_bound = o.m_not_bound;
                m_lonely = o.m_lonely;
                m_index = o.m_index;
                m_bound = o.m_bound;
                m_versions = o.m_versions;
                m_var_info = o.m_var_info;
            }


            bool var_to_vector(const var_type var, const size_type size) {

                const auto &iters = m_ptr_var_iterators->at(var);
                if(iters.size() == 1){
                    m_lonely.emplace_back(var);
                    return false;
                }else{
                    auto it = m_hash_table_position.find(var);
                    if (it == m_hash_table_position.end()) {
                        info_var_type info;
                        info.name = var;
                        info.weight = size;
                        info.is_bound = false;
                        info.pos = m_var_info.size();
                        m_var_info.emplace_back(info);
                        m_hash_table_position.insert({var, info.pos});
                        m_not_bound.push_back(info.pos);
                    } else {
                        info_var_type &info = m_var_info[it->second];
                        if (info.weight > size) {
                            info.weight = size;
                        }
                    }
                    return true;
                }

            }



            void var_to_related(const var_type var, const var_type rel) {


                auto pos_var = m_hash_table_position[var];
                m_var_info[pos_var].related.insert(rel);
                auto pos_rel = m_hash_table_position[rel];
                m_var_info[pos_rel].related.insert(var);
            }


        public:

            gao_adaptive_similarity() = default;

            gao_adaptive_similarity(const std::vector<triple_pattern> *triple_patterns,
                       const std::vector<ltj_iter_basic_type> *basic_iterators,
                       const std::vector<ltj_iter_similarity_type> *sim_iterators,
                       const var_to_iterators_type *var_iterators,
                       ring_type *r) {
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_basic_iterators = basic_iterators;
                m_ptr_sim_iterators = sim_iterators;
                m_ptr_var_iterators = var_iterators;
                m_ptr_ring = r;


                //1. Filling var_info with data about each variable
                //std::cout << "Filling... " << std::flush;
                size_type nsim_i = 0, sim_i = 0;
                for (const triple_pattern &triple_pattern : *m_ptr_triple_patterns) {
                    bool s = false, p = false, o = false;
                    var_type var_s, var_p, var_o;
                    size_type size;
                    if(!triple_pattern.is_similarity()) {
                        if (triple_pattern.s_is_variable()) {
                            var_s = (var_type) triple_pattern.term_s.value;
                            size = gao_trait_type::subject(m_ptr_ring, m_ptr_basic_iterators->at(nsim_i));
                            s = var_to_vector(var_s, size);
                        }
                        if (triple_pattern.p_is_variable()) {
                            var_p = (var_type) triple_pattern.term_p.value;
                            size = gao_trait_type::predicate(m_ptr_ring, m_ptr_basic_iterators->at(nsim_i));
                            p = var_to_vector(var_p, size);
                        }
                        if (triple_pattern.o_is_variable()) {
                            var_o = (var_type) triple_pattern.term_o.value;
                            size = gao_trait_type::object(m_ptr_ring, m_ptr_basic_iterators->at(nsim_i));
                            o = var_to_vector(var_o, size);
                        }

                        if (s && p) {
                            var_to_related(var_s, var_p);
                        }
                        if (s && o) {
                            var_to_related(var_s, var_o);
                        }
                        if (p && o) {
                            var_to_related(var_p, var_o);
                        }
                        ++nsim_i;
                    }else {
                        if (triple_pattern.s_is_variable()) {
                            var_s = (var_type) triple_pattern.term_s.value;
                            size = gao_trait_type::subject_sim(m_ptr_ring, m_ptr_sim_iterators->at(sim_i));
                            s = var_to_vector(var_s, size);
                        }

                        if (triple_pattern.o_is_variable()) {
                            var_o = (var_type) triple_pattern.term_o.value;
                            size = gao_trait_type::object_sim(m_ptr_ring, m_ptr_sim_iterators->at(sim_i));
                            o = var_to_vector(var_o, size);
                        }

                        if (s && o) {
                            var_to_related(var_s, var_o);
                        }

                        ++sim_i;
                    }

                }
                m_index = 0;
                //std::cout << "Done. " << std::endl;

                //std::cout << "Done. " << std::endl;
            }

            //! Copy constructor
            gao_adaptive_similarity(const gao_adaptive_similarity &o) {
                copy(o);
            }

            //! Move constructor
            gao_adaptive_similarity(gao_adaptive_similarity &&o) {
                *this = std::move(o);
            }

            //! Copy Operator=
            gao_adaptive_similarity &operator=(const gao_adaptive_similarity &o) {
                if (this != &o) {
                    copy(o);
                }
                return *this;
            }

            //! Move Operator=
            gao_adaptive_similarity &operator=(gao_adaptive_similarity &&o) {
                if (this != &o) {
                    m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                    m_ptr_basic_iterators = std::move(o.m_ptr_basic_iterators);
                    m_ptr_sim_iterators = std::move(o.m_ptr_sim_iterators);
                    m_ptr_ring = std::move(o.m_ptr_ring);
                    m_ptr_var_iterators = o.m_ptr_var_iterators;
                    m_hash_table_position = std::move(o.m_hash_table_position);
                    m_lonely = std::move(o.m_lonely);
                    m_index = o.m_index;
                    m_bound = std::move(o.m_bound);
                    m_versions = std::move(o.m_versions);
                    m_var_info = std::move(o.m_var_info);
                    m_not_bound = std::move(o.m_not_bound);
                }
                return *this;
            }

            void swap(gao_adaptive_similarity &o) {
                std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
                std::swap(m_ptr_basic_iterators, o.m_ptr_basic_iterators);
                std::swap(m_ptr_sim_iterators, o.m_ptr_sim_iterators);
                std::swap(m_ptr_var_iterators, o.m_ptr_var_iterators);
                std::swap(m_ptr_ring, o.m_ptr_ring);
                std::swap(m_hash_table_position, o.m_hash_table_position);
                std::swap(m_lonely, o.m_lonely);
                std::swap(m_index, o.m_index);
                std::swap(m_bound, o.m_bound);
                std::swap(m_versions, o.m_versions);
                std::swap(m_var_info, o.m_var_info);
                std::swap(m_not_bound, o.m_not_bound);
            }

            inline var_type next() {

                if(m_index < m_var_info.size()){ //No lonely
                    size_type min = -1ULL;
                    list_iterator_type min_iter;
                    // Lineal search on variables that are not is_bound
                    for(auto iter = m_not_bound.begin(); iter != m_not_bound.end(); ++iter){
                        //Take the one with the smallest weight
                        const auto &v = m_var_info[*iter];
                        if(min > v.weight){
                            min = v.weight;
                            min_iter = iter;
                        }
                    }
                    //Remove from list
                    size_type min_pos = *min_iter;
                    m_var_info[min_pos].is_bound = true;
                    m_bound.emplace(min_pos);
                    m_not_bound.erase(min_iter); //Remove the variable from a list of unbound variables
                    ++m_index;
                    return  m_var_info[min_pos].name;
                }else{
                    //Return the next lonely variable
                    ++m_index;
                    return m_lonely[m_index-m_var_info.size()-1];
                }

            }



            inline void down() {

                if(m_index-1 < m_var_info.size()){ //No lonely

                    auto pos_last = m_bound.top();
                    const auto &related = m_var_info[pos_last].related;
                    version_type version;
                    for(const auto &rel : related){ //Iterates on the related variables
                        const auto pos = m_hash_table_position[rel];
                        if(!m_var_info[pos].is_bound){
                            bool u = false;
                            size_type min_w = m_var_info[pos].weight, w;
                            auto &iters = m_ptr_var_iterators->at(rel);
                            for(ltj_iter_type* iter : iters){ //Check each iterator

                                if(iter->is_variable_subject(rel)){
                                    if(iter->is_similarity()){
                                        auto s_iter = static_cast<ltj_iter_similarity_type*>(iter);
                                        w = gao_trait_type::subject_sim(m_ptr_ring, *s_iter);
                                    }else{
                                        auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                        w = gao_trait_type::subject(m_ptr_ring, *b_iter); //New weight
                                    }
                                }else if (iter->is_variable_predicate(rel)){
                                    auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                    w = gao_trait_type::predicate(m_ptr_ring, *b_iter);
                                }else{
                                    if(iter->is_similarity()){
                                        auto s_iter = static_cast<ltj_iter_similarity_type*>(iter);
                                        w = gao_trait_type::object_sim(m_ptr_ring, *s_iter);
                                    }else {
                                        auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                        w = gao_trait_type::object(m_ptr_ring, *b_iter);
                                    }
                                }
                                if(min_w > w) {
                                    min_w = w;
                                    u = true;
                                }
                            }

                            if(u){
                                update_type update{pos,  m_var_info[pos].weight};
                                version.emplace_back(update);  //Store an update
                                m_var_info[pos].weight = min_w;
                            }
                        }
                    }
                    m_versions.emplace(version); //Add the updates to versions
                }

            };

            inline void up() {
                if(m_index-1 < m_var_info.size()){ //No lonely
                    for(const update_type& update : m_versions.top()){ //Restart the weights
                        m_var_info[update.pos].weight = update.w;
                    }
                    m_versions.pop();
                }

            };

            inline void done() {
                --m_index;
                if(m_index < m_var_info.size()) { //No lonely
                    auto pos = m_bound.top();
                    m_not_bound.push_back(pos); //Restart the list of unbound variables
                    m_var_info[pos].is_bound = false;
                    m_bound.pop();
                }
            }


            inline void restart() {
                m_bound = bound_type();
                m_not_bound.clear();
                for(auto &info : m_var_info){
                    info.is_bound = false;
                    m_not_bound.push_back(info.pos);
                }
                m_index = 0;

            }

            inline size_type size() {
                return m_var_info.size() + m_lonely.size();
            }

            inline size_type beg_lonely(){
                return m_var_info.size();
            }

            inline var_type get_lonely(size_type index){
                return m_lonely[index-m_var_info.size()];
            }
        };
    };
}

#endif //RING_GAO_SIZE_HPP
