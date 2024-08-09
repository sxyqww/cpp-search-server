#include "request_queue.h"
#include <algorithm>

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

RequestQueue::QueryResult::QueryResult(int timestamp, int result_count) 
    : timestamp(timestamp), result_count(result_count) {
}

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddResult(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto result = search_server_.FindTopDocuments(raw_query, status);
    AddResult(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto result = search_server_.FindTopDocuments(raw_query);
    AddResult(result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}

void RequestQueue::AddResult(int result_count) {
    ++current_time_;
    while (!requests_.empty() && requests_.front().timestamp <= current_time_ - min_in_day_) {
        if (requests_.front().result_count == 0) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }

    requests_.emplace_back(current_time_, result_count);

    if (result_count == 0) {
        ++no_result_requests_;
    }
}
