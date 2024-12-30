#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

RequestQueue::QueryResult::QueryResult(int timestamp, int result_count) 
    : timestamp_(timestamp), result_count_(result_count) {
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
    while (!requests_.empty() && requests_.front().timestamp_ <= current_time_ - min_in_day_) {
        if (requests_.front().result_count_ == 0) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }

    requests_.emplace_back(current_time_, result_count);

    if (result_count == 0) {
        ++no_result_requests_;
    }
}
