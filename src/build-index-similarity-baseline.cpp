/*
 * build-index.cpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
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


#include <iostream>
#include "ring_knn_naive_v2.hpp"
#include <fstream>
#include <sdsl/construct.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

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

std::vector<uint64_t> tokenizer(const std::string &input, const char &delimiter){
    std::stringstream stream(input);
    std::string token;
    std::vector<uint64_t> res;
    while(getline(stream, token, delimiter)){
        std::string str = trim(token);
        uint64_t val = stoull(str);
        res.emplace_back(val);
    }
    return res;
}

uint64_t read_graph(std::ifstream &ifs, knn_graph_type &knn_graph){
    std::string line;
    uint64_t max_k = 0;
    while(std::getline(ifs, line)){
        vector<uint64_t> terms = tokenizer(line, ' ');
        std::vector<knn_item_type> list;
        for(uint64_t i = 0; i < terms.size(); ++i){
            knn_item_type item{terms[i], i+1};
            list.emplace_back(item);
            if(max_k < item.k) max_k = item.k;
        }
        knn_graph.push_back(list);
    }
    return max_k;
}

template<class ring>
void build_index(const std::string &dataset, const std::string &output){

    std::string data = dataset + ".dat";
    std::string dir = dataset + "-knn-dir.dat";

    vector<spo_triple> D, E;

    std::ifstream ifs(data);
    uint64_t s, p , o;
    do {
        ifs >> s >> p >> o;
        D.push_back(spo_triple(s, p, o));

    } while (!ifs.eof());
    D.shrink_to_fit();

    knn_graph_type g;
    std::ifstream ifs_dir(dir);
    auto max_k = read_graph(ifs_dir, g);

    cout << "--Indexing " << D.size() << " triples" << endl;
    memory_monitor::start();
    auto start = timer::now();

    ring A(D, g, max_k);
    auto stop = timer::now();
    memory_monitor::stop();
    cout << "  Index built  " << sdsl::size_in_bytes(A) << " bytes" << endl;

    sdsl::store_to_file(A, output);
    cout << "Index saved" << endl;
    cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
    cout << memory_monitor::peak() << " bytes." << endl;

}

int main(int argc, char **argv)
{

    if(argc != 3){
        std::cout << "Usage: " << argv[0] << " <dataset> [ring|c-ring|ring-sel]" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string type    = argv[2];
    if(type == "ring"){
        std::string index_name = dataset + ".ring-knn-naive";
        build_index<ring_ltj::ring_knn_naive_v2<>>(dataset, index_name);
    }else if (type == "c-ring"){
        std::string index_name = dataset + ".c-ring-knn-naive";
        build_index<ring_ltj::c_ring_knn_naive_v2>(dataset, index_name);
    }else if (type == "ring-sel") {
        std::string index_name = dataset + ".ring-sel-knn-naive";
        build_index<ring_ltj::ring_knn_naive_v2_sel>(dataset, index_name);
    }else{
        std::cout << "Usage: " << argv[0] << " <dataset> [ring|c-ring|ring-sel]" << std::endl;
    }

    return 0;
}

