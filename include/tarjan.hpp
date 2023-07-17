//
// Created by Adrián on 27/6/23.
//

#ifndef RING_TARJAN_HPP
#define RING_TARJAN_HPP

#include <stdint.h>
#include <vector>
#include <stack>
#include <iostream>
#include <unordered_map>

namespace ring_ltj {

    template<class var_t = uint8_t>
    class tarjan {
    public:
        typedef uint64_t size_type;
        typedef var_t var_type;
        typedef int value_type;
        typedef std::vector<var_type> scc_type;
        typedef std::vector<scc_type> dag_type;
        typedef std::vector<std::vector<value_type>> graph_type;
        typedef std::vector<var_type> map_type;
        typedef std::pair<size_type, size_type> edge_type;

    private:
        value_type m_t = 0;
        std::vector<value_type> m_ids;
        std::vector<value_type> m_low;
        std::vector<bool> m_in_stack;
        std::stack<value_type> m_st;
        const graph_type *m_ptr_graph;
        const map_type* m_ptr_map;

        void findSCC(value_type u, dag_type &dag){
            m_ids[u] = m_low[u] = m_t;
            ++m_t;
            m_st.push(u);
            m_in_stack[u] = true;

            for(const auto &v : m_ptr_graph->at(u)){
                if(m_ids[v] == -1){
                    findSCC(v, dag);
                    m_low[u] = std::min(m_low[u], m_low[v]); //Tree edge (propagate low value from the tgt to src)
                }else if(m_in_stack[v]){
                    m_low[u] = std::min(m_low[u], m_ids[v]);  //Back edge (take the value from the tgt to the src)
                }
            }

            int w;
            if(m_ids[u] == m_low[u]){
                scc_type scc;
                while(m_st.top() != u){
                    w = m_st.top();
                    m_in_stack[w] = false;
                    m_st.pop();
                    scc.push_back(m_ptr_map->at(w));
                }
                w = m_st.top();
                m_in_stack[w] = false;
                m_st.pop();
                scc.push_back(m_ptr_map->at(w));
                dag.push_back(scc);
            }
        }

        void copy(const tarjan &o) {
            m_t = o.m_t;
            m_ids = o.m_ids;
            m_low = o.m_low;
            m_in_stack = o.m_in_stack;
            m_st = o.m_st;
            m_ptr_map = o.m_ptr_map;
            m_ptr_graph = o.m_ptr_graph;
        }

    public:

        tarjan(const graph_type* g, const map_type* m) {
            m_ptr_graph = g;
            m_ptr_map = m;
            m_ids = std::vector<value_type>(g->size(), -1);
            m_low = std::vector<value_type>(g->size(), -1);
            m_in_stack = std::vector<bool>(g->size(), false);
        }

        void run(std::vector<dag_type> &dags, graph_type &edges){
            for(int u = 0; u < m_ptr_graph->size(); ++u){
                if(m_ids[u] == -1){
                    dag_type dag;
                    findSCC(u, dag);
                    std::reverse(dag.begin(), dag.end());
                    dags.push_back(dag);
                }
            }
            std::unordered_map<size_type, size_type> map;
            int i = 0;
            for(const auto& dag : dags){
                for(const auto &scc : dag){
                    map.insert({m_low[scc[0]], i});
                    ++i;
                }
            }

            std::unordered_set<edge_type, hash_pair> set_edges;
            edges.resize(i);
            for(int u = 0; u < m_ptr_graph->size(); ++u){
                for(int n : m_ptr_graph->at(u)){
                    if(m_low[u] != m_low[n]){
                        auto b = set_edges.insert({map[m_low[u]], map[m_low[n]]});
                        if(b.second){
                            edges[map[m_low[u]]].emplace_back(map[m_low[n]]);
                        }
                    }
                }
            }
        }

        void run(std::vector<dag_type> &dags){
            for(int u = 0; u < m_ptr_graph->size(); ++u){
                if(m_ids[u] == -1){
                    dag_type dag;
                    findSCC(u, dag);
                    std::reverse(dag.begin(), dag.end());
                    dags.push_back(dag);
                }
            }
        }

        //! Copy constructor
        tarjan(const tarjan &o) {
            copy(o);
        }

        //! Move constructor
        tarjan(tarjan &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        tarjan &operator=(const tarjan &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        tarjan &operator=(tarjan &&o) {
            if (this != &o) {
                m_t = o.m_t;
                m_ids = std::move(o.m_ids);
                m_low = std::move(o.m_low);
                m_in_stack = std::move(o.m_in_stack);
                m_st = std::move(o.m_st);
                m_ptr_map = o.m_ptr_map;
                m_ptr_graph = o.m_ptr_graph;
            }
            return *this;
        }

        void swap(tarjan &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_t, o.m_t);
            std::swap(m_ids, o.m_ids);
            std::swap(m_low, o.m_low);
            std::swap(m_in_stack, o.m_in_stack);
            std::swap(m_st, o.m_st);
            std::swap(m_ptr_map, o.m_ptr_map);
            std::swap(m_ptr_graph, o.m_ptr_graph);
        }
    };

}

#endif //RING_TARJAN_HPP
