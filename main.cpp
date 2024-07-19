#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

// Function to read a line from the input
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

// Function to read a line from the input and return it as a number
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine(); // Clear the input buffer
    return result;
}

// Function to split a string into words
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// Class to represent the search server
class SearchServer {
public:
    // Set the stop words for the search server
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    // Add a document to the search server
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {    
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double frequency = 1 * 1.0 / words.size();
        for (auto & word : words) {
           word_to_document_freqs[word][document_id] += frequency;
        }
        document_ratings_[document_id] = ComputeAverageRating(ratings);
        document_status_[document_id] = status;
    }

    // Find the top documents matching the query
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, status);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    int GetDocumentCount() const {
        return document_status_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs.count(word) && word_to_document_freqs.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs.count(word) && word_to_document_freqs.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        sort(matched_words.begin(), matched_words.end());
        matched_words.erase(unique(matched_words.begin(), matched_words.end()), matched_words.end());

        return {matched_words, document_status_.at(document_id)};
    }       
private:
    // Struct to represent a query word
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // Struct to represent a query
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    // Map to store documents
    map<string, map<int, double>> word_to_document_freqs;
    set<string> stop_words_;
    map<int, int> document_ratings_;
    map<int, DocumentStatus> document_status_;

    // Check if a word is a stop word
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // Split a string into words excluding stop words
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // Parse a query word
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    // Parse a query
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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
    vector<Document> FindAllDocuments(const Query& query, DocumentStatus status) const {
        map<int, double> doc_relevance;
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
        
        vector<Document> matched_documents;
        for (const auto& [id, relevance] : doc_relevance) {
            if (document_status_.at(id) == status) {

                matched_documents.push_back({id, relevance, document_ratings_.at(id)});
            
        }
        }
        return matched_documents;
    }

    // Compute the average rating of a document
    static int ComputeAverageRating(const vector<int>& ratings) {
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

// Function to create a search server
/*SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();
        int rating_size;
        cin >> rating_size;
        
        // Create a vector of size rating_size filled with zeros
        vector <int> ratings(rating_size, 0);

        for (auto& elem : ratings) {
            cin >> elem; // Fill the vector with rating values
        }

        search_server.AddDocument(document_id, document, ratings);
        ReadLine(); // Clear the input buffer
    }

    return search_server;
*/
// ====================================ПРИМЕР===============================================
void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    const int document_count = search_server.GetDocumentCount();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const auto [words, status] = search_server.MatchDocument("пушистый кот"s, document_id);
        PrintMatchDocumentResult(document_id, words, status);
    }
}