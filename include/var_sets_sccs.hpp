//
// Created by Adrián on 24/1/23.
//

#ifndef RING_VAR_SETS_SCSS_HPP
#define RING_VAR_SETS_SCSS_HPP

#include <vector>
#include <unordered_set>

namespace ring_ltj {


    template<class var_t = uint8_t,
            class const_t = uint64_t>
    class var_sets_sccs {

    public:
        typedef var_t var_type;
        typedef const_t const_type;
        typedef uint64_t size_type;
        typedef std::unordered_set<var_type> set_type;
        typedef std::vector<set_type> set_vec_type;
        typedef std::vector<var_type> scc_type;
        struct info_var_type{
            //var_type name;
            //Linked info
            size_type weight = UINT64_MAX;
            std::unordered_set<var_type> linked;
            size_type sim_cnt = 0;
            std::vector<var_type> sim_tgts;
            bool is_bound = false;
        };


        typedef std::vector<info_var_type> info_var_vector_type;
        typedef std::vector<scc_type> info_scc_vector_type;
        typedef typename set_type::iterator set_iterator_type;


    private:
        info_var_vector_type    m_info_var_vec;
        info_scc_vector_type    m_info_scc_vec;
        size_type               m_scc_i;
        set_vec_type            m_sets;

        void copy(const var_sets_sccs &o) {
            m_info_var_vec = o.m_info_var_vec;
            m_info_scc_vec = o.m_info_scc_vec;
            m_scc_i = o.m_scc_i;
            m_sets = o.m_sets;
        }

    public:

        const info_var_vector_type& info = m_info_var_vec;
        const set_vec_type&         sets = m_sets;

        var_sets_sccs() = default;

        var_sets_sccs(const size_type n_vars){
            m_scc_i = 0;
            m_sets.resize(num_sets);
            m_info_var_vec.resize(n_vars);
        }

        //! Copy constructor
        var_sets_sccs(const var_sets_sccs &o) {
            copy(o);
        }

        //! Move constructor
        var_sets_sccs(var_sets_sccs &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        var_sets_sccs &operator=(const var_sets_sccs &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        var_sets_sccs &operator=(var_sets_sccs &&o) {
            if (this != &o) {
                m_info_var_vec = std::move(o.m_info_var_vec);
                m_info_scc_vec = std::move(o.m_info_scc_vec);
                m_scc_i = std::move(o.m_scc_i);
                m_sets = std::move(o.m_sets);
            }
            return *this;
        }

        void swap(var_sets_sccs &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_info_var_vec, o.m_info_var_vec);
            std::swap(m_info_scc_vec, o.m_info_scc_vec);
            std::swap(m_scc_i, o.m_scc_i);
            std::swap(m_sets, o.m_sets);
        }

        /**
         * Methods to update m_info_var_vec
         */


        //Is_bound
        inline void set_is_bound(const var_type var, bool v){
            m_info_var_vec[var].is_bound = v;
        }

        inline bool is_bound(const var_type var){
            return m_info_var_vec[var].is_bound;
        }

        inline bool is_lonely(const var_type var){
            return m_sets[set_enum_type::lonely].find(var) != m_sets[set_enum_type::lonely].end();
        }

        inline bool is_sim(const var_type var){
            return m_sets[set_enum_type::sim].find(var) != m_sets[set_enum_type::sim].end();
        }

        //Weight
        inline void set_weight(const var_type var, size_type w){
            m_info_var_vec[var].weight = w;
        }

        //Linked
        inline void add_linked(const var_type v1, const var_type v2){
            m_info_var_vec[v1].linked.insert(v2);
            m_info_var_vec[v2].linked.insert(v1);
        }

        //Sim cnt
        inline bool no_tgt(const var_type var){
            return m_info_var_vec[var].sim_cnt == 0;
        }

        inline void set_sim_cnt(const var_type var, size_type c){
            m_info_var_vec[var].sim_cnt = c;
        }

        //Sim cnt
        inline void inc_sim_cnt(const var_type var){
            ++m_info_var_vec[var].sim_cnt;
        }

        //Sim cnt
        inline void dec_sim_cnt(const var_type var){
            --m_info_var_vec[var].sim_cnt;
        }

        //Targets
        inline void add_tgt(const var_type src, const var_type tgt){
            m_info_var_vec[src].sim_tgts.emplace_back(tgt);
        }


        /**
         * Methods to manage the list
         */

        inline bool empty(const set_enum_type set){
            return m_sets[set].empty();
        }

        inline set_iterator_type insert(const var_type var, const set_enum_type set){
            auto p = m_sets[set].insert(var);
            return p.first;
        }

        inline void erase(const var_type var, const set_enum_type set){
            m_sets[set].erase(var);
        }

        inline set_iterator_type begin(const set_enum_type set){
            return m_sets[set].begin();
        }

        inline set_iterator_type end(const set_enum_type set){
            return m_sets[set].end();
        }

        void add_scc(std::vector<var_type> &scc){
            m_info_scc_vec.emplace_back(std::move(scc));
        }

        inline bool exist_scc(){
            return (m_scc_i < m_info_scc_vec.size());
        }

        void insert_next_scc(){
            for(const auto &v : m_info_scc_vec[m_scc_i]){
                m_sets[set_enum_type::ready].insert(v);
            }
            ++m_scc_i;
        }

        void remove_prev_scc(){
            if(m_scc_i > 0){
                --m_scc_i;
                for(const auto &v : m_info_scc_vec[m_scc_i]){
                    m_sets[set_enum_type::ready].erase(v);
                }
            }
        }

        inline size_type size(){
            return m_info_var_vec.size();
        }

        void print(){
            std::cout << "***  Variables   ***" << std::endl;
            size_type weight = UINT64_MAX;
            std::unordered_set<var_type> linked;
            //Sym links info
            size_type sim_cnt = 0;
            std::vector<var_type> sim_tgts;
            bool is_bound = false;

            for(size_type i = 0; i < m_info_var_vec.size(); ++i){
                const auto &a = m_info_var_vec[i];
                std::cout << "Variable " << i << ": { weight=" << a.weight << " |linked|=" << a.linked.size();
                std::cout << " sim_cnt=" << a.sim_cnt << " |sim_tgts|=" << a.sim_tgts.size() << " is_bound=";
                std::cout << a.is_bound << "}" << std::endl;
            }
            std::cout << "***    Sets     ***" << std::endl;
            std::cout << "Mand: {";
            for(const auto &i : m_sets[set_enum_type::mand]){
                std::cout << (uint64_t) i << " -> ";
            }
            std::cout << "}" << std::endl;
            std::cout << "Ready: {";
            for(const auto &i : m_sets[set_enum_type::ready]){
                std::cout << (uint64_t) i << " -> ";
            }
            std::cout << "}" << std::endl;
            std::cout << "Sim: {";
            for(const auto &i : m_sets[set_enum_type::sim]){
                std::cout << (uint64_t) i << " -> ";
            }
            std::cout << "}" << std::endl;
            std::cout << "Lonely: {";
            for(const auto &i : m_sets[set_enum_type::lonely]){
                std::cout << (uint64_t) i << " -> ";
            }
            std::cout << "}" << std::endl;
            std::cout << "***  SCCs   ***" << std::endl;
            std::cout << "[ ";
            for(const auto &scc : m_info_scc_vec){
                std::cout << "{";
                for(const auto &v : scc){
                    std::cout << (size_type) v << ", ";
                }
                std::cout << "}, ";
            }
            std::cout << " ] i=" << m_scc_i << std::endl;
            std::cout << std::endl;


        }
    };

}

#endif //RING_VAR_SETS_HPP
