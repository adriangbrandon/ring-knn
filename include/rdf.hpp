
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
// Created by Adrián on 5/12/22.
//

#ifndef RING_RDF_HPP
#define RING_RDF_HPP

#include <configuration.hpp>
#include <unordered_map>
#include <algorithm>
#include <file.hpp>
#include <triple_pattern.hpp>


typedef struct {
    uint32_t id;
    std::vector<uint32_t> adjs;
} id_list_t;


bool sortbysec(const std::pair<std::string, uint32_t> &a,
               const std::pair<std::string, uint32_t> &b)
{
    return (a.second < b.second);
}


bool sortbyid(const id_list_t &a, const id_list_t &b)
{
    return (a.id < b.id);
}

std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(' ');
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

std::vector<std::string> tokenizer(const std::string &input, const char &delimiter){
    std::stringstream stream(input);
    std::string token;
    std::vector<std::string> res;
    while(getline(stream, token, delimiter)){
        res.emplace_back(trim(token));
    }
    return res;
}


bool sortby_triple(const spo_triple &a, const spo_triple &b)
{
    if(get<0>(a) == get<0>(b)){
        if(get<1>(a) == get<1>(b)){
            return get<2>(a) < get<2>(b);
        }
        return get<1>(a) < get<1>(b);
    }
    return get<0>(a) < get<0>(b);
}

uint64_t file_to_str2id(const std::string &file_name, std::unordered_map<std::string, uint32_t> &dict){
    uint64_t size = utils::file::file_size(file_name);
    std::ifstream in(file_name);
    std::string url, str_id;
    uint64_t id;
    uint64_t max_id = 0;
    uint64_t cnt = 0, total_bytes = 0;
    while(true){
        std::string line;
        std::getline(in, line);
        if(in.eof()) break;
        total_bytes += line.size();
        line = trim(line);
        auto p = line.find(' ');
        str_id = line.substr(0, p);
        id = std::stoull(str_id);
        url = line.substr(p+1);

        if(id > max_id) max_id = id;
        dict.insert({url, id});
        if(cnt % 100000 == 0){
            double_t d = (total_bytes / (double_t) size) * 100;
            std::cout << "\r" << d << "% (" << total_bytes << "/" << size << ") bytes read." << std::flush;
        }
        ++cnt;
    }
    double_t d = 100;
    std::cout << "\r" << d << "% (" << total_bytes << "/" << total_bytes << ") bytes read." << std::endl;
    in.close();
    return max_id;
}


uint64_t file_to_id2str(const std::string &file_name, std::unordered_map<uint32_t, std::string> &dict){
    std::ifstream in(file_name);
    std::string url;
    uint32_t id;
    uint32_t max_id = 0;
    while(true){
        in >> id >> url;
        if(in.eof()) break;
        if(id > max_id) max_id = id;
        dict.insert({id, url});
    }
    in.close();
    return max_id;
}

void map_to_file(const std::string &file_name, std::unordered_map<std::string, uint32_t> &map, bool append = false){

    std::vector<std::pair<std::string, uint32_t>> pairs(map.begin(), map.end());
    map.clear();
    std::sort(pairs.begin(), pairs.end(), sortbysec);

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

void map_to_file_unsorted(const std::string &file_name, std::unordered_map<std::string, uint32_t> &map){
    std::ofstream out(file_name);
    for(const auto &pair : map){
        out << pair.second << " " << pair.first << std::endl;
    }
    out.close();
}

std::string next(const std::string &line, uint64_t &beg){
    uint64_t  b, e;
    b = line.find('<', beg); //Check URI
    if(b == std::string::npos){
        b = line.find('"', beg); //Check Literal
        if(b == std::string::npos){ //No literal
            b = beg + 1;
            e = line.rfind('.', b) - 2;
            if (e == std::string::npos) {
                e = line.size() - 1;
            }
        }else { //Literal
            e = line.rfind('.', b) - 2;
            if (e == std::string::npos) {
                e = line.size() - 1;
            }
        }
    }else{ //Uri
        e = line.find('>', beg);
    }
    beg = e + 1;
    return line.substr(b, e-b+1);
}

uint32_t get_id(const std::string &v, std::unordered_map<std::string, uint32_t> &map,
                uint32_t &new_id){

    auto it = map.find(v);
    if(it != map.end()){
        return it->second;
    }else{
        ++new_id;
        map.insert({v, new_id});
        return new_id;
    }
}


void dat2ttl(const std::string &dat, const std::string &ttl,
             std::unordered_map<uint32_t, std::string> &map_so,
             std::unordered_map<uint32_t, std::string> &map_p){

    std::ifstream in(dat);
    std::ofstream out(ttl);
    uint32_t s, p, o;
    std::string s_uri, p_uri, o_uri;
    while(true){
        in >> s >> p >> o;
        if(in.eof()) break;
        out << map_so[s] << " " << map_p[p] << " " << map_so[o] << " ." << std::endl;
    }
    in.close();
    out.close();
}

void ttl2dat(const std::string &dat, const std::string &ttl,
             uint32_t max_so, uint32_t max_p,
             std::unordered_map<std::string, uint32_t> &map_so, std::unordered_map<std::string, uint32_t> &map_p){

    uint64_t size = utils::file::file_size(ttl);
    std::ifstream in(ttl);
    std::ofstream out(dat);
    std::string line;
    std::string s, p, o;
    uint32_t s_id, p_id, o_id;
    uint64_t cnt = 0, total_bytes = 0;
    while (std::getline(in, line)) {
        total_bytes += line.size();
        uint64_t index = 0;
        s = next(line, index);
        s_id = get_id(s, map_so, max_so);
        p = next(line, index);
        p_id = get_id(p, map_p, max_p);
        o = next(line, index);
        o_id = get_id(o, map_so, max_so);
        out << s_id << " " << p_id << " " << o_id << std::endl;
        if(cnt % 100000 == 0){
            double_t d = (total_bytes / (double_t) size) * 100;
            std::cout << "\r" << d << "% (" << total_bytes << "/" << size << ") bytes read." << std::flush;
        }
        ++cnt;
    }
    double_t d = (total_bytes / (double_t) total_bytes) * 100;
    std::cout <<  "\r" <<  d << "% (" << total_bytes << "/" << total_bytes << ") bytes read." << std::flush;
    in.close();
    out.close();
}

void ttl2clean(const std::string &src, const std::string &tgt, const std::string &pattern){

    uint64_t size = utils::file::file_size(src);
    std::ifstream in(src);
    std::ofstream out(tgt);
    std::string line;
    std::string s, p, o;
    uint32_t s_id, p_id, o_id;
    uint64_t cnt = 0, total_bytes = 0;
    while (std::getline(in, line)) {
        total_bytes += line.size();
        uint64_t index = 0;
        s = next(line, index);
        p = next(line, index);
        o = next(line, index);
        if(p.substr(0, pattern.size()) == pattern){
            out << s << " " << p << " " << o << " ." << std::endl;
        }
        if(cnt % 100000 == 0){
            double_t d = (total_bytes / (double_t) size) * 100;
            std::cout << "\r" << d << "% (" << total_bytes << "/" << size << ") bytes read." << std::flush;
        }
        ++cnt;
    }
    double_t d = (total_bytes / (double_t) total_bytes) * 100;
    std::cout <<  "\r" <<  d << "% (" << total_bytes << "/" << total_bytes << ") bytes read." << std::flush;
    in.close();
    out.close();
}


void knn(const std::string &in_file, const std::string &out_file,
             const uint32_t k,
             std::unordered_map<std::string, uint32_t> &map_so){




    std::ifstream in(in_file);
    std::ofstream out(out_file);
    std::string line;
    std::string n1, n2;
    uint32_t n1_id, n2_id, l = 0, max_so = 0, p_n1 = 0;
    std::vector<id_list_t> lists;
    std::vector<uint32_t> adjs;
    while (std::getline(in, line)) {
        uint64_t index = 0;
        n1 = next(line, index);
        n1_id = get_id(n1, map_so, max_so);
        n2 = next(line, index);
        n2_id = get_id(n2, map_so, max_so);
        if(l > 0 && n1_id != p_n1){
            lists.emplace_back(id_list_t{p_n1, adjs});
            adjs.clear();
        }
        adjs.emplace_back(n2_id);
        ++l;
        p_n1 = n1_id;
    }
    lists.emplace_back(id_list_t{p_n1, adjs});
    in.close();
    std::sort(lists.begin(), lists.end(), sortbyid);
    uint64_t l_i = 0, c_lower = 0;
    for(const auto& e : lists){
        if(e.adjs.size() < k) {
            std::cout << "List of " << e.id << " is smaller than k=" << k << ": " << e.adjs.size() << std::endl;
        }
        for(uint64_t i = 0; i < k; ++i){
            if(i < e.adjs.size()) {
                out << e.adjs[i];
            }else{
                out << 0;
            }
            if(i < k-1) out << " ";
        }
        out << std::endl;
        ++l_i;
    }
    out.close();
}


void append(const std::string &ofile, const std::string &ifile,
             uint32_t max_so, uint32_t max_p,
             std::unordered_map<std::string, uint32_t> &map_so, std::unordered_map<std::string, uint32_t> &map_p){


    std::ifstream in(ifile);
    std::ofstream out(ofile, std::ios_base::app);
    std::string line;
    std::string s, p, o;
    uint32_t s_id, p_id, o_id;
    while (std::getline(in, line)) {
        uint64_t index = 0;
        s = next(line, index);
        s_id = get_id(s, map_so, max_so);
        p = next(line, index);
        p_id = get_id(p, map_p, max_p);
        o = next(line, index);
        o_id = get_id(o, map_so, max_so);
        out << s_id << " " << p_id << " " << o_id << std::endl;
    }
    in.close();
    out.close();
}


#endif //RING_RDF_HPP
