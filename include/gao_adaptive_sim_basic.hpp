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


#ifndef RING_GAO_ADAPTIVE_SIM_BASIC_HPP
#define RING_GAO_ADAPTIVE_SIM_BASIC_HPP

#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>
#include <ring.hpp>

#define PRINT_VARSET 0



namespace ring_ltj {

    namespace gao {

        template<class ring_t = ring<>,  class var_t = uint8_t,
                class const_t = uint64_t>
        class gao_adaptive_sim_basic {

        public:
            typedef var_t var_type;
            typedef const_t const_type;
            typedef uint64_t size_type;
            typedef ring_t ring_type;
            typedef ltj_iterator_base<var_type, const_type> ltj_iter_type;
            typedef ltj_iterator<ring_type, var_type, const_type> ltj_iter_basic_type;
            typedef ltj_iterator_similarity<ring_type, var_type, const_type>     ltj_iter_bi_similarity_type;
            typedef ltj_iterator_uni_similarity<ring_type, var_type, const_type> ltj_iter_uni_similarity_type;
            typedef var_sets_sccs<var_type, const_type> var_sets_type;

            /*enum spo_type {subject, predicate, object};
            typedef struct {
                std::vector<spo_type> vec_spo;
                size_type iter_pos;
            } spo_iter_type;*/

            typedef  std::list<size_type> list_type;
            typedef  typename list_type::iterator list_iterator_type;
            typedef enum {insert, erase, insert_scc} operation_enum_type;

            typedef struct {
                var_type var;
                size_type w;
            } update_weight_type;

            typedef struct {
                var_type var;
                set_enum_type set;
                operation_enum_type op;
            } update_set_type;



            typedef std::vector<update_weight_type> version_weight_type;
            typedef std::vector<update_set_type>    version_set_type;

            typedef utils::trait_size gao_trait_type;
            typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
            typedef std::stack<version_weight_type> versions_weight_type;
            typedef std::stack<version_set_type> versions_set_type;
            typedef std::stack<size_type> bound_type;

        private:
            const std::vector<ltj_iter_basic_type> *m_ptr_basic_iterators;
            const std::vector<ltj_iter_bi_similarity_type> *m_ptr_bi_sim_iterators;
            const std::vector<ltj_iter_uni_similarity_type> *m_ptr_uni_sim_iterators;
            const var_to_iterators_type *m_ptr_var_iterators;
            ring_type *m_ptr_ring;
            std::unordered_map<var_type, size_type> m_hash_table_position;
            var_sets_type m_var_sets;
            size_type m_index;
            size_type m_size_lonely;
            bound_type m_bound;
            versions_weight_type m_versions_weight;
            versions_set_type m_versions_set;



            void copy(const gao_adaptive_sim_basic &o) {
                m_ptr_basic_iterators = o.m_ptr_basic_iterators;
                m_ptr_bi_sim_iterators = o.m_ptr_bi_sim_iterators;
                m_ptr_uni_sim_iterators = o.m_ptr_uni_sim_iterators;
                m_ptr_var_iterators = o.m_ptr_var_iterators;
                m_ptr_ring = o.m_ptr_ring;
                m_hash_table_position = o.m_hash_table_position;
                m_index = o.m_index;
                m_size_lonely = o.m_size_lonely;
                m_bound = o.m_bound;
                m_versions_weight = o.m_versions_weight;
                m_versions_set = o.m_versions_set;
                m_var_sets = o.m_var_sets;
            }

            bool var_to_vector(const var_type var, const size_type size){
                const auto &info = m_var_sets.info[var];
                if (info.weight > size) {
                    m_var_sets.set_weight(var, size);
                }
                const auto &iters = m_ptr_var_iterators->at(var);
                return (iters.size() > 1);
            }

            /*bool var_to_vector(const var_type var, const size_type size) {

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

            }*/

            void similarity_vars(const var_type src, const var_type tgt) {
                m_var_sets.add_tgt(src, tgt);
                m_var_sets.inc_sim_cnt(tgt);
            }

            void linked_vars(const var_type var, const var_type rel) {
                m_var_sets.add_linked(var, rel);
                m_var_sets.add_linked(rel, var);
            }

        public:

            gao_adaptive_sim_basic() = default;

            gao_adaptive_sim_basic(const std::vector<ltj_iter_basic_type> *basic_iterators,
                       const std::vector<ltj_iter_uni_similarity_type> *uni_sim_iterators,
                       const std::vector<ltj_iter_bi_similarity_type> *bi_sim_iterators,
                       const var_to_iterators_type *var_iterators,
                       const size_type num_vars,
                       ring_type *r) {

                m_ptr_basic_iterators = basic_iterators;
                m_ptr_bi_sim_iterators = bi_sim_iterators;
                m_ptr_uni_sim_iterators = uni_sim_iterators;
                m_ptr_var_iterators = var_iterators;
                m_ptr_ring = r;
                m_var_sets = var_sets_type(num_vars);

                bool s_non_lonely = false, p_non_lonely = false, o_non_lonely = false;
                var_type var_s, var_p, var_o;
                size_type size, n_variables;

                //1. Basic iterators
                for(const auto & b_iter : *m_ptr_basic_iterators){
                    const triple_pattern* triple_pattern = b_iter.ptr_triple_pattern;
                    s_non_lonely = false; p_non_lonely = false; o_non_lonely = false;
                    if (triple_pattern->s_is_variable()) {
                        var_s = (var_type) triple_pattern->term_s.value;
                        size = gao_trait_type::subject(m_ptr_ring, b_iter);
                        s_non_lonely = var_to_vector(var_s, size);
                    }
                    if (triple_pattern->p_is_variable()) {
                        var_p = (var_type) triple_pattern->term_p.value;
                        size = gao_trait_type::predicate(m_ptr_ring, b_iter);
                        p_non_lonely = var_to_vector(var_p, size);
                    }
                    if (triple_pattern->o_is_variable()) {
                        var_o = (var_type) triple_pattern->term_o.value;
                        size = gao_trait_type::object(m_ptr_ring, b_iter);
                        o_non_lonely = var_to_vector(var_o, size);
                    }

                    if (s_non_lonely && p_non_lonely) {
                        linked_vars(var_s, var_p);
                    }
                    if (s_non_lonely && o_non_lonely) {
                        linked_vars(var_s, var_o);
                    }
                    if (p_non_lonely && o_non_lonely) {
                        linked_vars(var_p, var_o);
                    }
                }

                std::unordered_map<var_type, size_type> dict;
                //2. Uni-Sim iterators
                for(const auto & uni_iter : *m_ptr_uni_sim_iterators){
                    s_non_lonely = false; p_non_lonely = false; o_non_lonely = false;
                    n_variables = 0;
                    const triple_pattern* triple_pattern = uni_iter.ptr_triple_pattern;
                    if (triple_pattern->s_is_variable()) {
                        var_s = (var_type) triple_pattern->term_s.value;
                        size = gao_trait_type::subject_sim(m_ptr_ring, uni_iter);
                        s_non_lonely = var_to_vector(var_s, size);
                        ++n_variables;
                    }

                    if (triple_pattern->o_is_variable()) {
                        var_o = (var_type) triple_pattern->term_o.value;
                        size = gao_trait_type::object_sim(m_ptr_ring, uni_iter);
                        o_non_lonely = var_to_vector(var_o, size);
                        ++n_variables;

                    }

                    if (n_variables == 2){
                        similarity_vars(var_s, var_o);
                    }

                    if (s_non_lonely && o_non_lonely) { //No lonely
                        linked_vars(var_s, var_o);

                    }
                }

                //3. Bi-Sim iterators
                for(const auto & bi_iter : *m_ptr_bi_sim_iterators){
                    s_non_lonely = false; p_non_lonely = false; o_non_lonely = false;
                    n_variables = 0;
                    const triple_pattern* triple_pattern = bi_iter.ptr_triple_patterns[0];
                    if (triple_pattern->s_is_variable()) {
                        var_s = (var_type) triple_pattern->term_s.value;
                        size = gao_trait_type::subject_sim(m_ptr_ring, bi_iter);
                        s_non_lonely = var_to_vector(var_s, size);
                        ++n_variables;
                    }

                    if (triple_pattern->o_is_variable()) {
                        var_o = (var_type) triple_pattern->term_o.value;
                        size = gao_trait_type::object_sim(m_ptr_ring,  bi_iter);
                        o_non_lonely = var_to_vector(var_o, size);
                        ++n_variables;
                    }

                    if (s_non_lonely && o_non_lonely) { //No lonely
                        linked_vars(var_s, var_o);
                    }
                    if (n_variables == 2){
                        similarity_vars(var_s, var_o);
                        similarity_vars(var_o, var_s);
                    }
                }

                m_index = 0;
                m_size_lonely = 0;
                //Add each var to its set
                for(var_type var = 0; var < m_var_sets.size(); ++var){
                    if(m_ptr_var_iterators->at(var).size() == 1){ //Lonely
                        m_var_sets.insert(var, set_enum_type::lonely);
                        ++m_size_lonely;
                    }else{ //Ready
                        m_var_sets.insert(var, set_enum_type::ready);
                    }
                }


#if PRINT_VARSET
                m_var_sets.print();
                std::cout << "Constructor. " << std::endl;
#endif
                //std::cout << "Done. " << std::endl;
            }

            //! Copy constructor
            gao_adaptive_sim_basic(const gao_adaptive_sim_basic &o) {
                copy(o);
            }

            //! Move constructor
            gao_adaptive_sim_basic(gao_adaptive_sim_basic &&o) {
                *this = std::move(o);
            }

            //! Copy Operator=
            gao_adaptive_sim_basic &operator=(const gao_adaptive_sim_basic &o) {
                if (this != &o) {
                    copy(o);
                }
                return *this;
            }

            //! Move Operator=
            gao_adaptive_sim_basic &operator=(gao_adaptive_sim_basic &&o) {
                if (this != &o) {
                    m_ptr_basic_iterators = std::move(o.m_ptr_basic_iterators);
                    m_ptr_bi_sim_iterators = std::move(o.m_ptr_bi_sim_iterators);
                    m_ptr_uni_sim_iterators = std::move(o.m_ptr_uni_sim_iterators);
                    m_ptr_ring = std::move(o.m_ptr_ring);
                    m_ptr_var_iterators = o.m_ptr_var_iterators;
                    m_hash_table_position = std::move(o.m_hash_table_position);
                    m_var_sets = std::move(o.m_var_sets);
                    m_index = o.m_index;
                    m_bound = std::move(o.m_bound);
                    m_versions_weight = std::move(o.m_versions_weight);
                    m_versions_set = std::move(o.m_versions_set);
                }
                return *this;
            }

            void swap(gao_adaptive_sim_basic &o) {
                std::swap(m_ptr_basic_iterators, o.m_ptr_basic_iterators);
                std::swap(m_ptr_bi_sim_iterators, o.m_ptr_bi_sim_iterators);
                std::swap(m_ptr_uni_sim_iterators, o.m_ptr_uni_sim_iterators);
                std::swap(m_ptr_var_iterators, o.m_ptr_var_iterators);
                std::swap(m_ptr_ring, o.m_ptr_ring);
                std::swap(m_hash_table_position, o.m_hash_table_position);
                std::swap(m_var_sets, o.m_var_sets);
                std::swap(m_index, o.m_index);
                std::swap(m_bound, o.m_bound);
                std::swap(m_versions_weight, o.m_versions_weight);
                std::swap(m_versions_set, o.m_versions_set);
            }

            inline var_type next() {

                size_type min = UINT64_MAX;
                size_type sim_cnt = 0;
                var_type r = 0;
                version_set_type version_set;
                if(m_var_sets.empty(set_enum_type::ready)){ //Lonely
                    r = *m_var_sets.begin(set_enum_type::lonely);
                    //Remove it from the set
                    m_var_sets.erase(r, set_enum_type::lonely);
                    //Record of erasing the variable from the lonely set
                    update_set_type update_set{r, set_enum_type::lonely, operation_enum_type::erase};
                    version_set.emplace_back(update_set);
                }else{

                    //Choose the set that is not empty
                    //set_enum_type set = !m_var_sets.empty(set_enum_type::ready) ?
                    //        set_enum_type::ready : set_enum_type::sim;
                    set_enum_type set = set_enum_type::ready;
                    // Linear search on variables that are not bounded
                    for(auto iter = m_var_sets.begin(set); iter != m_var_sets.end(set); ++iter){
                        const auto &v = m_var_sets.info[*iter];
                        //Take the one with the smallest weight
                        if(min > v.weight){
                            min = v.weight;
                            sim_cnt = v.sim_cnt;
                            r = *iter;
                        }else if (min == v.weight && v.sim_cnt < sim_cnt){
                            min = v.weight;
                            sim_cnt = v.sim_cnt;
                            r = *iter;
                        }
                    }

                    //Remove it from the set
                    m_var_sets.erase(r, set);
                    //Record of erasing the variable from the chosen set
                    update_set_type update_set{r, set, operation_enum_type::erase};
                    version_set.emplace_back(update_set);

                    for(auto &tgt : m_var_sets.info[r].sim_tgts){
                        if(!m_var_sets.is_lonely(tgt) && !m_var_sets.is_bound(tgt)){
                            m_var_sets.dec_sim_cnt(tgt);
                        }
                    }

                }
                ++m_index;
                m_bound.push(r);
                m_var_sets.set_is_bound(r, true);
                m_versions_set.emplace(version_set);
#if PRINT_VARSET
                m_var_sets.print();
                std::cout << "Next. " << std::endl;
#endif
                return r;
            }



            inline void down() { //Given the new constant sets the new weights

                if(m_index-1 < m_size_lonely){ //No lonely
                    auto var = m_bound.top();
                    //const auto &linked = m_var_sets[var].linked;
                    version_weight_type version_weight;
                    for(const auto &link : m_var_sets.info[var].linked){ //Iterates on the linked variables
                        if(!m_var_sets.info[link].is_bound){
                            bool u = false;
                            size_type min_w = m_var_sets.info[link].weight, w;
                            auto &iters = m_ptr_var_iterators->at(link);
                            for(ltj_iter_type* iter : iters){ //Check each iterator
                                if(iter->is_variable_subject(link)){
                                    switch (iter->is_similarity()) {
                                        case 2: {
                                            auto s_iter = static_cast<ltj_iter_bi_similarity_type*>(iter);
                                            w = gao_trait_type::subject_sim(m_ptr_ring, *s_iter);
                                            break;
                                        }
                                        case 1: {
                                            auto s_iter = static_cast<ltj_iter_uni_similarity_type*>(iter);
                                            w = gao_trait_type::subject_sim(m_ptr_ring, *s_iter);
                                            break;
                                        }
                                        case 0: {
                                            auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                            w = gao_trait_type::subject(m_ptr_ring, *b_iter); //New weight
                                            break;
                                        }
                                    }
                                }else if (iter->is_variable_predicate(link)){
                                    auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                    w = gao_trait_type::predicate(m_ptr_ring, *b_iter);
                                }else{
                                    switch (iter->is_similarity()) {
                                        case 2: {
                                            auto s_iter = static_cast<ltj_iter_bi_similarity_type*>(iter);
                                            w = gao_trait_type::object_sim(m_ptr_ring, *s_iter);
                                            break;
                                        }
                                        case 1: {
                                            auto s_iter = static_cast<ltj_iter_uni_similarity_type*>(iter);
                                            w = gao_trait_type::object_sim(m_ptr_ring, *s_iter);
                                            break;
                                        }
                                        case 0: {
                                            auto b_iter = static_cast<ltj_iter_basic_type*>(iter);
                                            w = gao_trait_type::object(m_ptr_ring, *b_iter); //New weight
                                            break;
                                        }
                                    }
                                }
                                if(min_w > w) {
                                    min_w = w;
                                    u = true;
                                }
                            }

                            if(u){
                                update_weight_type update{link,  m_var_sets.info[link].weight};
                                version_weight.emplace_back(update);  //Store an update
                                m_var_sets.set_weight(link, min_w);
                            }
                        }
                    }
                    m_versions_weight.emplace(version_weight); //Add the updates to versions
                }
            };

            inline void up() { //Restores the previous weights
                if(m_index-1 < m_size_lonely){ //No lonely
                    for(const update_weight_type& update : m_versions_weight.top()){ //Restart the weights
                        m_var_sets.set_weight(update.var, update.w);
                    }
                    m_versions_weight.pop();
                }
            };

            inline void done() { //Restores the sets
                --m_index;
                var_type var = m_bound.top();
                m_bound.pop();
                auto top = m_versions_set.top();
                for(auto it = top.rbegin(); it != top.rend();  ++it){ //Restart the sets
                    auto &update = *it;
                    if(update.op == operation_enum_type::insert){
                        m_var_sets.erase(update.var, update.set);
                    }else if (update.op == operation_enum_type::erase){
                        m_var_sets.insert(update.var, update.set);
                    }
                }
                const auto &v = m_var_sets.info[var];
                for(auto &tgt : v.sim_tgts){
                    if(!m_var_sets.is_bound(tgt) && !m_var_sets.is_lonely(tgt)){
                        m_var_sets.inc_sim_cnt(tgt);
                    }
                }
                m_var_sets.set_is_bound(var, false);
                m_versions_set.pop();
#if PRINT_VARSET
                m_var_sets.print();
                std::cout << "Done." << std::endl;
#endif
            }

            inline size_type size() {
                return m_var_sets.size();
            }
        };
    };
}

#endif //RING_GAO_SIZE_HPP
