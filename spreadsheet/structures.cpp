#include "common.h"

#include <algorithm>
#include <cctype>
#include <sstream>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return NONE.col < col && col < MAX_COLS && NONE.row < row && row < MAX_ROWS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }

    int curr_col = col + 1;
    std::string revers;
    while (curr_col > 0) {
        revers.push_back('A' + (--curr_col) % LETTERS);
        curr_col /= LETTERS;
    }

    return std::string(revers.rbegin(), revers.rend()) + std::to_string(row + 1);
}

Position Position::FromString(std::string_view str) {
    size_t pos = 0;
    int curr_col = 0;
    for (; pos < str.size(); ++pos) {
        char ch = str[pos];
        if (ch >= 'A' && ch <= 'Z' && pos <= MAX_POS_LETTER_COUNT) {
            (curr_col *= LETTERS) += (ch - 'A') + 1;
        }
        else {
            if (isdigit(ch)) break;
            return Position::NONE;
        }
    }

    int curr_row = 0;
    if (!(pos > 0 && pos < str.size())) {
        return Position::NONE;
    }

    std::istringstream iss(std::string{ str.substr(pos) });
    if (!(iss >> curr_row) || curr_row > MAX_ROWS || !iss.eof()) {
        return Position::NONE;
    }

    return { curr_row - 1, curr_col - 1 };
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}
