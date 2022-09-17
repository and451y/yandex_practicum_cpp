#include "request_queue.h"
#include "search_server.h"
#include "document.h"

RequestQueue::RequestQueue(const SearchServer& search_server) noexcept
    : search_server_(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentStatus status) {
  const std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
  RefreshDeque(result);
  return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
  const std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
  RefreshDeque(result);
  return result;
}

int RequestQueue::GetNoResultRequests() const noexcept {
  return empty_resul_cnt_;
}

void RequestQueue::RefreshDeque(const std::vector<Document>& result) {
  if (static_cast<int>(requests_.size()) >= sec_in_day_) {
    if (requests_.front().empty_result == true)
      --empty_resul_cnt_;

    requests_.pop_front();
  }

  if (result.empty()) {
    requests_.push_back( {true});
    ++empty_resul_cnt_;
  } else {
    requests_.push_back( {false});
  }
}
