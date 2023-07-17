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

#ifndef RING_RPQ_WT_ITERATOR_HPP
#define RING_RPQ_WT_ITERATOR_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_range_iterator {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef std::pair<node_type, range_type> pnvr_type;
        typedef std::stack<pnvr_type> stack_type;

    private:
        const wt_type* m_wt_ptr;
        stack_type m_stack;
        size_type m_size = 0;
        size_type m_max_level;
        range_type  m_range;

        inline value_type c_sym(value_type c, size_type level){
            return (c >> (m_max_level - level));
        }

        void copy(const wt_range_iterator &o) {
            m_wt_ptr = o.m_wt_ptr;
            m_stack = o.m_stack;
            m_size = o.m_size;
            m_max_level = o.m_max_level;
            m_range = o.m_range;
        }

    public:

        wt_range_iterator() = default;

        wt_range_iterator(const wt_type* wt_ptr, const range_type &range){
            m_wt_ptr = wt_ptr;
            m_range = range;
            pnvr_type element{m_wt_ptr->root(), m_range};
            m_stack.emplace(element);
            m_size = 1;
            m_max_level = m_wt_ptr->max_level;
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){

            while (!m_stack.empty()) {
                const pnvr_type &x = m_stack.top();
                if (m_wt_ptr->is_leaf(x.first)) {
                   auto r = value_type(x.first.sym);
                   m_stack.pop();
                   return r;
                }else{
                    size_type rnk;
                    array<range_type, 2> child_ranges;
                    auto child =  m_wt_ptr->my_expand(x.first, x.second,
                                                          child_ranges[0], child_ranges[1], rnk);
                    m_stack.pop();
                    if(!empty(child_ranges[1])){
                        m_stack.emplace(std::move(child[1]), child_ranges[1]);
                    }
                    if(!empty(child_ranges[0])){
                        m_stack.emplace(std::move(child[0]), child_ranges[0]);
                    }
                }
            }
            return 0; //No more values
        }

        value_type next(value_type c){
            while (!m_stack.empty()) {
                const pnvr_type &x = m_stack.top();
                if (m_wt_ptr->is_leaf(x.first)) {
                    auto r = value_type(x.first.sym);
                    m_stack.pop();
                    return r;
                }else{
                    std::array<range_type, 2> child_ranges;
                    size_type rnk;
                    auto c_sym_l = c_sym(c, x.first.level + 1); //+1 because we check next level nodes
                    auto child =  m_wt_ptr->my_expand(x.first, x.second,
                                                             child_ranges[0], child_ranges[1], rnk);
                    m_stack.pop();
                    if(!empty(child_ranges[1]) && child[1].sym >= c_sym_l){
                        m_stack.emplace(std::move(child[1]), child_ranges[1]);
                    }

                    if(!empty(child_ranges[0]) && child[0].sym >= c_sym_l){
                        m_stack.emplace(std::move(child[0]), child_ranges[0]);
                    }
                }
            }
            return 0; //No more values
        }

        bool is_empty() const {
            return m_size == 0;
        }

        size_type distinct() const {
            if(m_size == 0) return 0;
            return m_range.size();
        }

        //! Copy constructor
        wt_range_iterator(const wt_range_iterator &o) {
            copy(o);
        }

        //! Move constructor
        wt_range_iterator(wt_range_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_range_iterator &operator=(const wt_range_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_range_iterator &operator=(wt_range_iterator &&o) {
            if (this != &o) {
                m_wt_ptr = std::move(o.m_wt_ptr);
                m_stack = std::move(o.m_stack);
                m_size = o.m_size;
                m_max_level = o.m_max_level;
                m_range = o.m_range;
            }
            return *this;
        }

        void swap(wt_range_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt_ptr, o.m_wt_ptr);
            std::swap(m_stack, o.m_stack);
            std::swap(m_size, o.m_size);
            std::swap(m_max_level, o.m_max_level);
            std::swap(m_range, o.m_range);
        }
    };

}

#endif //RING_RPQ_WT_ITERATOR_HPP
