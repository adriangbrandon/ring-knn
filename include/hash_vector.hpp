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
// Created by Adrián on 1/11/22.
//

#ifndef RING_KARP_RABIN_HPP
#define RING_KARP_RABIN_HPP

#include <bithacks.hpp>

namespace hash {


        typedef __uint128_t uint128_t;
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef uint64_t hash_type;
        static constexpr hash_type PRIME = 3355443229;
        static constexpr hash_type MERSENNE_PRIME_POW = 61;
        static constexpr hash_type MERSENNE_PRIME = (1ULL << 61) - 1;


        struct hash_pair {
            template <class T1, class T2>
            size_t operator()(const std::pair<T1, T2>& p) const
            {
                auto hash1 = std::hash<T1>{}(p.first);
                auto hash2 = std::hash<T2>{}(p.second);

                if (hash1 != hash2) {
                    return hash1 ^ hash2;
                }

                // If hash1 == hash2, their XOR is zero.
                return hash1;
            }
        };

        struct hash_vector {
            template<class V>
            hash_type operator()(const std::vector<V> &v) const {
                uint128_t hash = 0;
                for (const auto &a : v) {
                    hash = (hash * PRIME) + a;
                    hash = utils::bithacks::mersenne_mod(hash, MERSENNE_PRIME, MERSENNE_PRIME_POW);
                }
                return (hash_type) hash;
            }
        };

        struct equal_vector {
            template<class V>
            bool operator()(const std::vector<V> &v1, const std::vector<V> &v2) const {
                if (v1.size() != v2.size()) return false;
                for (size_type i = 0; i < v1.size(); ++i) {
                    if (v1[i] != v2[i]) return false;
                }
                return true;
            }
        };
}

#endif //RING_KARP_RABIN_HPP
