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



#ifndef RING_LTJ_ALGORITHM_SIMILARITY_BASELINE_HPP
#define RING_LTJ_ALGORITHM_SIMILARITY_BASELINE_HPP


#include <triple_pattern.hpp>
#include <ring_knn_naive_v2.hpp>
#include <ltj_iterator.hpp>
#include <gao_adaptive.hpp>
#include <descriptor.hpp>
#include <hash_vector.hpp>
#include <unordered_set>

namespace ring_ltj {

    template<class ring_t = ring_knn_naive_v2<>,
             class var_t = uint8_t, class cons_t = uint64_t,
             class gao_t = gao::gao_adaptive<ring_t, var_t, cons_t> >
    class ltj_algorithm_similarity_baseline {

    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef cons_t const_type;
        typedef gao_t gao_type;
        typedef std::vector<const_type> const_vec_type;
        typedef ltj_iterator<ring_type, var_type, const_type> ltj_iter_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;

        typedef std::unordered_map<var_type, size_type> kr_pos_type; //to fingerprint
        typedef std::unordered_set<const_vec_type, ::hash::hash_vector, ::hash::equal_vector> kr_table_type;

        typedef std::vector<value_type> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;

        typedef std::unordered_set<size_type> type1_sim_patterns_set_type;
        typedef std::unordered_set<size_type> type2_sim_patterns_set_type;
        typedef std::unordered_set<size_type> bound_variables_set_type;

        typedef struct {
            tuple_type tuple_base;
            std::vector<value_type> kr_values;
            std::vector<descriptor> descriptors;
        } register_type;


    private:
        const std::vector<triple_pattern>* m_ptr_triple_patterns;
        const std::vector<triple_pattern>* m_ptr_triple_sim_patterns;
        gao_type m_gao;
        ring_type* m_ptr_ring;
        std::vector<ltj_iter_type> m_iterators;
        std::vector<size_type> m_weight_sim_patterns;
        var_to_iterators_type m_var_to_iterators;
        bool m_is_empty = false;

        bound_variables_set_type m_bound_variables_set;
        type1_sim_patterns_set_type m_type1_set;
        type2_sim_patterns_set_type m_type2_set;
        size_type m_var_size;



        void copy(const ltj_algorithm_similarity_baseline &o) {
            m_ptr_triple_patterns = o.m_ptr_triple_patterns;
            m_gao = o.m_gao;
            m_ptr_ring = o.m_ptr_ring;
            m_iterators = o.m_iterators;
            m_var_to_iterators = o.m_var_to_iterators;
            m_is_empty = o.m_is_empty;
            m_weight_sim_patterns = o.m_weight_sim_patterns;
            m_bound_variables_set = o.m_bound_variables_set;
            m_type1_set = o.m_type1_set;
            m_type2_set = o.m_type2_set;
            m_var_size = o.m_var_size;
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



    public:


        ltj_algorithm_similarity_baseline() = default;

        ltj_algorithm_similarity_baseline(const std::vector<triple_pattern>* triple_patterns,
                                          const std::vector<triple_pattern>* triple_sim_patterns,
                                          ring_type* ring){

            m_ptr_triple_patterns = triple_patterns;
            m_ptr_triple_sim_patterns = triple_sim_patterns;
            m_ptr_ring = ring;

            m_iterators.reserve(m_ptr_triple_patterns->size());
            m_ptr_triple_patterns = triple_patterns;
            m_ptr_ring = ring;

            std::unordered_set<var_type> vars;
            size_type i = 0, pos = 0;
            m_iterators.reserve(m_ptr_triple_patterns->size());
            for(const auto& triple : *m_ptr_triple_patterns){
                m_iterators.emplace_back(ltj_iter_type(&triple, m_ptr_ring));
                if(m_iterators[i].is_empty()){
                    m_is_empty = true;
                    return;
                }

                //For each variable we add the pointers to its iterators
                if(triple.o_is_variable()){
                    m_bound_variables_set.insert(triple.term_o.value);
                    add_var_to_iterator(triple.term_o.value, &(m_iterators[i]));
                    vars.insert(triple.term_o.value);
                }
                if(triple.p_is_variable()){
                    add_var_to_iterator(triple.term_p.value, &(m_iterators[i]));
                    vars.insert(triple.term_p.value);
                }
                if(triple.s_is_variable()){
                    m_bound_variables_set.insert(triple.term_s.value);
                    add_var_to_iterator(triple.term_s.value, &(m_iterators[i]));
                    vars.insert(triple.term_s.value);
                }
                ++i;
            }

            m_gao = gao_type(m_ptr_triple_patterns, &m_iterators, &m_var_to_iterators, m_ptr_ring);

            i = 0;
            std::unordered_map<var_type, size_type> table;
            for(const auto &triple : *m_ptr_triple_sim_patterns){
                if(triple.s_is_variable()){
                    auto it = m_bound_variables_set.find(triple.term_s.value);
                    if(it != m_bound_variables_set.end()){
                        m_type1_set.insert(i);
                    }else{
                        m_type2_set.insert(i);
                    }
                    auto it_t = table.find(triple.term_s.value);
                    if(it_t != table.end()){
                        ++it_t->second;
                    }else{
                        table.insert({triple.term_s.value, 1});
                    }
                    vars.insert(triple.term_s.value);
                }else{
                    m_type1_set.insert(i);
                }
                if(triple.o_is_variable()){
                    auto it_t = table.find(triple.term_o.value);
                    if(it_t != table.end()){
                        ++it_t->second;
                    }else{
                        table.insert({triple.term_o.value, 1});
                    }
                    vars.insert(triple.term_o.value);
                }
                ++i;
            }
            i = 0;
            m_weight_sim_patterns.resize(m_ptr_triple_sim_patterns->size());
            for(const auto &triple : *m_ptr_triple_sim_patterns){
                auto value = 0;
                if(triple.s_is_variable()){
                    auto v = table[triple.term_s.value];
                    if(v > value) value = v;
                    vars.insert(triple.term_s.value);
                }
                if(triple.o_is_variable()){
                    auto v = table[triple.term_o.value];
                    if(v > value) value = v;
                    vars.insert(triple.term_o.value);
                }
                m_weight_sim_patterns[i] = value;
                ++i;
            }
            m_var_size = vars.size();
        }

        //! Copy constructor
        ltj_algorithm_similarity_baseline(const ltj_algorithm_similarity_baseline &o) {
            copy(o);
        }

        //! Move constructor
        ltj_algorithm_similarity_baseline(ltj_algorithm_similarity_baseline &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_algorithm_similarity_baseline &operator=(const ltj_algorithm_similarity_baseline &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_algorithm_similarity_baseline &operator=(ltj_algorithm_similarity_baseline &&o) {
            if (this != &o) {
                m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                m_gao = std::move(o.m_gao);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_iterators = std::move(o.m_iterators);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_is_empty = o.m_is_empty;
                m_weight_sim_patterns = std::move(o.m_weight_sim_patterns);
                m_bound_variables_set = std::move(o.m_bound_variables_set);
                m_type1_set = std::move(o.m_type1_set);
                m_type2_set = std::move(o.m_type2_set);
                m_var_size = std::move(o.m_var_size);
            }
            return *this;
        }

        void swap(ltj_algorithm_similarity_baseline &o) {
            std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
            std::swap(m_gao, o.m_gao);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_iterators, o.m_iterators);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_weight_sim_patterns, o.m_weight_sim_patterns);
            std::swap(m_bound_variables_set, o.m_bound_variables_set);
            std::swap(m_type1_set, o.m_type1_set);
            std::swap(m_type2_set, o.m_type2_set);
            std::swap(m_var_size, o.m_var_size);
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
                //std::cout << "At: " << j << " var: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                bool ok;
                if(itrs.size() == 1 && itrs[0]->in_last_level()) {//Lonely variables
                    value_type c = itrs[0]->seek_last(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" << results.size() << ")" <<std::endl;
                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        assert(x_j < tuple.size());
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
                } else {
                    value_type c = seek(x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        assert(x_j < tuple.size());
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


        std::pair<size_type, bool> next_similarity_triple_pattern(){
            size_type next;
            if(m_type1_set.empty()){
                size_type weight = 0;
                for(const auto &id : m_type2_set){
                    if(m_weight_sim_patterns[id] > weight){
                        weight = m_weight_sim_patterns[id];
                        next = id;
                    }
                }
                m_type2_set.erase(next);
                return {next, false};
            }else{
                size_type weight = 0;
                for(const auto &id : m_type1_set){
                    if(m_weight_sim_patterns[id] > weight){
                        weight = m_weight_sim_patterns[id];
                        next = id;
                    }
                }
                m_type1_set.erase(next);
                return {next, true};
            }
        }

        void update_sets(size_type id){

            const auto &triple = m_ptr_triple_sim_patterns->at(id);
            if(triple.s_is_variable()){
                m_bound_variables_set.insert(triple.term_s.value);
            }
            if(triple.o_is_variable()){
                m_bound_variables_set.insert(triple.term_o.value);
            }
            std::vector<size_type> aux;
            for(const auto &p : m_type2_set){
                const auto &t = m_ptr_triple_sim_patterns->at(p);
                if(t.s_is_variable()){
                    if(m_bound_variables_set.find(t.term_s.value) != m_bound_variables_set.end()){
                        aux.push_back(p);
                    }
                }
            }
            for(const auto &p : aux){
                m_type1_set.insert(p);
                m_type2_set.erase(p);
            }

        }

        void solve_pattern(size_type id, bool is_type1, std::vector<tuple_type> &pre){
            std::vector<tuple_type> res;
            const auto &triple = m_ptr_triple_sim_patterns->at(id);
            if(is_type1){
                if(triple.o_is_variable()
                    && m_bound_variables_set.find(triple.term_o.value) == m_bound_variables_set.end()){

                    std::vector<value_type> vals;
                    //Expandir con objeto (directo) [sujeto fijado]
                    if (triple.s_is_variable()){
                        for(const auto& tuple : pre){
                            m_ptr_ring->knn_expand_fixed(triple.k_sim,  tuple[triple.term_s.value], vals);
                            for(const auto &v : vals){
                                auto aux_t = tuple;
                                aux_t[triple.term_o.value] = v;
                                res.push_back(aux_t);
                            }
                            vals.clear();
                        }
                    }else{
                        for(const auto& tuple : pre){
                            m_ptr_ring->knn_expand_fixed(triple.k_sim,  triple.term_s.value, vals);
                            for(const auto &v : vals){
                                auto aux_t = tuple;
                                aux_t[triple.term_o.value] = v;
                                res.push_back(aux_t);
                            }
                            vals.clear();
                        }
                    }
                }else{
                    //Chequear se existe  [ambos fijados]
                    if(triple.s_is_variable() && triple.o_is_variable()){
                        for(const auto& tuple : pre){
                            if(m_ptr_ring->knn_exists(tuple[triple.term_s.value], triple.k_sim, tuple[triple.term_o.value])){
                                res.emplace_back(tuple);
                            }
                        }
                    }else if (triple.s_is_variable() && !triple.o_is_variable()){
                        for(const auto& tuple : pre){
                            if(m_ptr_ring->knn_exists(tuple[triple.term_s.value], triple.k_sim, triple.term_o.value)){
                                res.emplace_back(tuple);
                            }
                        }
                    }else if (!triple.s_is_variable() && triple.o_is_variable()){
                        for(const auto& tuple : pre){
                            if(m_ptr_ring->knn_exists(triple.term_s.value, triple.k_sim, tuple[triple.term_o.value])){
                                res.emplace_back(tuple);
                            }
                        }
                    }else{
                        for(const auto& tuple : pre){
                            if(m_ptr_ring->knn_exists(triple.term_s.value, triple.k_sim, triple.term_o.value)){
                                res.emplace_back(tuple);
                            }
                        }
                    }
                }
            }else{
                if(!triple.o_is_variable() || (triple.o_is_variable()
                    && m_bound_variables_set.find(triple.term_o.value) != m_bound_variables_set.end())){

                    std::vector<value_type> vals;
                    //Expandir con sujeto (inverso) [objeto fijado]
                    if (triple.o_is_variable()){
                        for(const auto& tuple : pre){
                            m_ptr_ring->knn_inv_expand_fixed(triple.k_sim,  tuple[triple.term_o.value], vals);
                            for(const auto &v : vals){
                                auto aux_t = tuple;
                                aux_t[triple.term_s.value] = v;
                                res.push_back(aux_t);
                            }
                            vals.clear();
                        }
                    }else{
                        for(const auto& tuple : pre){
                            m_ptr_ring->knn_inv_expand_fixed(triple.k_sim,  triple.term_o.value, vals);
                            for(const auto &v : vals){
                                auto aux_t = tuple;
                                aux_t[triple.term_s.value] = v;
                                res.push_back(aux_t);
                            }
                            vals.clear();
                        }
                    }
                }else{
                    //Expandir con sujeto (inverso) [nada fijado]
                    std::vector<std::pair<value_type, value_type>> pairs;
                    m_ptr_ring->knn_inv_expand(triple.k_sim,  pairs);
                    for(const auto& tuple : pre){
                        for(const auto &v : pairs){
                            auto aux_t = tuple;
                            aux_t[triple.term_s.value] = v.first;
                            aux_t[triple.term_o.value] = v.second;
                            res.push_back(aux_t);
                        }
                    }
                }
            }
            pre = std::move(res);
        }

        void knn_filter( std::vector<tuple_type> &pre){

            while(!(m_type1_set.empty() && m_type2_set.empty())){
                auto next = next_similarity_triple_pattern();
                solve_pattern(next.first, next.second, pre);
                if(pre.empty()) return;
                update_sets(next.first);
            }
        }

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
            tuple_type t(m_var_size);
            search(0, t, res, start, limit_results, timeout_seconds);
            if(!m_iterators.empty() && res.empty()) return;
            knn_filter(res);
        };

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

        void print_results(std::vector<tuple_type> &res,
                           std::unordered_map<uint8_t, std::string> &ht,
                           const std::string &file_name){

           std::ofstream out(file_name);
            uint64_t i = 1;
            for(tuple_type &tuple :  res){
                out << "[" << i << "]:\t";
                uint64_t j = 0;
                for(auto& val : tuple){
                    out << "?" << ht[j] << "=" << val << " ";
                    ++j;
                }
                out << std::endl;
                ++i;
            }
            out.close();
        }


    };
}

#endif //RING_LTJ_ALGORITHM_HPP
