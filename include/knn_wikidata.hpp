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
// Created by Adrián on 13/12/22.
//

#ifndef RING_ENT_DIS_HPP
#define RING_ENT_DIS_HPP


#include <unordered_map>
#include <string>
#include <vector>

typedef struct {
    uint32_t e1;
    uint32_t e2;
    double_t d;
} data_type;


bool sort_sec(const std::pair<uint32_t, uint32_t> &a,
          const std::pair<uint32_t, uint32_t> &b)
{
    return (a.second < b.second);
}


void get_data(const std::string &line, std::vector<std::vector<uint32_t>> &knn_sim,
                   std::unordered_map<uint32_t, uint32_t> &entities, uint32_t &id, uint32_t k){

    auto i1 = line.find('\'')+1;
    auto j1 = line.find('#');
    auto i2 = j1+2;
    auto j2 = line.find('\'', i2);
    auto i3 = j1 + 3;
    auto j3 = line.find(')', i3);
    auto e1_s = line.substr(i1, j1-i1+1);
    auto e2_s = line.substr(i2, j2-i2+1);
    auto d_s = line.substr(i3, j3-i3+1);
    data_type data;
    uint32_t id1, id2;
    data.e1 = std::stoi(e1_s);
    auto it1 =  entities.find(data.e1);
    if(it1 == entities.end()) {
        entities.insert({data.e1, ++id});
        id1 = id;
        knn_sim.emplace_back();
    }else{
        id1 = it1->second;
    }
    data.e2 = std::stoi(e2_s);
    auto it2 = entities.find(data.e2);
    if(it2 == entities.end()){
        entities.insert({data.e2, ++id});
        id2 = id;
        knn_sim.emplace_back();
    }else{
        id2 = it2->second;
    }
    if(knn_sim[id1-1].size() < k){
        knn_sim[id1-1].emplace_back(id2);
    }
    if(knn_sim[id2-1].size() < k){
        knn_sim[id2-1].emplace_back(id1);
    }
}

void map_to_file(const std::string &file_name, std::unordered_map<uint32_t, uint32_t> &map, bool append = false){

    std::vector<std::pair<uint32_t , uint32_t>> pairs(map.begin(), map.end());
    map.clear();
    std::sort(pairs.begin(), pairs.end(), sort_sec);

    std::ofstream out;
    if(append){
        out.open(file_name, std::ios_base::app);
    }else{
        out.open(file_name);
    }
    for(const auto &pair : pairs){
        out << pair.second << " " << pair.first << std::endl;
    }
    out.close();
}


void knn_graph_wikidata(const std::string &file_name, const std::vector<std::vector<uint32_t >> &data){

    std::ofstream out(file_name);
    for(uint64_t i = 0; i < data.size(); ++i){
        for(uint64_t j = 0; j < data[i].size(); ++j){
            out << data[i][j];
            if(j < data[i].size()-1) out << " ";
        }
        out << std::endl;
    }
}








#endif //RING_ENT_DIS_HPP
