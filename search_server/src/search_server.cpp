#include "search_server.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
  std::set<int> remove_documents;

  for (auto current_id = search_server.begin(); current_id != search_server.end(); ++current_id) {
    auto current_doc_id_to_word_freq = search_server.GetWordFrequencies(*current_id);

      for (auto next_id = current_id + 1; next_id != search_server.end(); ++next_id) {
        auto next_doc_id_to_word_freq = search_server.GetWordFrequencies(*next_id);

        if (key_compare(next_doc_id_to_word_freq, current_doc_id_to_word_freq))
          remove_documents.emplace(*next_id);
      }
  }

  for (auto document_id : remove_documents) {
    search_server.RemoveDocument(document_id);
  }
}

bool SearchServer::TryAddDocument(
                                    int document_id,
                                    string_view document,
                                    DocumentStatus status,
                                    const vector<int>& ratings
 )
{
  try {
    AddDocument(document_id, document, status, ratings);

    return true;
  } catch (const invalid_argument& e) {
    cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;

    return false;
  }
}

bool SearchServer::TryFindTopDocuments(string_view raw_query) {
  cout << "Результаты поиска по запросу: "s << raw_query << endl;
  try {
    for (const Document& document : FindTopDocuments(raw_query)) {
      PrintDocument(document);
    }
    return true;
  } catch (const invalid_argument& e) {
    cout << "Ошибка поиска: "s << e.what() << endl;
    return false;
  }
}

bool SearchServer::TryMatchDocuments(string_view query) {
  try {
    cout << "Матчинг документов по запросу: "s << query << endl;

    for (auto it = begin(); it != end(); ++it) {
      const auto [words, status] = MatchDocument(query, *it);
    }

    return true;
  } catch (const invalid_argument& e) {
    cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    return false;
  }
}

void SearchServer::AddDocument(
                                 int document_id,
                                 string_view document,
                                 DocumentStatus status,
                                 const vector<int>& ratings
 )
{
  if ((document_id < 0) || documents_.count(document_id))
    throw invalid_argument("Invalid document_id"s);

  const auto words = SplitIntoWordsNoStop(document);
  const double inv_word_count = 1.0 / words.size();

  for (const auto& word : words) {
    word_to_document_freqs_[sv_to_s(word)][document_id] += inv_word_count;
    document_to_word_freqs_[document_id][sv_to_s(word)] += inv_word_count;
  }

  documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
  document_ids_.push_back(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
  auto it = find(document_ids_.begin(), document_ids_.end(), document_id);

  if (it == document_ids_.end())
    return;

  document_ids_.erase(it);
  documents_.erase(document_id);
  document_to_word_freqs_.erase(document_id);

  std::vector<const std::string*> words;

  for (const auto& [word, _] : word_to_document_freqs_) {
    words.push_back(&word);
  }

  for_each(words.begin(), words.end(),
           [&](const std::string* word){word_to_document_freqs_.at(*word).erase(document_id);}
   );
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
  SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
  auto it = find(std::execution::par, document_ids_.begin(), document_ids_.end(), document_id);

  if (it == document_ids_.end())
    return;

  document_ids_.erase(it);
  documents_.erase(document_id);
  document_to_word_freqs_.erase(document_id);

  std::vector<const std::string*> words;

  for (const auto& [word, _] : word_to_document_freqs_) {
    words.push_back(&word);
  }

  for_each(std::execution::par,
           words.begin(), words.end(),
           [&](const std::string* word){word_to_document_freqs_.at(*word).erase(document_id);}
   );
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
  return FindTopDocuments(std::execution::seq, raw_query,
                          [status](int document_id, DocumentStatus document_status, int rating) {
                            return document_status == status;
                          });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
  return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

const std::map<string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
  auto document_id_to_word_freqs = document_to_word_freqs_.find(document_id);
  std::map<string_view, double> result;

  for (const auto& [word, freqs] : (*document_id_to_word_freqs).second) {
    result.emplace(word, freqs);
  }
  return ((document_id_to_word_freqs != document_to_word_freqs_.end()) ? result : empty_map_);
}

tuple<std::vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
  if (!IsValidWord(raw_query))
    throw std::invalid_argument("request has invalid character"s);

  if (!documents_.count(document_id))
    throw std::out_of_range("noexist id"s);

  std::vector<std::string_view> matched_words;
  const auto query = ParseQuery(raw_query);

  if (!IsValidMinusWord(query.minus_words))
    throw std::invalid_argument("incorrect syntax of the minus word"s);

  for (const auto& word : query.plus_words) {
    if (!word_to_document_freqs_.count(sv_to_s(word)))
      continue;

    if (word_to_document_freqs_.at(sv_to_s(word)).count(document_id))
      matched_words.push_back(word);
  }

  for (const auto& word : query.minus_words) {
    if (!word_to_document_freqs_.count(sv_to_s(word)))
      continue;

    if (word_to_document_freqs_.at(sv_to_s(word)).count(document_id)) {
      matched_words.clear();
      break;
    }
  }

  return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
                                                                                        const std::execution::sequenced_policy&,
                                                                                        string_view raw_query,
                                                                                        int document_id
 ) const
{
  return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
                                                                                        const std::execution::parallel_policy&,
                                                                                        string_view raw_query,
                                                                                        int document_id
 ) const
{
  if (!IsValidWord(std::execution::par, raw_query))
    throw std::invalid_argument("request has invalid character"s);

  if (!documents_.count(document_id))
    throw std::out_of_range("noexist id"s);

  std::vector<std::string_view> matched_words;
  const auto query = ParseQuery(raw_query);

  if (!IsValidMinusWord(query.minus_words))
    throw std::invalid_argument("incorrect syntax of the minus word"s);

  for (const auto& word : query.plus_words) {
    if (!word_to_document_freqs_.count(sv_to_s(word)))
      continue;

    if (word_to_document_freqs_.at(sv_to_s(word)).count(document_id))
      matched_words.push_back(word);
  }

  for (const auto& word : query.minus_words) {
    if (!word_to_document_freqs_.count(sv_to_s(word)))
      continue;

    if (word_to_document_freqs_.at(sv_to_s(word)).count(document_id)) {
      matched_words.clear();
      break;
    }
  }

  return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(std::string_view word) const {
  return stop_words_.count(sv_to_s(word));
}


bool SearchServer::IsValidMinusWord(const set<string_view>& minus_words) {
  for (const auto& w : minus_words) {
    if (w.empty() || w[0] == '-')
      return false;
  }

  return true;
}

bool SearchServer::IsValidWord(const std::execution::sequenced_policy&, std::string_view word) const {
  return none_of(std::execution::seq, word.begin(), word.end(), [](char c) {
    return c >= '\0' && c < ' ';
  });
}

bool SearchServer::IsValidWord(std::string_view word) const {
  return SearchServer::IsValidWord(std::execution::seq, word);
}

bool SearchServer::IsValidWord(const std::execution::parallel_policy&, std::string_view word) const {
  return none_of(std::execution::par, word.begin(), word.end(), [](char c) {
    return c >= '\0' && c < ' ';
  });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
  vector<string_view> words;
  for (const auto& word : SplitIntoWords(text)) {
    if (!IsValidWord(word)) {
      throw invalid_argument("Word "s + sv_to_s(word) + " is invalid"s);
    }

    if (!IsStopWord(sv_to_s(word)))
      words.push_back(word);
  }

  return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
  if (ratings.empty())
    return 0;

  int rating_sum = 0;
  for (const int rating : ratings) {
    rating_sum += rating;
  }

  return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
  if (text.empty())
    throw invalid_argument("Query word is empty"s);

  bool is_minus = false;

  if (text[0] == '-') {
    is_minus = true;
    text.remove_prefix(1);
  }

  if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
    throw invalid_argument("Query word "s + sv_to_s(text) + " is invalid");
  }

  return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
  Query result;

  for (const auto& word : SplitIntoWords(text)) {
    auto query_word = ParseQueryWord(word);
    if (!query_word.is_stop) {
      if (query_word.is_minus) {
        result.minus_words.insert(string_view(query_word.data));
      } else {
        result.plus_words.insert(string_view(query_word.data));
      }
    }
  }

  return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
  return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
