#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "json.h"

namespace json {

enum NodeType {
  DICT,
  ARRAY,
  VALUE,
  KEY
};
    

class BaseContext;
class DictValueContext;
class DictItemContext;
class ArrayItemContext;

class Builder {
public:

  Builder() = default;

  DictValueContext Key(std::string key);
  BaseContext Value(Node::Value value);
  BaseContext Value(Node node);
  DictItemContext StartDict();
  ArrayItemContext StartArray();
  BaseContext EndDict();
  BaseContext EndArray();
  Node Build();

private:
  Node root_;
  std::vector<Node*> nodes_stack_;
  bool complete_object = false;

  Node* ParentTopStack();
  Node* TopStack();

  std::optional<NodeType> TopStackType() const;
};


class BaseContext {
public:

  BaseContext(Builder& builder) : builder_(builder) {}

  BaseContext Value(Node::Value value) {return builder_.Value(std::move(value));}
  BaseContext Value(Node node) {return builder_.Value(std::move(node.GetValue()));}

  DictValueContext Key(std::string key);
  DictItemContext StartDict();
  ArrayItemContext StartArray();

  BaseContext EndDict() {return  builder_.EndDict();}
  BaseContext EndArray() {return builder_.EndArray();}
  Node Build() {return builder_.Build();}

  virtual ~BaseContext() = default;
private:
  Builder& builder_;
};

//За вызовом StartArray следует не Value, не StartDict, не StartArray и не EndArray. (убрать build, key, endDict)
class ArrayItemContext : public BaseContext {
 public:
	using BaseContext::BaseContext;


	ArrayItemContext(BaseContext base) : BaseContext(base) {}
  ArrayItemContext Value(Node node) { return BaseContext::Value(std::move(node.GetValue())); }
	ArrayItemContext Value(Node::Value value) { return BaseContext::Value(std::move(value)); }

	Node Build() = delete;
	DictValueContext Key(std::string key) = delete;
	BaseContext EndDict() = delete;
};

//За вызовом StartDict следует не Key и не EndDict.
class DictItemContext : public BaseContext {
public:
	using BaseContext::BaseContext;
	DictItemContext(BaseContext base) : BaseContext(base) {}

  BaseContext Value(Node::Value value) = delete;
  DictItemContext StartDict() = delete;
  ArrayItemContext StartArray() = delete;
  BaseContext EndArray() = delete;
  Node Build() = delete;
};

//Непосредственно после Key вызван не Value, не StartDict и не StartArray.
//После вызова Value, последовавшего за вызовом Key, вызван не Key и не EndDict.

class DictValueContext : public BaseContext {
public:
	using BaseContext::BaseContext;

	DictValueContext(BaseContext base) : BaseContext(base) {}
  DictItemContext Value(Node node) { return BaseContext::Value(std::move(node.GetValue())); }
	DictItemContext Value(Node::Value value) { return BaseContext::Value(std::move(value)); }

  DictValueContext Key(std::string key) = delete;
  BaseContext EndDict() = delete;
  BaseContext EndArray() = delete;
  Node Build() = delete;
};


}  // namespace json
