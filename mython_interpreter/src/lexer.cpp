#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_(input) {
  NextToken();
}

const Token& Lexer::CurrentToken() const {
   return tokens_stream_.back();
}

void Lexer::DefineCurentLineIndentLevel(const string& lexem) {
  if (lexem.empty() && (tokens_stream_.empty() || (!tokens_stream_.empty() && tokens_stream_.back().Is<token_type::Newline>()))) {
    if (input_.peek() == '\n' || input_.peek() == '#') {
      current_line_indent_level_ = -1; // признак пустой строки или строки-коммента
    } else if (input_.peek() != ' ' || input_.eof()) {
      current_line_indent_level_ = 0;
    } else {
      current_line_indent_level_ = DefineLineIndentLevel(input_);
    }
  }
}

Token Lexer::NextToken() {
  Token token;

  char get;
  string lexem;

  while(true) {

    if (input_.peek() == '#') {

      if (!lexem.empty()) {
        token = LexemToToken(lexem);
        break;
      }



      while(input_.peek() != '\n' && !input_.eof()) { // пропускаем все что идет после #
        get = input_.get();
      }




      if (tokens_stream_.empty() || (!tokens_stream_.empty() && tokens_stream_.back().Is<token_type::Newline>())) {
        continue;
      } else {
        lexem += input_.get();
        token = LexemToToken(lexem);
        break;
      }

    }

    DefineCurentLineIndentLevel(lexem);

    if (current_line_indent_level_ == -1) { // если признак пустой строки, то get символ переноса и читаем дальше

      while(input_.peek() != '\n' && !input_.eof()) { // пропускаем все что идет после #
        get = input_.get();
      }

      get = input_.get();

      continue;
    }


    if (current_indent_level_ < current_line_indent_level_) {
        token = {token_type::Indent{}};
        ++current_indent_level_;
        lexem = " "s;
        break;
    } else if (current_indent_level_ > current_line_indent_level_) {
        token = {token_type::Dedent{}};
        --current_indent_level_;
        lexem = " "s; // need fix
        break;
    } else if (current_indent_level_ == current_line_indent_level_) {
      //do nothing, work on this indent level
    }

    if(IsLexemKeyWord(lexem)) {
      token = LexemToToken(lexem);
      break;
    }



    if ((input_.peek() == ' ' || input_.peek() == '\n' || input_.eof()) && !lexem.empty()) {
      token = LexemToToken(lexem);
      break;
    }

    get = input_.get();

    if (get == '\n' && tokens_stream_.empty())
      continue;

    if (lexem.empty() && (get == '0' || (get >= '1' && get <= '9'))) {
      input_.putback(get);
      lexem = ParseNumberLexem(input_);
      token = LexemToToken(lexem);
      break;
    }

    if (get == '"' || get == '\'') {
      input_.putback(get);
      lexem = ParseStringLexem(input_);
      token = LexemToToken(lexem);
      break;
    }

    if (get == '\n') {
      lexem += get;
      token = LexemToToken(lexem);
      break;
    }

    if (input_.eof() && !tokens_stream_.empty() && (tokens_stream_.back().Is<token_type::Eof>() || tokens_stream_.back().Is<token_type::Dedent>() || tokens_stream_.back().Is<token_type::Newline>())) {
      break;
    } else if (input_.eof() && !tokens_stream_.empty() && !tokens_stream_.back().Is<token_type::Newline>()) {
      lexem += '\n';
      token = LexemToToken(lexem);
      break;
    } else if (input_.eof()) {
      break;
    }

    if (get == '_' || (get >= 'A' && get <= 'Z') || (get >= 'a' && get <= 'z')) {
      input_.putback(get);
      lexem = GetIdLexem(input_);
      token = LexemToToken(lexem);
      break;
    }

    if ((get == '=' && input_.peek() == '=') ||
        (get == '>' && input_.peek() == '=') ||
        (get == '<' && input_.peek() == '=') ||
        (get == '!' && input_.peek() == '=')) {
      lexem += get;
      lexem += input_.get();
      token = LexemToToken(lexem);
      break;
    }

    if  (lexem.empty() && (
        !(get == '_' || get == ' ' || (get >= 'A' && get <= 'Z') || (get >= 'a' && get <= 'z') || get == '0' || (get >= '1' && get <= '9'))         )
        )
         {
      input_.putback(get);
      lexem = ParseCharLexem(input_);
      token = LexemToToken(lexem);
      break;
    }


    if (get != ' ') {
      lexem += get;
    }


  }

  if (lexem.empty())
    token = {token_type::Eof{}};

  tokens_stream_.push_back(token);

  return token;
}

int Lexer::DefineLineIndentLevel(istream& input) {
  string lexem;
  lexem = ParseIndentLexem(input);

  if (lexem == "#")
    return -1;

  int next_indent_level = (lexem.size() / 2);

  return next_indent_level;
}

std::string Lexer::ParseIndentLexem(istream& input) {
  string result;

  while (true) {
    if (input.peek() == '#') {
      return "#"s;
    }

    if (input.peek() != ' ')
      break;


    result += input.get();
  }

  return result;
}

std::string Lexer::ParseCharLexem(istream& input) {
  string result;
  result += input.get();
  return result;
}


std::string Lexer::GetIdLexem(istream& input) {
  string result;

  while (true) {
    if (!((input.peek() == '_' || (input.peek() >= 'A' && input.peek() <= 'Z') || (input.peek() >= 'a' && input.peek() <= 'z') || input.peek() == '0' || (input.peek() >= '1' && input.peek() <= '9')))) {
      break;
    }

    result += input.get();

  }

  return result;
}


std::string Lexer::ParseStringLexem(istream& input) {
  char c;
  string result;
  bool start = false;
  bool end = false;

  while (true) {

    if (start && end && &result.front() != &result.back()) {
      break;
    }

//    && (input.peek() == ' ' || input.peek() == '\n' || input_.eof())

    c = input.get();

    if (result.empty() && (c == '"' || c == '\'')) {
      result += c;
      start = true;
      continue;
    }

    if (c == '"') {
      result += '"';
      if (result.front() == '"')
        end = true;
    } else if (c == '\'') {
      result += '\'';
      if (result.front() == '\'')
        end = true;
    } else if (c == '\\') {
      c = input_.get();
      if (c == 'n')
        result += '\n';
      if (c == 't')
        result += '\t';
      if (c == 'r')
        result += '\r';
      if (c == '"')
        result += '"';
      if (c == '\\')
        result += '\\';
      if (c == '\'')
        result += '\'';
    } else {
      result += c;
    }

  }

  return result;
}


std::string Lexer::ParseNumberLexem(istream& input) {

  string result;

  while (true) {

    if (!(input.peek() == '0' || (input.peek() >= '1' && input.peek() <= '9'))) {
      break;
    }

    result += input.get();
  }

  return result;

}

Token Lexer::LexemToToken(std::string_view lexem) {

  Token result;

  // Лексема «число»

  if (lexem.front() == '0' || (lexem.front() >= '1' && lexem.front() <= '9')) {
    result = {token_type::Number{std::stoi(std::string(lexem))}};
  } else if (lexem.front() == '"' || lexem.front() == '\'') {
    lexem.remove_prefix(1);   // Лексема «строковая константа»
    lexem.remove_suffix(1);

    result = {token_type::String{std::string(lexem)}};
  } else if (lexem == "class"sv) {
    result = {token_type::Class{}};
  } else if (lexem == "return"sv) {
    result = {token_type::Return{}};
  } else if (lexem == "if"sv) {
    result = {token_type::If{}};
  } else if (lexem == "else"sv) {
    result = {token_type::Else{}};
  } else if (lexem == "def"sv) {

    result = {token_type::Def{}};
  } else if (lexem == "\n"sv) {

    result = {token_type::Newline{}};
  } else if (lexem == "print"sv) {

    result = {token_type::Print{}};
  } else if (lexem == "and"sv) {

    result = {token_type::And{}};
  } else if (lexem == "or"sv) {

    result = {token_type::Or{}};
  } else if (lexem == "not"sv) {
    result = {token_type::Not{}};

  } else if (lexem == "=="sv) {
    result = {token_type::Eq{}};

  } else if (lexem == "!="sv) {
    result = {token_type::NotEq{}};

  } else if (lexem == "<="sv) {
    result = {token_type::LessOrEq{}};

  } else if (lexem == ">="sv) {
    result = {token_type::GreaterOrEq{}};

  } else if (lexem == "None"sv) {
    result = {token_type::None{}};
  } else if (lexem == "True"sv) {
    result = {token_type::True{}};

  } else if (lexem == "False"sv) {
    result = {token_type::False{}};

  } else if (lexem.front() == '_' || (lexem.front() >= 'A' && lexem.front() <= 'Z') || (lexem.front() >= 'a' && lexem.front() <= 'z')) {
    result = {token_type::Id{std::string(lexem)}}; // Лексема «идентификатор»
  } else if (lexem.size() == 1) {  // Лексема «символ»
    result = {token_type::Char{lexem.front()}};
  }

  return result;

}

bool Lexer::IsLexemKeyWord(const std::string& lexem) {
  if (lexem == "class"s ||
      lexem == "return"s ||
      (lexem == "if"s) ||
      (lexem == "else"s) ||
      (lexem == "def"s) ||
      (lexem == "None"s) ||
      (lexem == "True"s) ||
      (lexem == "False"s) ||
      (lexem == "print"s) ||
      (lexem == "and"s) ||
      (lexem == "or"s) ||
      (lexem == "not"s)) {
    return true;
  }


      return false;

}



}  // namespace parse
