#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    Sheet() {
        empty_cell_ = std::move(std::make_unique<Cell>(*this));
        empty_cell_.get()->Set("");
    }

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CheckPrintSize();
    void ClearCache();
    bool CheckCircularRef(Position check_pos, const CellInterface* cell) const;

    struct PosHasher
    {
        // noexcept is recommended, but not required
        std::size_t operator()(const Position& pos) const noexcept
        {
            return std::hash<int>{}(pos.col + pos.row);
        }
    };

    std::unordered_map<Position, std::unique_ptr<Cell>, PosHasher> sheet_;
	Size print_size_ = {0, 0};
    std::unique_ptr<Cell> empty_cell_;
};
