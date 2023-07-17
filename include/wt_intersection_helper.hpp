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
// Created by Adrián on 22/9/22.
//

#ifndef RING_RPQ_WT_INTERSECTION_HELPER_HPP
#define RING_RPQ_WT_INTERSECTION_HELPER_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_intersection_helper {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef std::vector<range_type> range_vec_type;
        typedef std::vector<node_type> node_vec_type;
        typedef std::pair<node_vec_type, range_vec_type> pnvr_type;
        typedef std::stack<pnvr_type> stack_type;
        typedef struct {
            value_type value;
            std::vector<size_type> positions;
        } val_pos_type;

    private:
        const std::vector<wt_type>* m_ptr_wts;
        std::vector<range_type>  m_ranges;
        size_type m_size = 0;
        size_type m_max_level;
        node_vec_type m_nodes;

        inline value_type c_sym(value_type c, size_type level){
                return (c >> (m_max_level - level));
        }

        void copy(const wt_intersection_helper &o) {
            m_nodes = o.m_nodes;
            m_ptr_wts = o.m_ptr_wts;
            m_ranges = o.m_ranges;
            m_size = o.m_size;
            m_max_level = o.m_max_level;
        }

    public:

        const  std::vector<range_type>& ranges = m_ranges;

        wt_intersection_helper() = default;

        wt_intersection_helper(std::vector<wt_type>* ptr_wts, std::vector<range_type>& r){
            m_ptr_wts = ptr_wts;
            m_ranges = std::move(r);
            m_size = m_ranges.size();
            m_max_level = m_ptr_wts->at(0).max_level;
            for(size_type i = 0; i < m_size; ++i){
                m_nodes.emplace_back(m_ptr_wts->at(i).root());
            }
            //m_stack.emplace(pnvr_type(std::move(nodes), m_ranges));
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){
            stack_type stack;
            stack.emplace(pnvr_type(m_nodes, m_ranges));
            while (!stack.empty()) {
                const pnvr_type &x = stack.top();
                if (m_ptr_wts->at(0).is_leaf(x.first[0])) {
                   auto r = value_type(x.first[0].sym);
                   stack.pop();
                   return r;
                }else{
                    node_vec_type left_nodes, right_nodes;
                    range_vec_type left_ranges, right_ranges;
                    std::array<range_type, 2> child_ranges;
                    size_type rnk;
                    for(size_type i = 0; i < m_size; ++i){
                        auto child =  m_ptr_wts->at(i).my_expand(x.first[i], x.second[i],
                                                       child_ranges[0], child_ranges[1], rnk);

                        if(left_nodes.size() == i && !empty(child_ranges[0])){
                            left_nodes.emplace_back(std::move(child[0]));
                            left_ranges.emplace_back(child_ranges[0]);
                        }

                        if(right_nodes.size() == i && !empty(child_ranges[1])){
                            right_nodes.emplace_back(std::move(child[1]));
                            right_ranges.emplace_back(child_ranges[1]);
                        }

                        if(right_nodes.size() < i+1 && left_nodes.size() < i+1){
                            break;
                        }
                    }
                    stack.pop();
                    if(right_nodes.size() == m_size){
                        stack.emplace(right_nodes, right_ranges);
                    }
                    if(left_nodes.size() == m_size){
                        stack.emplace(left_nodes, left_ranges);
                    }
                }
            }
            return 0; //No intersection
        }


        /***
         * Next value of an intersection between WTs on the same alphabet greater than
         * a given value c.
         * @param A given value c.
         * @pre In each iteration c has to be greater or equal than the previous c.
         * @return
         */
        value_type next(value_type c){
            stack_type stack;
            stack.emplace(pnvr_type(m_nodes, m_ranges));
            while (!stack.empty()) {
                const pnvr_type &x = stack.top();
                if (m_ptr_wts->at(0).is_leaf(x.first[0])) {
                    auto r = value_type(x.first[0].sym);
                    stack.pop();
                    return r;
                }else{
                    node_vec_type left_nodes, right_nodes;
                    range_vec_type left_ranges, right_ranges;
                    std::array<range_type, 2> child_ranges;
                    size_type rnk;
                    bool stop = false;
                    auto c_sym_l = c_sym(c, x.first[0].level + 1); //+1 because we check next level nodes
                    for(size_type i = 0; !stop && i < m_size; ++i){
                        auto child =  m_ptr_wts->at(i).my_expand(x.first[i], x.second[i],
                                                                 child_ranges[0], child_ranges[1], rnk);

                        if(!empty(child_ranges[0]) && child[0].sym >= c_sym_l){
                            left_nodes.emplace_back(child[0]);
                            left_ranges.emplace_back(child_ranges[0]);
                        }

                        if(!empty(child_ranges[1]) && child[1].sym >= c_sym_l){
                            right_nodes.emplace_back(child[1]);
                            right_ranges.emplace_back(child_ranges[1]);
                        }

                        stop = !(right_nodes.size() == i+1 || left_nodes.size() == i+1);
                    }
                    stack.pop();
                    if(right_nodes.size() == m_size){
                        stack.emplace(pnvr_type(right_nodes, right_ranges));
                    }
                    if(left_nodes.size() == m_size){
                        stack.emplace(pnvr_type(left_nodes, left_ranges));
                    }
                }
            }
            return 0; //No intersection
        }


        size_type distinct() const {
            if(m_size == 0) return 0;
            return std::min(sdsl::size(m_ranges[0]), sdsl::size(m_ranges[1]));
        }

        bool is_empty() const {
            return m_size == 0;
        }

        //! Copy constructor
        wt_intersection_helper(const wt_intersection_helper &o) {
            copy(o);
        }

        //! Move constructor
        wt_intersection_helper(wt_intersection_helper &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_intersection_helper &operator=(const wt_intersection_helper &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_intersection_helper &operator=(wt_intersection_helper &&o) {
            if (this != &o) {
                m_nodes = std::move(o.m_nodes);
                m_ptr_wts = std::move(o.m_ptr_wts);
                m_ranges = std::move(o.m_ranges);
                m_size = o.m_size;
                m_max_level = o.m_max_level;
            }
            return *this;
        }

        void swap(wt_intersection_helper &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_nodes, o.m_nodes);
            std::swap(m_ptr_wts, o.m_ptr_wts);
            std::swap(m_ranges, o.m_ranges);
            std::swap(m_size, o.m_size);
            std::swap(m_max_level, o.m_max_level);
        }
    };

}

#endif //RING_RPQ_WT_INTERSECTION_HPP
