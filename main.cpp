#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <stdexcept>
using namespace std::literals;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
// Function to read a line from the input
std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// Function to read a line from the input and return it as a number
int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine(); // Clear the input buffer
    return result;
}

// Function to split a string into words
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

// Struct to represent a document
struct Document {
    Document() : id(0), relevance(0.0), rating(0) {
    }
    Document(int id_, double relevance_, int rating_) :
    id(id_), relevance(relevance_), rating(rating_) {
        
    }
    
    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// Class to represent the search server
class SearchServer {
public:
    SearchServer(const std::string& stop_word_text) {
        SetStopWords(stop_word_text);
    }

    template <typename Container>

    SearchServer(const Container& stop_word_container) {
        for (const auto& word : stop_word_container) {
            if (!word.empty()) {
                if (!IsValidSymbol(word)) {
                    throw std::invalid_argument("Stop words contain invalid characters (ASCII 0-31)");
                }
                stop_words_.insert(word);
            }
        }
    }

    // Set the stop words for the search server

    void SetStopWords(const std::string& text) {
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsValidSymbol(word)) {
                throw std::invalid_argument("Stop words contain invalid characters (ASCII 0-31)");
            }
            stop_words_.insert(word);
        }
    }

    // Add a document to the search server
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
        if (document_id < 0) {
            throw std::invalid_argument("Attempt to add a document with a negative ID!");
        }
        
        if (document_ratings_.count(document_id) > 0)  {
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
    // Find the top documents matching the query
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Predicate predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        std::sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs){
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)  {
            return lhs.rating > rhs.rating;
        }
            return lhs.relevance > rhs.relevance;
    });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    // Overload for FindTopDocuments without a status filter (defaults to ACTUAL)
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, [](int, DocumentStatus status, int) {
            return status == DocumentStatus::ACTUAL;
        });
    }

    // New method to find top documents by query and specific DocumentStatus
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        auto status_predicate = [status](int, DocumentStatus doc_status, int) {
            return doc_status == status;
        };
        return FindTopDocuments(raw_query, status_predicate);
    }
    int GetDocumentCount() const {
        return document_status_.size();
    }

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const {
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

        sort(matched_words.begin(), matched_words.end());
        matched_words.erase(unique(matched_words.begin(), matched_words.end()), matched_words.end());

        return {matched_words, document_status_.at(document_id)};
    }
    
    int GetDocumentId(const int& index) {
        if (!(index >= 0 && index < static_cast<int>(document_ids_.size()))) {
            throw std::out_of_range("The index of the transmitted document is out of the acceptable range!");
        }

        return document_ids_.at(index);
    }
private:
    // Struct to represent a query word
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    // Struct to represent a query
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    // Map to store documents
    std::map<std::string, std::map<int, double>> word_to_document_freqs;
    std::set<std::string> stop_words_;
    std::map<int, int> document_ratings_;
    std::map<int, DocumentStatus> document_status_;
    std::vector<int> document_ids_;


     bool IsValidSymbol(const std::string& text) const {
        for (const char& symbol: text) {
            if (symbol >= 0 && symbol <= 31) {
                return false;
            }
        }
        return true;
    }
    // Check if a word is a stop word
    bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    // Split a string into words excluding stop words
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // Parse a query word
    QueryWord ParseQueryWord(std::string text) const {
        bool is_minus = false;
        if (!IsValidSymbol(text)) {
            throw std::invalid_argument("The presence of invalid characters (with codes from 0 to 31) in the text of the document being added");
        }
        if (text[0] == '-') {
            if (text[1] ==  '-') {
                throw  std::invalid_argument("The presence of more than one minus sign before words that should not be in the required documents, for example, fluffy is a cat. In the middle of the words, cons are resolved, for example: time-out.");
            }
            is_minus = true;
            text = text.substr(1);
            if (text.empty()) throw std::invalid_argument("The absence of text after the 'minus' symbol in the search query: fluffy -");
            }
        return {text, is_minus, IsStopWord(text)};
        }
    

    // Parse a query
    Query ParseQuery(const std::string& text) const {
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

    // Find all documents matching the query
    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        std::map<int, double> doc_relevance;
        for (const auto& word_plus: query.plus_words) {
            if (word_to_document_freqs.count(word_plus) > 0) {
                const double idf = log(GetDocumentCount() * 1.0 / word_to_document_freqs.at(word_plus).size());
                for (const auto& [relevance_id, term_freq] : word_to_document_freqs.at(word_plus)) {
                    doc_relevance[relevance_id] += idf * term_freq;
                }
            }
        }
        for (const auto& word_minus: query.minus_words) {
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

    // Compute the average rating of a document
    static int ComputeAverageRating(const std::vector<int>& ratings) {
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
};


template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    Iterator begin() const {
        return first_;
    }

    Iterator end() const {
        return last_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};


template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
 std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}
// ====================================ПРИМЕР===============================================
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
int main() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Page break"s << std::endl;
    }
}

 