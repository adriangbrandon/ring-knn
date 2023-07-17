/*
 * configuration.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
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

#ifndef CONFIG_BWT
#define CONFIG_BWT

#include <set>
#include <map>
#include <tuple>
#include <vector>

#include <sdsl/suffix_arrays.hpp>
#include <sdsl/vectors.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/wavelet_trees.hpp>
#include <sdsl/wt_algorithm.hpp>

using namespace sdsl;
using namespace std;

typedef std::tuple<uint32_t, uint32_t, uint32_t> spo_triple;
typedef struct {
    uint64_t id;
    uint64_t k;
} knn_item_type;
typedef std::vector<std::vector<knn_item_type>> knn_graph_type;
enum descriptor_type {s, p, o, sim};
// Hash function
struct hash_pair{
    template<class Int>
    size_t operator()(const pair<Int ,Int> &x) const
    {
        return x.first ^ x.second;
    }
} ;

namespace ring_ltj{
    typedef enum {mand=0, ready=1, sim=2, lonely=3} set_enum_type;
    constexpr static uint64_t num_sets = 4;
}


//typedef sdsl::wm_int<bit_vector, rank_support_v<>, select_support_scan<>, select_support_scan<>> bwt_type;
/*typedef sdsl::wm_int<bit_vector> bwt_type;


typedef sdsl::bit_vector C_type;
typedef sdsl::rank_support_v<> C_rank_type;
typedef sdsl::select_support_mcl<> C_select_type;
typedef sdsl::select_support_mcl<0> C_select0_type;*/

#endif
