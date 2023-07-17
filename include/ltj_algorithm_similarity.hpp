/*
 * ltj_algorithm.hpp
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



#ifndef RING_LTJ_ALGORITHM_SIMILARITY_HPP
#define RING_LTJ_ALGORITHM_SIMILARITY_HPP


#include <triple_pattern.hpp>
#include <ring_similarity.hpp>
#include <ltj_iterator.hpp>
#include <ltj_iterator_similarity.hpp>
#include <ltj_iterator_uni_similarity.hpp>
#include <gao_adaptive_sim_v3.hpp>
#include <gao_adaptive_sim_v2.hpp>
#include <gao_adaptive_sim_basic.hpp>
#include <descriptor.hpp>
#include <hash_vector.hpp>

namespace ring_ltj {

    template<class ring_t = ring_similarity<>,
             class var_t = uint8_t, class cons_t = uint64_t,
             class gao_t = gao::gao_adaptive_sim_v3<ring_t, var_t, cons_t> >
    class ltj_algorithm_similarity {

    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef cons_t const_type;
        typedef gao_t gao_type;
        typedef ltj_iterator_base<var_type, const_type> ltj_iter_type;
        typedef std::vector<const_type> const_vec_type;
        typedef ltj_iterator<ring_type, var_type, const_type> ltj_iter_basic_type;
        typedef ltj_iterator_similarity<ring_type, var_type, const_type> ltj_iter_bi_similarity_type;
        typedef ltj_iterator_uni_similarity<ring_type, var_type, const_type> ltj_iter_uni_similarity_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;

        typedef std::unordered_map<var_type, size_type> kr_pos_type; //to fingerprint
        typedef std::unordered_set<const_vec_type, ::hash::hash_vector, ::hash::equal_vector> kr_table_type;

        typedef std::vector<value_type> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;

        typedef std::unordered_map<pair_term_pattern, std::pair<size_type, size_type>, hash_pair_term_pattern> sim_table_type;

        typedef struct {
            tuple_type tuple_base;
            std::vector<value_type> kr_values;
            std::vector<descriptor> descriptors;
        } register_type;


    private:
        const std::vector<triple_pattern>* m_ptr_triple_patterns;
        gao_type m_gao;
        ring_type* m_ptr_ring;
        std::vector<ltj_iter_basic_type> m_iterators_basic;
        std::vector<ltj_iter_bi_similarity_type> m_iterators_bi_similarity;
        std::vector<ltj_iter_uni_similarity_type> m_iterators_uni_similarity;
        var_to_iterators_type m_var_to_iterators;
        bool m_is_empty = false;

        kr_pos_type m_kr_pos;
        kr_table_type m_kr_table;

        void copy(const ltj_algorithm_similarity &o) {
            m_ptr_triple_patterns = o.m_ptr_triple_patterns;
            m_gao = o.m_gao;
            m_ptr_ring = o.m_ptr_ring;
            m_iterators_basic = o.m_iterators_basic;
            m_iterators_bi_similarity = o.m_iterators_bi_similarity;
            m_iterators_uni_similarity = o.m_iterators_uni_similarity;
            m_var_to_iterators = o.m_var_to_iterators;
            m_is_empty = o.m_is_empty;
            m_kr_pos = o.m_kr_pos;
            m_kr_table = o.m_kr_table;
        }


        inline void add_var_to_iterator(const var_type var, ltj_iter_type* ptr_iterator){
            auto it =  m_var_to_iterators.find(var);
            if(it != m_var_to_iterators.end()){
                it->second.push_back(ptr_iterator);
            }else{
                std::vector<ltj_iter_type*> vec = {ptr_iterator};
                m_var_to_iterators.insert({var, vec});
            }
        }

        inline void add_var_to_index(const var_type var, size_type &index){
            auto it =  m_kr_pos.find(var);
            if(it == m_kr_pos.end()){
                m_kr_pos.insert({var, index});
                ++index;
            }
        }

        inline void cartesian_product_rec(tuple_type &tuple,
                                          size_type t_i,
                                          const std::vector<descriptor> &desc,
                                          size_type d_i,
                                          const std::vector<std::vector<value_type>> &desc_vv,
                                          std::vector<tuple_type> &tuples,
                                          size_type limit = 0){

            if(d_i == desc.size()){
                tuples.push_back(tuple);
            }else{
                for(const auto& value : desc_vv[d_i]){
                    tuple[desc[d_i].var] = value;
                    cartesian_product_rec(tuple, t_i+1, desc, d_i+1,  desc_vv,tuples, limit);
                    if(limit == tuples.size()) return;
                }
            }

        }

        inline void cartesian_product(tuple_type &base,
                                      std::vector<descriptor> &descriptors,
                                      std::vector<tuple_type> &tuples, size_type limit = 0){
            //base.resize(m_gao.size());
            std::vector<value_type> aux;
            std::vector<std::vector<value_type>> desc_vv;
            for(auto &d : descriptors){
                if(d.state == s){
                    aux = m_ptr_ring->all_S_in_range(d.interval);
                }else if (d.state == p){
                    aux = m_ptr_ring->all_P_in_range(d.interval);
                }else{
                    aux = m_ptr_ring->all_O_in_range(d.interval);
                }
                desc_vv.push_back(aux);
            }
            auto t_i = base.size()-descriptors.size();
            cartesian_product_rec( base, t_i, descriptors, 0, desc_vv, tuples, limit);

        }


        inline void get_tuples(std::vector<register_type> &regs, std::vector<tuple_type> &tuples, size_type limit = 0){
            for(auto &r : regs){
                //auto tuple_base = r.tuple_no_lonely;
                m_kr_table.insert(r.kr_values);
                cartesian_product( r.tuple_base, r.descriptors, tuples, limit);
            }
        }

        inline bool skip_step(var_type var, const_type value,
                                     size_type &cnt_sim, std::vector<value_type> &kr_values){

            auto it = m_kr_pos.find(var);
            if(it != m_kr_pos.end()){
                kr_values[it->second] = value;
                ++cnt_sim;
                if(cnt_sim < kr_values.size()) return false;
                return m_kr_table.find(kr_values) != m_kr_table.end();
            }
            return false;
        }

    public:


        ltj_algorithm_similarity() = default;

        ltj_algorithm_similarity(const std::vector<triple_pattern>* triple_patterns, ring_type* ring,
                                 const size_type num_vars){

            m_ptr_triple_patterns = triple_patterns;
            m_ptr_ring = ring;

            sim_table_type sim_table;


            size_type i_basic = 0,  i_similarity = 0, i_triple = 0;
            m_iterators_basic.reserve(m_ptr_triple_patterns->size());
            m_iterators_bi_similarity.reserve(m_ptr_triple_patterns->size());
            m_iterators_uni_similarity.reserve(m_ptr_triple_patterns->size());
            for(const auto& triple : *m_ptr_triple_patterns){
                //Bulding iterators
                if(triple.is_similarity()){
                    pair_term_pattern so {triple.term_s, triple.term_o};
                    auto it = sim_table.find(so);
                    if(it != sim_table.end()){
                        if(it->second.second == -1ULL){
                            it->second.second = i_triple;
                        }
                    }else{
                        std::pair<size_type, size_type> positions {i_triple, -1ULL};
                        pair_term_pattern so_rev {triple.term_o, triple.term_s};
                        sim_table.insert({so_rev, positions});
                    }
                    ++i_similarity;
                }else{
                    m_iterators_basic.emplace_back(ltj_iter_basic_type(&triple, m_ptr_ring));
                    if(m_iterators_basic[i_basic].is_empty()){
                        m_is_empty = true;
                        return;
                    }
                    //For each variable we add the pointers to its iterators
                    if(triple.o_is_variable()){
                        add_var_to_iterator(triple.term_o.value, &(m_iterators_basic[i_basic]));
                    }
                    if(triple.p_is_variable()){
                        add_var_to_iterator(triple.term_p.value, &(m_iterators_basic[i_basic]));
                    }
                    if(triple.s_is_variable()){
                        add_var_to_iterator(triple.term_s.value, &(m_iterators_basic[i_basic]));
                    }
                    ++i_basic;
                }
                ++i_triple;
            }

            //Bulding similarity iterators
            size_type i_uni_sim = 0, i_bi_sim = 0;
            for(const auto &sim : sim_table){
                auto fst_triple = sim.second.first;
                auto sec_triple = sim.second.second;
                if(sec_triple == -1ULL){
                    //Unidirectional
                    m_iterators_uni_similarity.emplace_back(
                            ltj_iter_uni_similarity_type (&m_ptr_triple_patterns->at(fst_triple), m_ptr_ring));
                    if(m_iterators_uni_similarity[i_uni_sim].is_empty()){
                        m_is_empty = true;
                        return;
                    }
                    const auto& triple = m_ptr_triple_patterns->at(fst_triple);
                    //For each variable we add the pointers to its iterators
                    if(triple.o_is_variable()){
                        add_var_to_iterator(triple.term_o.value, &(m_iterators_uni_similarity[i_uni_sim]));
                    }
                    if(triple.s_is_variable()){
                        add_var_to_iterator(triple.term_s.value, &(m_iterators_uni_similarity[i_uni_sim]));
                    }
                    ++i_uni_sim;
                }else{
                    //Bidirectional
                    m_iterators_bi_similarity.emplace_back(
                            ltj_iter_bi_similarity_type(&m_ptr_triple_patterns->at(fst_triple),
                                                               &m_ptr_triple_patterns->at(sec_triple), m_ptr_ring));
                    if(m_iterators_bi_similarity[i_bi_sim].is_empty()){
                        m_is_empty = true;
                        return;
                    }
                    const auto& triple = m_ptr_triple_patterns->at(fst_triple);
                    //For each variable we add the pointers to its iterators
                    if(triple.o_is_variable()){
                        add_var_to_iterator(triple.term_o.value, &(m_iterators_bi_similarity[i_bi_sim]));
                    }
                    if(triple.s_is_variable()){
                        add_var_to_iterator(triple.term_s.value, &(m_iterators_bi_similarity[i_bi_sim]));
                    }
                    ++i_bi_sim;
                }
            }

            m_gao = gao_type(&m_iterators_basic, &m_iterators_uni_similarity, &m_iterators_bi_similarity,
                             &m_var_to_iterators, num_vars, m_ptr_ring);

        }

        //! Copy constructor
        ltj_algorithm_similarity(const ltj_algorithm_similarity &o) {
            copy(o);
        }

        //! Move constructor
        ltj_algorithm_similarity(ltj_algorithm_similarity &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_algorithm_similarity &operator=(const ltj_algorithm_similarity &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_algorithm_similarity &operator=(ltj_algorithm_similarity &&o) {
            if (this != &o) {
                m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                m_gao = std::move(o.m_gao);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_iterators_basic = std::move(o.m_iterators_basic);
                m_iterators_bi_similarity = std::move(o.m_iterators_bi_similarity);
                m_iterators_uni_similarity = std::move(o.m_iterators_uni_similarity);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_is_empty = o.m_is_empty;
                m_kr_pos = o.m_kr_pos;
                m_kr_table = o.m_kr_table;
            }
            return *this;
        }

        void swap(ltj_algorithm_similarity &o) {
            std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
            std::swap(m_gao, o.m_gao);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_iterators_basic, o.m_iterators_basic);
            std::swap(m_iterators_bi_similarity, o.m_iterators_bi_similarity);
            std::swap(m_iterators_uni_similarity, o.m_iterators_uni_similarity);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_kr_pos, o.m_kr_pos);
            std::swap(m_kr_table, o.m_kr_table);
        }


        /**
         *
         * @param j                 Index of the variable
         * @param tuple             Tuple of the current search
         * @param res               Results
         * @param start             Initial time to check timeout
         * @param limit_results     Limit of results
         * @param timeout_seconds   Timeout in seconds
         */
        bool search(const size_type j, tuple_type &tuple, std::vector<tuple_type> &res,
                    const time_point_type start,
                    const size_type limit_results = 0, const size_type timeout_seconds = 0){

            //(Optional) Check timeout
            if(timeout_seconds > 0){
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop-start).count();
                if(sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if(limit_results > 0 && res.size() == limit_results) return false;

            if(j == m_gao.size()){
                //Report results
                res.emplace_back(tuple);
                //std::cout << "Adding tuple" << std::endl;
            }else{
                var_type x_j = m_gao.next();
               // std::cout << "At: " << j << " var: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                bool ok;
                if(itrs.size() == 1 && itrs[0]->in_last_level()) {//Lonely variables

                    value_type c = itrs[0]->seek_last(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j] = c;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        m_gao.down();
                        //2. Search with the next variable x_{j+1}
                        ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                        m_gao.up();

                        c = itrs[0]->seek_last_next(x_j);
                    }

                }else {
                    value_type c = seek(x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j] = c;
                        //std::cout << "Set " << (uint64_t) x_j << " to " << c << std::endl;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type* iter : itrs) {
                            iter->down(x_j, c);
                        }
                        m_gao.down();
                        //3. Search with the next variable x_{j+1}
                        ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter : itrs) {
                            iter->up(x_j);
                        }
                        m_gao.up();
                        //5. Next constant for x_j
                        c = seek(x_j, c + 1);
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    }
                }
                m_gao.done();
            }
            return true;
        };

        /**
        *
        * @param res               Results
        * @param limit_results     Limit of results
        * @param timeout_seconds   Timeout in seconds
        */
        void join(std::vector<tuple_type> &res,
                  const size_type limit_results = 0, const size_type timeout_seconds = 0){
            if(m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_gao.size());
            search(0, t, res, start, limit_results, timeout_seconds);
        };


        /******** Basic functions *******/

        /**
         *
         * @param x_j   Variable
         * @param c     Constant. If it is unknown the value is -1
         * @return      The next constant that matches the intersection between the triples of x_j.
         *              If the intersection is empty, it returns 0.
         */

       /* value_type seek(const var_type x_j, value_type c=-1){
            value_type c_i, c_min = UINT64_MAX, c_max = 0;
            std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
            while (true){
                //Compute leap for each triple that contains x_j
                for(ltj_iter_type* iter : itrs){
                    if(c == -1){
                        c_i = iter->leap(x_j);
                    }else{
                        c_i = iter->leap(x_j, c);
                    }
                    if(c_i == 0) {
                        return 0; //Empty intersection
                    }
                    if(c_i > c_max) c_max = c_i;
                    if(c_i < c_min) c_min = c_i;
                    c = c_max;
                }
                if(c_min == c_max) return c_min;
                c_min = UINT64_MAX; c_max = 0;
            }
        }*/

       value_type seek(const var_type x_j, value_type c=-1){
           std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
           value_type c_i, c_prev = 0, i = 0, n_ok = 0;
           while (true){
               //Compute leap for each triple that contains x_j

               if(c == -1){
                   c_i = itrs[i]->leap(x_j);
               }else{
                   c_i = itrs[i]->leap(x_j, c);
               }
               if(c_i == 0) return 0; //Empty intersection
               n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
               if(n_ok == itrs.size()) return c_i;
               c = c_prev = c_i;
               i = (i+1 == itrs.size()) ? 0 : i+1;
           }
       }

        void print_gao(std::unordered_map<uint8_t, std::string> &ht){
            std::cout << "GAO: " << std::endl;
            for(uint64_t j = 0; j < m_gao.size(); ++j){
                std::cout << "?" << ht[m_gao.next()] << " ";
            }
            std::cout << std::endl;
        }

        void print_query(std::unordered_map<uint8_t, std::string> &ht){
            std::cout << "Query: " << std::endl;
            for(size_type i = 0; i <  m_ptr_triple_patterns->size(); ++i){
                m_ptr_triple_patterns->at(i).print(ht);
                if(i < m_ptr_triple_patterns->size()-1){
                    std::cout << " . ";
                }
            }
            std::cout << std::endl;
        }

        void print_results(std::vector<tuple_type> &res, std::unordered_map<uint8_t, std::string> &ht){
            std::cout << "Results: " << std::endl;
            uint64_t i = 1;
            for(tuple_type &tuple :  res){
                std::cout << "[" << i << "]:\t";
                uint64_t j = 0;
                for(auto& val : tuple){
                    std::cout << "?" << ht[j] << "=" << val << " ";
                    ++j;
                }
                std::cout << std::endl;
                ++i;
            }
        }


    };
}

#endif //RING_LTJ_ALGORITHM_HPP
