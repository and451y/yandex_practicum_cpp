#pragma once

#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "document.h"

std::string sv_to_s(std::string_view sv);

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words,
                              DocumentStatus status);

std::string ReadLine();

int ReadLineWithNumber();

std::vector<std::string_view> SplitIntoWords(std::string_view str);

std::ostream& operator<<(std::ostream& out, const Document& document);

template<typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings);

template <typename Map>
bool key_compare(Map const &lhs, Map const &rhs);

template<typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
  std::set<std::string, std::less<>> non_empty_strings;

  for (const auto& str : strings) {
    if (!str.empty()) {
      non_empty_strings.insert(sv_to_s(str));
    }
  }

  return non_empty_strings;
}

template <typename Map>
bool key_compare(Map const &lhs, Map const &rhs) {
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                      [] (auto a, auto b) { return a.first == b.first; });
}
