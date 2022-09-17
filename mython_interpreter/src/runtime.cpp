#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
  bool result = false;

  auto ptr_str = object.TryAs<String>();
  if (ptr_str) {
    result = !ptr_str->GetValue().empty() ? true : false;
  }

  auto ptr_num = object.TryAs<Number>();
  if (ptr_num) {
    result = ptr_num->GetValue() != 0 ? true : false;
  }

  auto ptr_bool = object.TryAs<Bool>();
  if (ptr_bool) {
    return ptr_bool->GetValue();
  }

  return result;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
  if (this->HasMethod("__str__"s, 0)) {
    auto result = this->Call("__str__"s, {}, context);
    result.Get()->Print(os, context);
  } else {
    os << this;
  }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
  auto method_ptr = cls_.GetMethod(method);

  if (method_ptr != nullptr && method_ptr->formal_params.size() == argument_count)
    return true;

  return false;
}

Closure& ClassInstance::Fields() {
  return fields_;
}

const Closure& ClassInstance::Fields() const {
  return fields_;
}

ClassInstance::ClassInstance(const Class& cls) : cls_(cls) {
}

//  * Вызывает у объекта метод method, передавая ему actual_args параметров.
//  * Параметр context задаёт контекст для выполнения метода.
//  * Если ни сам класс, ни его родители не содержат метод method, метод выбрасывает исключение

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
  auto method_ptr = cls_.GetMethod(method);

  if (method_ptr != nullptr && method_ptr->formal_params.size() == actual_args.size()) {

    Closure temp_closure;
    temp_closure["self"] = ObjectHolder::Share(*this);

    for (int i = 0; i < static_cast<int>(actual_args.size()); ++i) {
      temp_closure[method_ptr->formal_params[i]] = actual_args[i];
    }



    auto result = method_ptr->body.get()->Execute(temp_closure, context);

    return result;
  }


  throw std::runtime_error("Not implemented"s);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
     {
  name_ = name;

  if (!methods.empty()) {
    for (auto& foo : methods) {
      Method method;
      method.name = foo.name;
      method.formal_params = foo.formal_params;

      method.body.swap(foo.body);

      methods_.push_back(std::move(method));
    }
  }

  if (parent != nullptr) {
    parent_ = parent;
  }
}

const Method* Class::GetMethod(const std::string& name) const {
  auto result = std::find_if(methods_.begin(), methods_.end(), [&name](const Method& lhs){return lhs.name == name;});

  if (result != methods_.end()) {
    return &(*result);
  } else if (parent_ != nullptr) {
    const Method* m_ptr = parent_->GetMethod(name);
    return m_ptr;
  }

  return nullptr;
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
  os << "Class "sv << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
  if (lhs.Get() == nullptr && rhs.Get() == nullptr)
    return true;

  if (lhs.TryAs<String>() != nullptr && rhs.TryAs<String>() != nullptr) {
    return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
  }

  if (lhs.TryAs<Number>() != nullptr && rhs.TryAs<Number>() != nullptr) {
    return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
  }

  if (lhs.TryAs<Bool>() != nullptr && rhs.TryAs<Bool>() != nullptr) {
    return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
  }

  if (lhs.TryAs<ClassInstance>() != nullptr) {
    auto ptr = lhs.TryAs<ClassInstance>();
    if (ptr->HasMethod("__eq__"s, 1)) {
      ObjectHolder result = ptr->Call("__eq__", {rhs}, context);

      if (result.TryAs<Bool>() != nullptr) {
        return result.TryAs<Bool>()->GetValue();
      }
    }

  }

  throw std::runtime_error("Cannot compare objects for equality"s);
}

//* Если lhs и rhs - числа, строки или значения bool, функция возвращает результат их сравнения
//* оператором <.
//* Если lhs - объект с методом __lt__, возвращает результат вызова lhs.__lt__(rhs),
//* приведённый к типу bool. В остальных случаях функция выбрасывает исключение runtime_error.
//*
//* Параметр context задаёт контекст для выполнения метода __lt__


bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
  if (lhs.TryAs<String>() != nullptr && rhs.TryAs<String>() != nullptr) {
    return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
  }

  if (lhs.TryAs<Number>() != nullptr && rhs.TryAs<Number>() != nullptr) {
    return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
  }

  if (lhs.TryAs<Bool>() != nullptr && rhs.TryAs<Bool>() != nullptr) {
    return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
  }

  if (lhs.TryAs<ClassInstance>() != nullptr) {
    auto ptr = lhs.TryAs<ClassInstance>();
    if (ptr->HasMethod("__lt__"s, 1)) {
      ObjectHolder result = ptr->Call("__lt__", {rhs}, context);

      if (result.TryAs<Bool>() != nullptr) {
        return result.TryAs<Bool>()->GetValue();
      }
    }

  }

  throw std::runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Equal(lhs, rhs, context));
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Less(lhs, rhs, context));
}

}  // namespace runtime
