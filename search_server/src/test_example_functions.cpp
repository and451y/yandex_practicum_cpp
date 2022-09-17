#include "test_example_functions.h"

constexpr double EPSILON = 1e-6;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file,
                const std::string& func, unsigned line, const std::string& hint) {
  if (!value) {
    std::cerr << file << "("s << line << "): "s << func << ": "s;
    std::cerr << "ASSERT("s << expr_str << ") failed."s;
    if (!hint.empty()) {
      std::cerr << " Hint: "s << hint;
    }
    std::cerr << std::endl;
    abort();
  }
}

void TestAddDocument() {
  const int doc_id = 0;
  const std::string content = "dog cat horse"s;
  const std::vector<int> ratings = {-2, 0, 3};

  SearchServer server(""s);
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto founded_docs = server.FindTopDocuments("cat"s);
  const Document& first_doc = founded_docs[0];

  ASSERT_HINT(founded_docs.size(), "Request not found"s);
  ASSERT_EQUAL(first_doc.id, doc_id);
}

void TestExcludeStopWordsFromAddedDocumentContent() {
  const int doc_id = 42;
  const std::string content = "cat in the city"s;
  const std::vector<int> ratings = {1, 2, 3};

  {
    SearchServer server("the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("the"s).empty(), "Stop-word is not excluded"s);
  }
}

void TestExcludeMinusWordsFromAddedDocumentContent() {
  const int doc_id = 463;
  const std::string content = "elephant in shop"s;
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server(""s);
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

  ASSERT(server.FindTopDocuments("elephant -shop"s).empty());
}

void TestMatchingDocument() {
  const int doc_id = 463;
  const std::string content = "elephant in shop"s;
  const std::vector<std::string_view> match_content = {"elephant"sv, "shop"sv};
  const std::vector<int> ratings = {1, 2, 3};

  {
    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    auto [match_words, status] = server.MatchDocument("elephant -shop"s, doc_id);
    ASSERT(match_words.empty());
  }

  {
    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    auto [match_words, status] = server.MatchDocument("elephant shop"sv, doc_id);
    ASSERT_EQUAL(match_words, match_content);
  }

  {
    SearchServer search_server("and with"s);

    int id = 0;
    for (const std::string& text : {"funny pet and nasty rat"s, "funny pet with curly hair"s,
        "funny pet and not very nasty rat"s, "pet with rat and rat and rat"s,
        "nasty rat with curly hair"s, }) {
      search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::string query = "curly and funny -not"s;

    {
      const auto [words, status] = search_server.MatchDocument(query, 1);
      ASSERT_EQUAL(words.size(), 1u);
    }

    {
      const auto [words, status] = search_server.MatchDocument(std::execution::seq, query, 2);
      ASSERT_EQUAL(words.size(), 2u);

    }

    {
      const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 3);
      ASSERT_EQUAL(words.size(), 0u);

    }

  }
}

void TestSortFindDocumentsByDescentRelevance() {
  const std::vector<int> ratings = {1, 2, 3};
  SearchServer server(""s);

  server.AddDocument(2, "cat dog horse"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(1, "cat cat dog dog"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(3, "dog horse horse"s, DocumentStatus::ACTUAL, ratings);
  const std::vector<Document> result = server.FindTopDocuments("cat dog"s);

  ASSERT_EQUAL(1, result.at(0).id);
  ASSERT_EQUAL(2, result.at(1).id);
  ASSERT_EQUAL(3, result.at(2).id);
}

void TestComputeDocumentRating() {
  const std::vector<int> ratings = {1, 2, 3};
  SearchServer server(""s);
  server.AddDocument(463, "cat dog"s, DocumentStatus::ACTUAL, ratings);

  const std::vector<Document> result = server.FindTopDocuments("cat dog"s);

  ASSERT_EQUAL_HINT(2, result.at(0).rating, "Incorrect calculation of ratings"s);
}

void TestUserPredicate() {
  const std::vector<int> ratings = {1, 2, 3};
  SearchServer server(""s);
  server.AddDocument(1, "cat dog"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(2, "cat dog"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(4, "cat dog"s, DocumentStatus::ACTUAL, ratings);

  const std::vector<Document> result = server.FindTopDocuments(
      "cat dog"s, [](int document_id, DocumentStatus status, int rating) {
        return document_id % 2 == 0;
      }
  );

  ASSERT_EQUAL(2, result.at(0).id);
  ASSERT_EQUAL(4, result.at(1).id);
}

void TestSearchSpecifiedStatusDoc() {
  const std::vector<int> ratings = {1, 2, 3};
  SearchServer server(""s);

  server.AddDocument(1, "dog"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(2, "cat dog"s, DocumentStatus::IRRELEVANT, ratings);
  server.AddDocument(3, "cat"s, DocumentStatus::REMOVED, ratings);
  server.AddDocument(4, "cat dog"s, DocumentStatus::BANNED, ratings);

  const std::vector<Document> result = server.FindTopDocuments("cat dog"s, DocumentStatus::BANNED);
  ASSERT_EQUAL_HINT(4, result.at(0).id, "Incorrect BANEND document id"s);

}

void TestComputeRelevance() {
  const std::vector<int> ratings = {1, 2, 3};
  SearchServer server(""s);

  server.AddDocument(2, "cat dog horse"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(1, "cat cat dog dog"s, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(3, "dog horse horse"s, DocumentStatus::ACTUAL, ratings);
  const std::vector<Document> result = server.FindTopDocuments("cat dog"s);

  ASSERT_HINT(std::abs(0.202733 - result.at(0).relevance) < EPSILON,
              "Incorrect calculation of relevance"s);
}

void TestRemoveDuplicates() {
  SearchServer server("a the on is"s);
  server.AddDocument(0, "this test"s, DocumentStatus::ACTUAL, {1});
  server.AddDocument(1, "this green crocodile"s, DocumentStatus::ACTUAL, {1});
  server.AddDocument(2, "this test"s, DocumentStatus::ACTUAL, {1});
  {
    ASSERT_EQUAL(3, server.GetDocumentCount());
    auto [words, status] = server.MatchDocument("test"s, 2);
    ASSERT(!words.empty());
  }
  RemoveDuplicates(server);
  {
    ASSERT_EQUAL(2, server.GetDocumentCount());
  }
}

void TestParrallelFindDoc() {
  SearchServer search_server("and with"s);
  std::vector<std::string> str = {"white cat and yellow hat"s,
      "curly cat curly tail"s,
      "nasty dog with big eyes"s,
      "nasty pigeon john"s};
  int id = 0;
  for (const std::string& text : str) {
    search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
  }

  std::cout << "ACTUAL by default:"s << std::endl;
  // последовательная версия
  for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
    PrintDocument(document);
  }
  std::cout << "BANNED:"s << std::endl;
  // последовательная версия
  for (const Document& document : search_server.FindTopDocuments(std::execution::seq, "curly nasty cat"s,
                                                                 DocumentStatus::BANNED)) {
    PrintDocument(document);
  }

  std::cout << "Even ids:"s << std::endl;
  // параллельная версия
  for (const Document& document : search_server.FindTopDocuments(
      std::execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) {
        return document_id % 2 == 0;
      })) {
    PrintDocument(document);
  }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
  RUN_TEST(TestAddDocument);
  RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
  RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
  RUN_TEST(TestMatchingDocument);
  RUN_TEST(TestSortFindDocumentsByDescentRelevance);
  RUN_TEST(TestComputeDocumentRating);
  RUN_TEST(TestUserPredicate);
  RUN_TEST(TestSearchSpecifiedStatusDoc);
  RUN_TEST(TestComputeRelevance);
  RUN_TEST(TestRemoveDuplicates);
  //TestParrallelFindDoc();
}
