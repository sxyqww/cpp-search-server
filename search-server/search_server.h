#pragma once

#include "document.h"
#include <map>
#include <set>
#include <string>
#include <vector>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

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

