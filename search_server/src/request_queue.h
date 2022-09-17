#pragma once

#include <deque>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

class RequestQueue {
 public:
  explicit RequestQueue(const SearchServer& search_server) noexcept;

  template<typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string& raw_query,
                                       DocumentPredicate document_predicate);

  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

  std::vector<Document> AddFindRequest(const std::string& raw_query);

  int GetNoResultRequests() const noexcept;
 private:
  struct QueryResult {
    bool empty_result;
  };

  std::deque<QueryResult> requests_;
  const static int sec_in_day_ = 1440;
  const SearchServer& search_server_;
  int empty_resul_cnt_ = 0;

  void RefreshDeque(const std::vector<Document>& result);

};

template<typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentPredicate document_predicate) {
  const std::vector<Document> result = search_server_.FindTopDocuments(raw_query,
                                                                       document_predicate);
  RefreshDeque(result);
  return result;
}
