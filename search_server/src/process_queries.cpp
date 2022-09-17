#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
                                                  const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    
    std::transform(std::execution::par,
                   queries.begin(), queries.end(),
                   result.begin(),
                   [&search_server](const std::string& querie){return search_server.FindTopDocuments(querie);}
                   
    );
        
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    auto queries_result = ProcessQueries(search_server, queries);
    size_t size = 0;
    for (auto documents : queries_result) {
        size += documents.size();
    }
    
    std::vector<Document> flat_result(size);
    auto it = flat_result.begin();
    for (auto documents : queries_result) {
        it = std::move(documents.begin(), documents.end(), it);
    }
    
    return flat_result;
}
