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
// Created by Adrián on 17/5/22.
//

#ifndef RING_RPQ_WT_INTERSECTION_HPP
#define RING_RPQ_WT_INTERSECTION_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {

    template<class t_wt>
    std::vector<typename t_wt::value_type>
    intersect_nofreq(const t_wt& wt, const std::vector<range_type>& ranges, typename t_wt::size_type t=0)
    {
        using std::get;
        using size_type      = typename t_wt::size_type;
        using value_type     = typename t_wt::value_type;
        using node_type      = typename t_wt::node_type;
        using pnvr_type      = std::pair<node_type, range_vec_type>;
        typedef std::stack<pnvr_type> stack_type;

        static_assert(has_expand<t_wt, std::array<node_type,2>(const node_type&)>::value,
                "intersect requires t_wt to have expand(const node_type&)");

        std::vector<size_type> res;

        auto push_node = [&wt,&t](stack_type& s, node_type& child,
                                  range_vec_type& child_range) {
            auto end = std::remove_if(child_range.begin(), child_range.end(),
                                      [&](const range_type& x) { return empty(x);});
            if (end > child_range.begin() + t - 1) {
                s.emplace(pnvr_type(child, range_vec_type(child_range.begin(),
                                                          end)));
            }
        };

        if (ranges.empty())
            return res;

        t = (t==0) ? ranges.size() : t;

        std::stack<pnvr_type> stack;
        stack.emplace(pnvr_type(wt.root(), ranges));

        while (!stack.empty()) {
            pnvr_type x = stack.top(); stack.pop();

            if (wt.is_leaf(x.first)) {
                const auto& iv = x.second;
                if (t <= iv.size()) {
                    res.emplace_back(wt.sym(x.first));
                }
            } else {
                auto child        = wt.expand(x.first);
                auto child_ranges = wt.expand(x.first, x.second);

                push_node(stack, get<1>(child), get<1>(child_ranges));
                push_node(stack, get<0>(child), get<0>(child_ranges));
            }
        }
        return res;
    }


    template<class t_wt>
    std::vector<typename t_wt::value_type>
    intersect_wts(const std::vector<t_wt>& wts,
                  const std::vector<range_type>& ranges)
    {
        using std::get;
        using size_type      = typename t_wt::size_type;
        using value_type     = typename t_wt::value_type;
        using node_type      = typename t_wt::node_type;
        using node_vec_type  = std::vector<node_type>;
        using pnvr_type      = std::pair<node_vec_type, range_vec_type>;
        typedef std::stack<pnvr_type> stack_type;

        static_assert(has_expand<t_wt, std::array<node_type,2>(const node_type&)>::value,
                      "intersect requires t_wt to have expand(const node_type&)");

        std::vector<value_type> res;

        if (ranges.empty())
            return res;

        size_type t =  ranges.size();


        node_vec_type nodes;
        for(size_type i = 0; i < t; ++i){
            nodes.emplace_back(wts[i]->root());
        }


        std::stack<pnvr_type> stack;
        stack.emplace(pnvr_type(nodes, ranges));

        while (!stack.empty()) {
            const pnvr_type& x = stack.top();
            if (wts[0].is_leaf(x.first[0])) {
                res.emplace_back(wts[0].sym(x.first[0]));
                stack.pop();
            }else{
                node_vec_type left_nodes, right_nodes;
                range_vec_type left_ranges, right_ranges;
                std::array<range_type, 2> child_ranges;
                size_type rnk;
                bool stop = false;
                for(size_type i = 0; i < t; ++i){
                    auto child =  wts[i].my_expand(x.first[i], x.second[i],
                                                    child_ranges[0], child_ranges[1], rnk);

                    if(left_nodes.size() == i && !empty(child_ranges[0])){
                        left_nodes.emplace_back(child[0]);
                        left_ranges.emplace_back(child_ranges[0]);
                    }

                    if(right_nodes.size() == i && !empty(child_ranges[1])){
                        right_nodes.emplace_back(child[1]);
                        right_ranges.emplace_back(child_ranges[1]);
                    }

                    if (right_nodes.size() < i+1 && left_nodes.size() < i+1){
                        break;
                    }
                }
                stack.pop();
                if(right_nodes.size() == t){
                    stack.emplace(pnvr_type(std::move(right_nodes), std::move(right_ranges)));
                }

                if(left_nodes.size() == t){
                    stack.emplace(pnvr_type(std::move(left_nodes), std::move(left_ranges)));
                }
            }
        }
        return res;
    }

    template<class t_wt, class range_type = typename sdsl::range_type>
    std::vector<typename t_wt::value_type>
    //intersect_iter(const std::vector<std::pair<t_wt, sdsl::range_vec_type>> &wt_ranges_v)
    intersect_iter(const std::vector<t_wt*>& p_wts, const std::vector<range_type>& p_ranges)
    {
        using std::get;
        using size_type      = typename t_wt::size_type;
        using value_type     = typename t_wt::value_type;
        using node_type      = typename t_wt::node_type;
        typedef struct {
            const t_wt& wt;
            node_type node;
            range_type ranges;
        } tuple_type;
        using stack_vector_type = std::vector<tuple_type>;
        typedef std::stack<stack_vector_type> stack_type;

        std::vector<value_type> res;
        stack_vector_type vec;
        stack_type stack;

        for(size_type i=0; i < p_wts.size(); i++){
            const t_wt& wt = *p_wts[i];
            //Can't be const & cause both node and ranges can get invalid / deleted during execution.
            node_type node = wt.root();
            range_type ranges = p_ranges[i];
            //tuple_type t = {wt, node, ranges};
            vec.emplace_back(tuple_type{wt, node, ranges}); //Adrian: Ojo con el WT de aqui, no lo copia pero
            //como ahora tienes el p_wts no necesitas el wt en el vec
            // Vamos quitar todo lo que podamos :)
        }
        if(p_wts.size() > 0){  //Adrian: Mejor comprueba al inicio que el np_wts.size() == 0. En ese caso devuelves un vector vacío
            stack.emplace(vec);
        }

        while (!stack.empty()) {
            bool symbol_reported = false;
            const stack_vector_type& wt_ranges_level_v = stack.top();
            bool empty_left_range = false, empty_right_range = false;
            stack_vector_type left_children_v;
            stack_vector_type right_children_v;
            for(const tuple_type& data : wt_ranges_level_v){
                const t_wt& wt = data.wt;
                const node_type& node = data.node;
                const range_type& ranges = data.ranges;
                if (wt.is_leaf(node)) { //Adrian: esto lo pondría fuera del for. Compruebas para el primer WT si el nodo es leaf
                    // si no lo es iteras en el for
                    res.emplace_back(wt.sym(node));
                    symbol_reported = true;
                    break;//No need to continue with the other tuples.
                } else {


                    const auto& children = wt.expand(node);
                    const auto& children_ranges = wt.expand(node, ranges);
                    if(!empty_left_range){ //Adrian: Comprobar que left_children_v.size() < wts.size() && sdsl::empty(std::get<0>(children_ranges)[0]).
                        // En ese caso se añade al vector. Lo mismo para right_children y te ahorras las variables
                        if(sdsl::empty(std::get<0>(children_ranges)[0])){
                            empty_left_range = true;
                        }
                        left_children_v.emplace_back(tuple_type{wt, get<0>(children), get<0>(children_ranges)});
                    }
                    if(!empty_right_range){
                        if(sdsl::empty(std::get<1>(children_ranges)[0])){
                            empty_right_range = true;
                        }
                        right_children_v.emplace_back(tuple_type{wt, get<1>(children), get<1>(children_ranges)});
                    }

                    if(empty_left_range && empty_right_range){ //Adrian: OJO: Lo moví aquí para evitar otra vuelta en el for.
                        //Creo que estas variables las puedes ahorrar haciendo lo que pongo arriba
                        // y comprobando que left_children_v.size() < iteracion+1 && right_children_v.size() < iteracion+1
                        break;
                    }
                }
            }
            stack.pop();
            if(!symbol_reported && !empty_right_range){ //Adrian: Comprobar que left_children_v.size() == wts.size()
                stack.emplace(right_children_v);//This operation takes a ton of time.
            }
            if(!symbol_reported && !empty_left_range){ //Adrian: Comprobar que right_children_v.size() == wts.size()
                stack.emplace(left_children_v);//This operation takes a ton of time.
            }
        }
        //std::cout << "Intersection size: " << res.size() << std::endl;
        return res;
    }



}

#endif //RING_RPQ_WT_INTERSECTION_HPP
