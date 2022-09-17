#pragma once

#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

#define RUN_TEST(func) RunTestImpl((func), #func)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

using namespace std::string_literals;

template<typename Container>
std::ostream& Print(std::ostream& os, const Container& container);

template<typename First, typename Second>
std::ostream& operator<<(std::ostream& os, const std::pair<First, Second>& container);

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container);

template<typename Element>
std::ostream& operator<<(std::ostream& os, const std::set<Element>& container);

template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& container);

template<typename T>
void RunTestImpl(T t, const std::string& func);

template<typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
                     const std::string& file, const std::string& func, unsigned line,
                     const std::string& hint);

void AssertImpl(bool value, const std::string& expr_str, const std::string& file,
                const std::string& func, unsigned line, const std::string& hint);

void TestAddDocument();
void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeMinusWordsFromAddedDocumentContent();
void TestMatchingDocument();
void TestSortFindDocumentsByDescentRelevance();
void TestComputeDocumentRating();
void TestUserPredicate();
void TestSearchSpecifiedStatusDoc();
void TestComputeRelevance();
void TestRemoveDuplicates();
void TestParrallelFindDoc();

void TestSearchServer();

template<typename Container>
std::ostream& Print(std::ostream& os, const Container& container) {
  using namespace std::string_literals;
  bool first_element = true;
  for (const auto& element : container) {
    if (true == first_element) {
      os << element;
      first_element = false;
    } else {
      os << ", "s << element;
    }
  }
  return os;
}

template<typename First, typename Second>
std::ostream& operator<<(std::ostream& os, const std::pair<First, Second>& container) {
  using namespace std::string_literals;
  os << container.first << ": "s << container.second;
  return os;
}

template<typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container) {
  using namespace std::string_literals;
  out << "{"s;
  Print(out, container);
  out << "}"s;
  return out;
}

template<typename Element>
std::ostream& operator<<(std::ostream& os, const std::set<Element>& container) {
  os << "{"s;
  Print(os, container);
  os << "}"s;
  return os;
}

template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& container) {
  os << "{"s;
  Print(os, container);
  os << "}"s;
  return os;
}

template<typename T>
void RunTestImpl(T t, const std::string& func) {
  t();
  std::cerr << func << " OK"s << std::endl;
}

template<typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
                     const std::string& file, const std::string& func, unsigned line,
                     const std::string& hint) {
  if (t != u) {
    std::cerr << std::boolalpha;
    std::cerr << file << "("s << line << "): "s << func << ": "s;
    std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
    std::cerr << t << " != "s << u << "."s;
    if (!hint.empty()) {
      std::cerr << " Hint: "s << hint;
    }
    std::cerr << std::endl;
    abort();
  }
}
