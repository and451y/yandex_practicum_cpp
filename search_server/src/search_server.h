#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <execution>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <utility>
#include <future>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::literals;

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
 public:
  template<typename StringContainer>
  explicit SearchServer(const StringContainer& stop_words);

  explicit SearchServer(const std::string& stop_words_text)
      : SearchServer(SplitIntoWords(stop_words_text))
  {
  }

  int GetDocumentCount() const noexcept {
    return documents_.size();
  }

  auto begin() const noexcept {
    return document_ids_.begin();
  }

  auto end() const noexcept {
    return document_ids_.end();
  }

  bool TryAddDocument(int document_id,
                      std::string_view document,
                      DocumentStatus status,
                      const std::vector<int>& ratings);

  bool TryFindTopDocuments(std::string_view raw_query);

  bool TryMatchDocuments(std::string_view query);

  void AddDocument(int document_id,
                   std::string_view document,
                   DocumentStatus status,
                   const std::vector<int>& ratings);

  const std::map<std::string_view, double> GetWordFrequencies(int document_id) const;

  void RemoveDocument(int document_id);
  void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
  void RemoveDocument(const std::execution::parallel_policy&, int document_id);

  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,
                                                                          std::string_view raw_query,
                                                                          int document_id) const;
  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,
                                                                          std::string_view raw_query,
                                                                          int document_id) const;

  std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

  std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

  template<typename DocumentPredicate>
  std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

  template<typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;

  template<typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
                                         std::string_view raw_query,
                                         DocumentStatus status) const;

  template<typename DocumentPredicate, typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
                                         std::string_view raw_query,
                                         DocumentPredicate document_predicate) const;

 private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };

  struct QueryWord {
    std::string_view data;
    bool is_minus;
    bool is_stop;
  };

  struct Query {
    std::set<std::string_view> plus_words;
    std::set<std::string_view> minus_words;
  };

  std::set<std::string, std::less<>> stop_words_;
  std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
  std::map<int, std::map<std::string, double, std::less<>>> document_to_word_freqs_;
  std::map<std::string_view, double> empty_map_ = {};
  std::map<int, DocumentData> documents_;
  std::vector<int> document_ids_;

  static bool IsValidMinusWord(const std::set<std::string_view>& minus_words);

  static int ComputeAverageRating(const std::vector<int>& ratings);

  bool IsStopWord(std::string_view word) const;

  bool IsValidWord(std::string_view word) const;
  bool IsValidWord(const std::execution::sequenced_policy&, std::string_view word) const;
  bool IsValidWord(const std::execution::parallel_policy&, std::string_view word) const;

  std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

  QueryWord ParseQueryWord(std::string_view text) const;

  Query ParseQuery(std::string_view text) const;

  double ComputeWordInverseDocumentFreq(const std::string& word) const;

  template <typename ExecutionPolicy, typename ForwardRange, typename Function>
  static void ForEach(const ExecutionPolicy& policy, ForwardRange& range, Function function);

  template<typename DocumentPredicate, typename ExecutionPolicy>
  std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy,
                                         const Query& query,
                                         DocumentPredicate document_predicate) const;
};

void RemoveDuplicates(SearchServer& search_server);

template <typename ExecutionPolicy, typename ForwardRange, typename Function>
void SearchServer::ForEach(const ExecutionPolicy& policy, ForwardRange& range, Function function) {
  if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>
      || std::is_same_v<typename std::iterator_traits<typename ForwardRange::iterator>::iterator_category,
                        std::random_access_iterator_tag>
  ) {
    std::for_each(policy, range.begin(), range.end(), function);
  } else {
    static constexpr int PART_COUNT = 8;
    const auto part_length = std::size(range) / PART_COUNT;
    auto part_begin = range.begin();
    auto part_end = std::next(part_begin, part_length);

    std::vector<std::future<void>> futures;
    for (int i = 0; i < PART_COUNT;
        ++i, part_begin = part_end, part_end =
            (i == PART_COUNT - 1) ? range.end() : next(part_begin, part_length)) {
      futures.push_back(std::async([function, part_begin, part_end] {
        for_each(part_begin, part_end, function);
      }));
    }
  }
}

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
  for (const auto& s : stop_words) {
    if (!IsValidWord(s))
      throw std::invalid_argument("stop word has invalid character"s);
  }

  for (const auto& w : MakeUniqueNonEmptyStrings(stop_words)) {
    stop_words_.insert(w);
  }
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
  return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
                                                  const ExecutionPolicy& policy,
                                                  std::string_view raw_query,
                                                  DocumentStatus status
 ) const
{
  return FindTopDocuments(policy, raw_query,
                          [status](int document_id, DocumentStatus document_status, int rating) {
                            return document_status == status;
                          });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
  return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
                                                       const ExecutionPolicy& policy,
                                                       std::string_view raw_query,
                                                       DocumentPredicate document_predicate
 ) const
{
  const auto query = ParseQuery(raw_query);

  auto matched_documents = FindAllDocuments(policy, query, document_predicate);

  sort(matched_documents.begin(), matched_documents.end(),
       [](const Document& lhs, const Document& rhs) {
         if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
           return lhs.rating > rhs.rating;
         } else {
           return lhs.relevance > rhs.relevance;
         }
       });

  if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
  }

  return matched_documents;
}

template<typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(
                                                       const ExecutionPolicy& policy,
                                                       const Query& query,
                                                       DocumentPredicate document_predicate
 ) const
{
  std::map<int, double> document_to_relevance;

  if constexpr (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
    ConcurrentMap<int, double> con_document_to_relevance(8);

    auto function = [&con_document_to_relevance, this, &document_predicate](const auto& word){
                        if (!word_to_document_freqs_.count(sv_to_s(word))) {
                          return;
                        }

                        const double inverse_document_freq = ComputeWordInverseDocumentFreq(sv_to_s(word));
                        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(sv_to_s(word))) {
                          const auto& document_data = documents_.at(document_id);
                          if (document_predicate(document_id, document_data.status, document_data.rating)) {
                            con_document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;;
                          }
                        }
                      };

    SearchServer::ForEach(policy, query.plus_words, function);

    document_to_relevance = move(con_document_to_relevance.BuildOrdinaryMap());
  } else {
      for (const auto& word : query.plus_words) {
        if (!word_to_document_freqs_.count(sv_to_s(word)))
          continue;

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(sv_to_s(word));

        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(sv_to_s(word))) {
          const auto& document_data = documents_.at(document_id);
          if (document_predicate(document_id, document_data.status, document_data.rating)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
          }
        }
      }
  }

  for (const auto& word : query.minus_words) {
    if (!word_to_document_freqs_.count(sv_to_s(word)))
      continue;

    for (const auto [document_id, _] : word_to_document_freqs_.at(sv_to_s(word))) {
      document_to_relevance.erase(document_id);
    }
  }

  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] : document_to_relevance) {
    matched_documents.push_back( {document_id, relevance, documents_.at(document_id).rating});
  }

  return matched_documents;
}
