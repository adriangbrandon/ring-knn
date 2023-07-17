/***
BSD 2-Clause License

Copyright (c) 2018, Adrián
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


//
// Created by Adrián on 12/9/22.
//

#ifndef RING_KNN_GRAPH_CDS_HPP
#define RING_KNN_GRAPH_CDS_HPP


#include <configuration.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <wt_intersection_iterator.hpp>
#include <wt_intersection_helper.hpp>
#include <wt_range_iterator.hpp>
#include <wt_range_helper.hpp>

namespace ring_ltj {

    template<class wm_bit_vector_t = sdsl::bit_vector,
             class wm_rank_t = sdsl::rank_support_v<1>, class b_bit_vector_t = sdsl::bit_vector>
    class knn_graph_cds {

    public:
        typedef uint64_t size_type;
        typedef uint64_t value_type;
        typedef sdsl::wm_int<wm_bit_vector_t, wm_rank_t,
                sdsl::select_support_scan<1>, sdsl::select_support_scan<0>> wt_type;
        typedef b_bit_vector_t b_type;
        typedef wt_intersection_iterator<wt_type> intersection_iterator_type;
        typedef wt_intersection_helper<wt_type> intersection_helper_type;
        typedef wt_range_iterator<wt_type> range_iterator_type;
        typedef wt_range_helper<wt_type> range_helper_type;
        typedef typename b_bit_vector_t::select_1_type b_select_1_type;

    private:
        std::vector<wt_type> m_wts;
        b_type  m_b;
        b_select_1_type m_b_select;
        size_type m_max_k;
        size_type m_nodes;


        void copy(const knn_graph_cds &o) {
            m_wts = o.m_wts;
            m_b = o.m_b;
            m_b_select = o.m_b_select;
            m_b_select.set_vector(&m_b);
            m_max_k = o.m_max_k;
            m_nodes = o.m_nodes;
        }

        inline size_type p(const value_type x, const size_type k){
            return m_b_select((x-1)*m_max_k + k)+1 - (x-1)*m_max_k - k + 1;
        }

        inline range_type range_in_g(const value_type x, const size_type k){
            return range_type{(x-1)*m_max_k+1, (x-1)*m_max_k+k};
        }

        inline range_type range_in_inv_g(const value_type x, const size_type k){
            return range_type{p(x,1), p(x, k+1)-1};
        }

        struct sort_inverse {
            bool operator()(const knn_item_type &a, const knn_item_type &b) {
                if (a.k == b.k) {
                    return a.id < b.id;
                } else {
                    return a.k < b.k;
                }
            }
        };

    public:

        const size_type& max_k = m_max_k;
        const size_type& nodes = m_nodes;

        knn_graph_cds() = default;

        //The inverted list is sorted by the k value
        knn_graph_cds(const knn_graph_type &g, const size_type max_k_p){

            m_max_k = max_k_p;
            m_nodes = g.size();
            m_wts.resize(2);

            knn_graph_type inv_g(m_nodes);
            {
                sdsl::int_vector<> aux(m_nodes*m_max_k+1, 0);
                aux[0] = m_nodes-1;
                for(size_type i = 0; i < m_nodes; ++i){
                    for(size_type j = 0; j < g[i].size(); ++j){
                        auto& element = g[i][j];
                        aux[i*m_max_k + element.k]=element.id;
                        knn_item_type inv_item{i+1, element.k};
                        inv_g[element.id-1].push_back(inv_item);
                    }
                }
                sdsl::util::bit_compress(aux);
                construct_im(m_wts[0], aux);
            }

            for(size_type i = 0; i < m_nodes; ++i){
                sort(inv_g[i].begin(), inv_g[i].end(), sort_inverse());
            }

            {
                sdsl::int_vector<> aux(m_nodes*m_max_k+1, 0);
                aux[0] = m_nodes-1;
                m_b = b_type(2*m_max_k*m_nodes+1, 0); //
                size_type b_index = 0, a_i = 1;
                for(size_type i = 0; i < m_nodes; ++i) {
                    size_type prev_k = 0;
                    for(size_type j = 0; j < inv_g[i].size(); ++j) {
                        while(prev_k < inv_g[i][j].k){
                            m_b[b_index++] = 1;
                            ++prev_k;
                        }
                        m_b[b_index++] = 0;
                        aux[a_i++] = inv_g[i][j].id;
                        prev_k = inv_g[i][j].k;
                    }
                    while(prev_k < m_max_k){
                        m_b[b_index++] = 1;
                        ++prev_k;
                    }
                }
                m_b[b_index] = 1;
                sdsl::util::bit_compress(aux);
                construct_im(m_wts[1], aux);
            }
            sdsl::util::init_support(m_b_select, &m_b);
        }

        //! Copy constructor
        knn_graph_cds(const knn_graph_cds &o) {
            copy(o);
        }

        //! Move constructor
        knn_graph_cds(knn_graph_cds &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        knn_graph_cds &operator=(const knn_graph_cds &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        knn_graph_cds &operator=(knn_graph_cds &&o) {
            if (this != &o) {
                m_wts = std::move(o.m_wts);
                m_b = std::move(o.m_b);
                m_b_select = std::move(o.m_b_select);
                m_b_select.set_vector(&m_b);
                m_max_k = o.m_max_k;
                m_nodes = o.m_nodes;
            }
            return *this;
        }

        void swap(knn_graph_cds &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wts, o.m_wts);
            std::swap(m_b, o.m_b);
            sdsl::util::swap_support(m_b_select, o.m_b_select, &m_b, &o.m_b);
            std::swap(m_max_k, o.m_max_k);
            std::swap(m_nodes, o.m_nodes);
        }

        void print_structure(){
            std::cout << "Graph" << std::endl;
            std::cout << "===============" << std::endl;
            std::cout << "S: ";
            for(uint64_t i = 0; i < m_wts[0].size(); ++i){
                std::cout << m_wts[0][i] << ", ";
            }
            std::cout << std::endl;

            std::cout << "Inverse Graph" << std::endl;
            std::cout << "===============" << std::endl;
            std::cout << "B: ";
            for(uint64_t i = 0; i < m_b.size(); ++i){
                std::cout << (uint64_t) m_b[i] << ", ";
            }
            std::cout << std::endl;
            std::cout << "S': ";
            for(uint64_t i = 0; i < m_wts[1].size(); ++i){
                std::cout << m_wts[1][i] << ", ";
            }
            std::cout << std::endl;

        }

        inline void beg_intersection_iterator(const value_type x, const size_type k1, const size_type k2,
                                              intersection_iterator_type& it){
            if(x > m_nodes || k1 > m_max_k || k2 > m_max_k){
                it = intersection_iterator_type();
            }else{
                std::vector<range_type> ranges = {range_in_g(x, k1), range_in_inv_g(x, k2)};
                it = intersection_iterator_type(&m_wts, ranges);
            }
        }

        inline void beg_intersection_helper(const value_type x, const size_type k1, const size_type k2,
                                              intersection_helper_type& it){
            if(x > m_nodes || k1 > m_max_k || k2 > m_max_k){
                it = intersection_helper_type();
            }else{
                std::vector<range_type> ranges = {range_in_g(x, k1), range_in_inv_g(x, k2)};
                it = intersection_helper_type(&m_wts, ranges);
            }
        }

        inline void beg_range_iterator(const value_type x, const size_type k,
                                       bool subject, range_iterator_type& it){

            if(x > m_nodes || k > m_max_k){
                it = range_iterator_type();
            }else{
               if(subject){
                   range_type range = range_in_g(x, k);
                   it = range_iterator_type(&m_wts[0], range);
               }else{
                   range_type range = range_in_inv_g(x, k);
                   it = range_iterator_type(&m_wts[1], range);
               }
            }
        }

        inline void beg_range_helper(const value_type x, const size_type k,
                                       bool subject, range_helper_type& it){

            if(x > m_nodes || k > m_max_k){
                it = range_helper_type();
            }else{
                if(subject){
                    range_type range = range_in_g(x, k);
                    it = range_helper_type(&m_wts[0], range);
                }else{
                    range_type range = range_in_inv_g(x, k);
                    it = range_helper_type(&m_wts[1], range);
                }
            }
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::serialize_vector(m_wts, out, child, "wts");
            written_bytes += m_b.serialize(out, child, "b");
            written_bytes += m_b_select.serialize(out, child, "b_select");
            written_bytes += sdsl::write_member(m_max_k, out, child, "max_k");
            written_bytes += sdsl::write_member(m_nodes, out, child, "nodes");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }


        void load(std::istream &in) {
            m_wts.resize(2);
            sdsl::load_vector(m_wts, in);
            m_b.load(in);
            m_b_select.load(in);
            m_b_select.set_vector(&m_b);
            sdsl::read_member(m_max_k, in);
            sdsl::read_member(m_nodes, in);
        }



    };
}

#endif //RING_KNN_GRAPH_CDS_HPP
