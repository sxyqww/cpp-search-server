#include "remove_duplicates.h"
#include <set>
#include <iostream>


void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> unique_word_sets;
    std::vector<int> duplicates_to_removes;

    for (int document_id : search_server) {
        const auto& word_frequencies = search_server.GetWordFrequencies(document_id);
        
        std::set<std::string> set_words;
        for (const auto& [word, _]: word_frequencies) {
            set_words.insert(word);
        }

        if (unique_word_sets.count(set_words) > 0) {
            duplicates_to_removes.push_back(document_id);
        }
        else {
            unique_word_sets.insert(set_words);
        }
    }

    for (int document_id: duplicates_to_removes) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id " << document_id << std::endl;

    }
}