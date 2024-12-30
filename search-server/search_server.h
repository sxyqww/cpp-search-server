#pragma once

#include "document.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;


class SearchServer {
public:
    SearchServer(const std::string& stop_word_text);
    
    template <typename Container>
    SearchServer(const Container& stop_word_container);

    void SetStopWords(const std::string& text);
    
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
    
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Predicate predicate) const;
    
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    
    int GetDocumentCount() const;
    
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    
    int GetDocumentId(const int& index);



private:
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::map<std::string, std::map<int, double>> word_to_document_freqs;
    std::set<std::string> stop_words_;
    std::map<int, int> document_ratings_;
    std::map<int, DocumentStatus> document_status_;
    std::vector<int> document_ids_;

    bool IsValidSymbol(const std::string& text) const;
    bool IsStopWord(const std::string& word) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    QueryWord ParseQueryWord(std::string text) const;
    Query ParseQuery(const std::string& text) const;

    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);
};

template <typename Container>
SearchServer::SearchServer(const Container& stop_word_container) {
    for (const auto& word : stop_word_container) {
        if (!word.empty()) {
            if (!IsValidSymbol(word)) {
                throw std::invalid_argument("Stop words contain invalid characters (ASCII 0-31)");
            }
            stop_words_.insert(word);
        }
    }
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Predicate predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance;
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    std::map<int, double> doc_relevance;
    for (const auto& word_plus : query.plus_words) {
        if (word_to_document_freqs.count(word_plus) > 0) {
            const double idf = log(GetDocumentCount() * 1.0 / word_to_document_freqs.at(word_plus).size());
            for (const auto& [relevance_id, term_freq] : word_to_document_freqs.at(word_plus)) {
                doc_relevance[relevance_id] += idf * term_freq;
            }
        }
    }
    for (const auto& word_minus : query.minus_words) {
        if (word_to_document_freqs.count(word_minus) > 0) {
            for (const auto& [relevance_id, _] : word_to_document_freqs.at(word_minus)) {
                doc_relevance.erase(relevance_id);
            }
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [id, relevance] : doc_relevance) {
        const auto document_status = document_status_.at(id);
        const auto document_rating = document_ratings_.at(id);
        if (predicate(id, document_status, document_rating)) {
            matched_documents.push_back({id, relevance, document_rating});
        }
    }
    return matched_documents;
}
