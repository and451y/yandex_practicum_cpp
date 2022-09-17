#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    std::optional<double>& GetMutableCache() const;

    bool IsReferenced() const;

private:
    class Impl {
    public:
        using Value = std::variant<std::monostate, std::string, std::unique_ptr<FormulaInterface>>;

        Value& GetMutableValue() {
            return value_;
        }

        const Value& GetValue() const {
            return value_;
        }

    private:
        Value value_;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
    };

    class TextImpl : public Impl {
    public:
        TextImpl(const std::string& text) {
            GetMutableValue() = text;
        }
    };


    class FormulaImpl : public Impl {
    public:
        FormulaImpl(const std::string& text) {
            GetMutableValue() = ParseFormula(text);
        }
    };


    std::unique_ptr<Impl> impl_;
    mutable std::optional<double> cache_;
    Sheet& sheet_;
};

