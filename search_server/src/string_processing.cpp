#include "string_processing.h"

using namespace std;

string sv_to_s(const string_view sv) {
  return {sv.data(), sv.size()};
}

void PrintDocument(const Document& document) {
  cout << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
       << document.relevance << ", "s << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
  cout << "{ "s << "document_id = "s << document_id << ", "s << "status = "s
       << static_cast<int>(status) << ", "s << "words ="s;
  for (const string& word : words) {
    cout << ' ' << word;
  }
  cout << "}"s << endl;
}

string ReadLine() {
  string s;
  getline(cin, s);
  return s;
}

int ReadLineWithNumber() {
  int result;
  cin >> result;
  ReadLine();
  return result;
}

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    const int64_t pos_end = str.npos;
    while (true) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str.substr(0) : str.substr(0, space));
        if (space == pos_end) {
            break;
        } else {
            str.remove_prefix(space+1);
        }
    }

    return result;
}

ostream& operator<<(ostream& out, const Document& document) {
  out << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s << document.relevance
      << ", "s << "rating = "s << document.rating << " }"s;
  return out;
}
