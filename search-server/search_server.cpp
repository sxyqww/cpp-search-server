#include "search_server.h"
#include "string_processing.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>

SearchServer::SearchServer(const std::string& stop_word_text) {
    SetStopWords(stop_word_text);
}

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

void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidSymbol(word)) {
            throw std::invalid_argument("Stop words contain invalid characters (ASCII 0-31)");
        }
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Attempt to add a document with a negative ID!");
    }

    if (document_ratings_.count(document_id) > 0) {
        throw std::invalid_argument("Attempt to add a document with the id of a previously added document!");
    }
    if (!IsValidSymbol(document)) {
        throw std::invalid_argument("The presence of invalid characters (with codes from 0 to 31) in the text of the document being added");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double frequency = 1 * 1.0 / words.size();
    for (auto & word : words) {
        word_to_document_freqs[word][document_id] += frequency;
    }
    document_ratings_[document_id] = ComputeAverageRating(ratings);
    document_status_[document_id] = status;
    document_ids_.push_back(document_id);
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

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, [](int, DocumentStatus status, int) {
        return status == DocumentStatus::ACTUAL;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    auto status_predicate = [status](int, DocumentStatus doc_status, int) {
        return doc_status == status;
    };
    return FindTopDocuments(raw_query, status_predicate);
}

int SearchServer::GetDocumentCount() const {
    return document_status_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs.count(word) && word_to_document_freqs.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs.count(word) && word_to_document_freqs.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, document_status_.at(document_id)};
}

int SearchServer::GetDocumentId(const int& index) {
    if (!(index >= 0 && index < static_cast<int>(document_ids_.size()))) {
        throw std::out_of_range("The index of the transmitted document is out of the acceptable range!");
    }

    return document_ids_.at(index);
}

bool SearchServer::IsValidSymbol(const std::string& text) const {
    for (const char& symbol : text) {
        if (symbol >= 0 && symbol <= 31) {
            return false;
        }
    }
    return true;
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    if (!IsValidSymbol(text)) {
        throw std::invalid_argument("The presence of invalid characters (with codes from 0 to 31) in the text of the document being added");
    }
    if (text[0] == '-') {
        if (text[1] == '-') {
            throw std::invalid_argument("The presence of more than one minus sign before words that should not be in the required documents, for example, fluffy is a cat. In the middle of the words, cons are resolved, for example: time-out.");
        }
        is_minus = true;
        text = text.substr(1);
        if (text.empty()) throw std::invalid_argument("The absence of text after the 'minus' symbol in the search query: fluffy -");
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
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

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int sum_assesments = 0;
    for (const int& elem : ratings) {
        sum_assesments += elem;
    }
    const int average_rating = sum_assesments / static_cast<int>(ratings.size());
    return average_rating;
}
