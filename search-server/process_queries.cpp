#include "process_queries.h"

using std::vector;


vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<std::string>& queries) {
        vector<vector<Document>> res(queries.size());
        std::transform(std::execution::par, queries.begin(), queries.end(), res.begin(), 
        [&search_server] (const std::string& query) {
            return search_server.FindTopDocuments(query);
        }
        );
    return res; 
    }

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<std::string>& queries) {
        vector<vector<Document>> partial_result = ProcessQueries(search_server, queries);
        std::list<Document> joined_result;
        for (const auto& vec_doc: partial_result) {
            joined_result.insert(joined_result.end(), vec_doc.begin(), vec_doc.end());
        }

        return joined_result;
    }