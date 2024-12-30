#pragma once
#include "search_server.h"
#include <deque>
#include <vector>
#include <algorithm>


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int timestamp_;
        int result_count_;

        QueryResult(int timestamp, int result_count);
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int no_result_requests_ = 0;
    int current_time_ = 0;

    void AddResult(int result_count);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddResult(result.size());
    return result;
}



