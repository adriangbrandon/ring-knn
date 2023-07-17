/*
 * query-index.cpp
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
#include <utility>
#include <chrono>
#include <triple_pattern.hpp>
#include <ltj_algorithm_similarity_baseline.hpp>
#include <ltj_algorithm_similarity_baseline_v2.hpp>
#include <utils.hpp>

using namespace std;
using namespace std::chrono;

bool get_file_content(string filename, vector<string> & vector_of_strings)
{
    // Open the File
    ifstream in(filename.c_str());
    // Check if object is valid
    if(!in)
    {
        cerr << "Cannot open the File : " << filename << endl;
        return false;
    }
    string str;
    // Read the next line from File until it reaches the end.
    while (getline(in, str))
    {
        // Line contains string of length > 0 then save it in vector
        if(str.size() > 0)
            vector_of_strings.push_back(str);
    }
    //Close The File
    in.close();
    return true;
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

bool is_variable(string & s)
{
    return (s.at(0) == '?');
}

bool is_similarity(string &s){
    return (s.at(0) == 'k');
}


bool is_best(string &s){
    return (s.at(0) == 'b');
}

uint8_t get_variable(string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars){
    auto var = s.substr(1);
    auto it = hash_table_vars.find(var);
    if(it == hash_table_vars.end()){
        uint8_t id = hash_table_vars.size();
        hash_table_vars.insert({var, id });
        return id;
    }else{
        return it->second;
    }
}

uint64_t get_constant(string &s){
    return std::stoull(s);
}

uint64_t get_k_sim(string &s){
    return std::stoull(s.substr(1));
}

uint64_t get_k_best(string &s){
    return std::stoull(s.substr(1));
}

ring_ltj::triple_pattern get_triple(string & s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
    vector<string> terms = tokenizer(s, ' ');

    ring_ltj::triple_pattern triple;
    if(is_variable(terms[0])){
        triple.var_s(get_variable(terms[0], hash_table_vars));
    }else{
        triple.const_s(get_constant(terms[0]));
    }
    if(is_variable(terms[1])){
        triple.var_p(get_variable(terms[1], hash_table_vars));
    }else if(is_similarity(terms[1])) {
        triple.similarity(get_k_sim(terms[1]));
    }else if(is_best(terms[1])){
        triple.best(get_k_best(terms[1]));
    }else{
        triple.const_p(get_constant(terms[1]));
    }
    if(is_variable(terms[2])){
        triple.var_o(get_variable(terms[2], hash_table_vars));
    }else{
        triple.const_o(get_constant(terms[2]));
    }
    return triple;
}

std::string get_type(const std::string &file){
    auto p = file.find_last_of('.');
    return file.substr(p+1);
}


template<class ring_type, class ltj_algorithm>
void query(const std::string &file, const std::string &queries){
    vector<string> dummy_queries;
    bool result = get_file_content(queries, dummy_queries);

    ring_type graph;

    cout << " Loading the index..."; fflush(stdout);
    sdsl::load_from_file(graph, file);

    cout << endl << " Index loaded " << sdsl::size_in_bytes(graph) << " bytes" << endl;

    std::ifstream ifs;
    uint64_t nQ = 0;

    high_resolution_clock::time_point start, stop;

    if(result)
    {

        int count = 1;
        for (string& query_string : dummy_queries) {

            //vector<Term*> terms_created;
            //vector<Triple*> query;
            std::unordered_map<std::string, uint8_t> hash_table_vars;
            std::vector<ring_ltj::triple_pattern> query, query_sim;
            vector<string> tokens_query = tokenizer(query_string, '.');
            bool best = false, skip = false;
            uint64_t k_best = 0;
            for (uint64_t i = 0; !skip && i < tokens_query.size(); ++i) {
                string& token = tokens_query[i];
                auto triple_pattern = get_triple(token, hash_table_vars);
                if(triple_pattern.is_best()){
                    if(best){
                        skip = (k_best != triple_pattern.k_best);
                    }else{
                        best = true;
                        k_best = triple_pattern.k_best;
                    }
                }
                if(triple_pattern.is_similarity()){
                    query_sim.push_back(triple_pattern);
                }else{
                    query.push_back(triple_pattern);
                }

            }
            if(skip) {
                std::cout << "Incorrect query" << std::endl;
                continue;
            }


            // vector<string> gao = get_gao(query);
            // vector<string> gao = get_gao_min_opt(query, graph);
            // cout << gao [0] << " - " << gao [1] << " - " << gao[2] << endl;

            typedef std::vector<typename ltj_algorithm::tuple_type> results_type;
            results_type res;

            start = high_resolution_clock::now();
            ltj_algorithm ltj(&query, &query_sim, &graph);
            ltj.join(res, 0, 600);
            stop = high_resolution_clock::now();
            auto total_time = duration_cast<nanoseconds>(stop - start).count();

            std::unordered_map<uint8_t, std::string> ht;
            for(const auto &p : hash_table_vars){
                ht.insert({p.second, p.first});
            }

            //cout << "Query Details:" << endl;




            cout << nQ <<  ";" << res.size() << ";" << total_time << endl;

            std::string file_name = "res/q" + std::to_string(count) + ".txt";
            //cout << "##########" << endl;
            //ltj.print_query(ht);
            ltj.print_results(res, ht, file_name);
            //cout << "##########" << endl;

            nQ++;

            // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

            //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
            count += 1;
        }

        //graph.print_knngraph();
    }
}


int main(int argc, char* argv[])
{

    //typedef ring::c_ring ring_type;
    if(argc != 3){
        std::cout << "Usage: " << argv[0] << " <index> <queries>" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string queries = argv[2];
    std::string type = get_type(index);

    if(type == "ring-knn-naive"){
        typedef ring_ltj::ring_knn_naive_v2<> ring_type;
        typedef ring_ltj::ltj_algorithm_similarity_baseline<ring_type, uint8_t, uint64_t> ltj_algorithm_type;
        query<ring_type, ltj_algorithm_type>(index, queries);
    }else if (type == "c-ring-knn-naive"){
        typedef ring_ltj::c_ring_knn_naive_v2 ring_type;
        typedef ring_ltj::ltj_algorithm_similarity_baseline<ring_type, uint8_t, uint64_t> ltj_algorithm_type;
        query<ring_type, ltj_algorithm_type>(index, queries);
    }else if (type == "ring-sel-knn-naive") {
        typedef ring_ltj::ring_knn_naive_v2_sel ring_type;
        typedef ring_ltj::ltj_algorithm_similarity_baseline<ring_type, uint8_t, uint64_t> ltj_algorithm_type;
        query<ring_type, ltj_algorithm_type>(index, queries);
    }else{
        std::cout << "Type of index: " << type << " is not supported." << std::endl;
    }



	return 0;
}

