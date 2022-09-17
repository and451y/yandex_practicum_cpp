#include <optional>

#include "json_builder.h"

using namespace json;
using namespace std::literals;

DictValueContext Builder::Key(std::string key) {
  if (TopStackType() == NodeType::KEY || TopStackType() != NodeType::DICT)
    throw std::logic_error("key problem"s);

  Node::Value key_val(std::move(key));

  nodes_stack_.emplace_back(new Node(std::get<std::string>(key_val)));
  return DictValueContext(*this);
}

BaseContext Builder::Value(Node::Value value) {
  if (complete_object)
    throw std::logic_error("complete_object"s);

  if (*TopStackType() != NodeType::ARRAY && *TopStackType() != NodeType::KEY
      && TopStackType().has_value())
    throw std::logic_error("value problem"s);

  if (*TopStackType() == NodeType::ARRAY) {
    TopStack()->AsArray().emplace_back(Node(value));
  } else if (*TopStackType() == NodeType::KEY) {
    ParentTopStack()->AsDict().emplace(TopStack()->AsString(), Node(value));
    delete TopStack();
    nodes_stack_.pop_back();
  } else {
    root_.GetValueForSet() = (Node(value).GetValue());
  }

  if (nodes_stack_.empty())
    complete_object = true;

  return BaseContext(*this);
}

BaseContext Builder::Value(Node node) {
  return Value(node.GetValue());
}

DictItemContext Builder::StartDict() {
  if (complete_object)
    throw std::logic_error("complete_object"s);

  Node::Value dict(Dict {});
  Node* node_ptr = new Node(std::get<Dict>(dict));
  nodes_stack_.emplace_back(node_ptr);

  return DictItemContext(*this);
}

ArrayItemContext Builder::StartArray() {
  if (complete_object)
    throw std::logic_error("complete_object"s);

  Node::Value array(Array {});
  Node* node_ptr = new Node(std::get<Array>(array));
  nodes_stack_.emplace_back(node_ptr);

  return ArrayItemContext(*this);
}

BaseContext Builder::EndDict() {
  if (complete_object)
    throw std::logic_error("complete_object"s);

  if (*TopStackType() != NodeType::DICT)
    throw std::logic_error("dict problem"s);

  Node* current_dict = TopStack();
  nodes_stack_.pop_back();

  Value(current_dict->GetValue());

  delete current_dict;

  if (nodes_stack_.empty())
    complete_object = true;

  return BaseContext(*this);
}

BaseContext Builder::EndArray() {
  if (complete_object)
    throw std::logic_error("complete_object"s);

  if (*TopStackType() != NodeType::ARRAY)
    throw std::logic_error("array problem"s);

  Node* current_array = TopStack();
  nodes_stack_.pop_back();

  Value(current_array->GetValue());

  delete current_array;

  if (nodes_stack_.empty())
    complete_object = true;

  return BaseContext(*this);
}

json::Node Builder::Build() {
  if (!nodes_stack_.empty() || !complete_object)
    throw std::logic_error("build problem"s);

  return root_;
}

Node* Builder::ParentTopStack() {
  return *std::prev(nodes_stack_.end(), 2);
}

Node* Builder::TopStack() {
  return nodes_stack_.back();
}

std::optional<NodeType> Builder::TopStackType() const {
  if (nodes_stack_.empty())
    return std::nullopt;

  NodeType result;

  if (nodes_stack_.back()->IsArray()) {
    result = NodeType::ARRAY;
  } else if (nodes_stack_.back()->IsDict()) {
    result = NodeType::DICT;
  } else if (nodes_stack_.size() > 1 && nodes_stack_.back()->IsString()) {
    Node* parent_top = *std::prev(nodes_stack_.end(), 2);

    if (parent_top->IsDict()) {
      result = NodeType::KEY;
    } else {
      result = NodeType::VALUE;
    }
  } else {
    result = NodeType::VALUE;
  }

  return result;
}

DictValueContext BaseContext::Key(std::string key) {
  return builder_.Key(std::move(key));
}

DictItemContext BaseContext::StartDict() {
  return builder_.StartDict();
}

ArrayItemContext BaseContext::StartArray() {
  return builder_.StartArray();
}
