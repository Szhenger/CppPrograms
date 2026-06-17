#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <optional>
#include <algorithm>
#include <format>
#include <print>
#include <ranges>
#include <cctype>
#include <stdexcept>

// Replaces the string constants with a strictly typed enum
enum class Direction {
    ACROSS,
    DOWN
};

constexpr std::string_view to_string(Direction d) {
    return d == Direction::ACROSS ? "across" : "down";
}

class Variable {
public:
    int i;
    int j;
    Direction direction;
    int length;
    std::vector<std::pair<int, int>> cells;

    Variable(int i, int j, Direction direction, int length)
        : i(i), j(j), direction(direction), length(length) {
        
        cells.reserve(length);
        for (int k = 0; k < length; ++k) {
            cells.emplace_back(
                i + (direction == Direction::DOWN ? k : 0),
                j + (direction == Direction::ACROSS ? k : 0)
            );
        }
    }

    // C++20/23 defaulted spaceship operator replaces Python's __eq__ and __hash__.
    // We explicitly write it to only compare the core identifying fields.
    auto operator<=>(const Variable& other) const {
        if (auto cmp = i <=> other.i; cmp != 0) return cmp;
        if (auto cmp = j <=> other.j; cmp != 0) return cmp;
        if (auto cmp = direction <=> other.direction; cmp != 0) return cmp;
        return length <=> other.length;
    }

    bool operator==(const Variable& other) const {
        return (*this <=> other) == 0;
    }

    // Equivalents to Python's __str__ and __repr__
    std::string to_string() const {
        return std::format("({}, {}) {} : {}", i, j, ::to_string(direction), length);
    }

    std::string to_repr() const {
        return std::format("Variable({}, {}, '{}', {})", i, j, ::to_string(direction), length);
    }
};

// Hook into std::format (and std::print) so we can directly print Variable objects
template <>
struct std::formatter<Variable> : std::formatter<std::string> {
    auto format(const Variable& v, format_context& ctx) const {
        return std::formatter<std::string>::format(v.to_string(), ctx);
    }
};

class Crossword {
public:
    int height = 0;
    int width = 0;
    std::vector<std::vector<bool>> structure;
    std::set<std::string> words;
    std::set<Variable> variables;

    // Maps a pair of Variables to either std::nullopt (no overlap) or a pair of intersection indices
    using OverlapMap = std::map<std::pair<Variable, Variable>, std::optional<std::pair<int, int>>>;
    OverlapMap overlaps;

    Crossword(const std::string& structure_file, const std::string& words_file) {
        // 1. Determine structure of crossword
        std::ifstream s_file(structure_file);
        if (!s_file.is_open()) {
            throw std::runtime_error("Could not open structure file.");
        }

        std::vector<std::string> contents;
        std::string line;
        while (std::getline(s_file, line)) {
            // Trim carriage returns to handle Windows CRLF files on Unix systems
            if (!line.empty() && line.back() == '\r') line.pop_back();
            contents.push_back(line);
        }

        height = contents.size();
        for (const auto& l : contents) {
            if (l.size() > width) width = l.size();
        }

        for (int i = 0; i < height; ++i) {
            std::vector<bool> row;
            for (int j = 0; j < width; ++j) {
                if (j >= contents[i].size()) {
                    row.push_back(false);
                } else if (contents[i][j] == '_') {
                    row.push_back(true);
                } else {
                    row.push_back(false);
                }
            }
            structure.push_back(row);
        }

        // 2. Save vocabulary list
        std::ifstream w_file(words_file);
        if (!w_file.is_open()) {
            throw std::runtime_error("Could not open words file.");
        }
        
        while (std::getline(w_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            
            // Convert string to uppercase using C++20 ranges
            std::ranges::transform(line, line.begin(), [](unsigned char c){ return std::toupper(c); });
            words.insert(line);
        }

        // 3. Determine variable set
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                
                // Vertical words
                bool starts_word_down = structure[i][j] && (i == 0 || !structure[i - 1][j]);
                if (starts_word_down) {
                    int length = 1;
                    for (int k = i + 1; k < height; ++k) {
                        if (structure[k][j]) length++;
                        else break;
                    }
                    if (length > 1) {
                        variables.emplace(i, j, Direction::DOWN, length);
                    }
                }

                // Horizontal words
                bool starts_word_across = structure[i][j] && (j == 0 || !structure[i][j - 1]);
                if (starts_word_across) {
                    int length = 1;
                    for (int k = j + 1; k < width; ++k) {
                        if (structure[i][k]) length++;
                        else break;
                    }
                    if (length > 1) {
                        variables.emplace(i, j, Direction::ACROSS, length);
                    }
                }
            }
        }

        // 4. Compute overlaps for each word
        for (const auto& v1 : variables) {
            for (const auto& v2 : variables) {
                if (v1 == v2) continue;

                std::optional<std::pair<int, int>> intersection;
                
                // Check cross-reference of coordinate pairs. Because crossing words can
                // mathematically only intersect at one cell, breaking early is safe.
                for (size_t idx1 = 0; idx1 < v1.cells.size(); ++idx1) {
                    for (size_t idx2 = 0; idx2 < v2.cells.size(); ++idx2) {
                        if (v1.cells[idx1] == v2.cells[idx2]) {
                            intersection = {static_cast<int>(idx1), static_cast<int>(idx2)};
                            break;
                        }
                    }
                    if (intersection) break;
                }
                
                overlaps[{v1, v2}] = intersection;
            }
        }
    }

    std::set<Variable> neighbors(const Variable& var) const {
        // Given a variable, return set of overlapping variables
        std::set<Variable> result;
        for (const auto& v : variables) {
            if (v == var) continue;
            
            auto it = overlaps.find({v, var});
            if (it != overlaps.end() && it->second.has_value()) {
                result.insert(v);
            }
        }
        return result;
    }
};
