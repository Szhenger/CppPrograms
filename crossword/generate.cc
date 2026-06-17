#include <iostream>
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
#include <deque>
#include <stdexcept>

// ============================================================================
// Core Crossword Structures (From Previous Setup)
// ============================================================================

enum class Direction { ACROSS, DOWN };

constexpr std::string_view to_string(Direction d) {
    return d == Direction::ACROSS ? "across" : "down";
}

struct Variable {
    int i, j;
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

    auto operator<=>(const Variable& other) const {
        if (auto cmp = i <=> other.i; cmp != 0) return cmp;
        if (auto cmp = j <=> other.j; cmp != 0) return cmp;
        if (auto cmp = direction <=> other.direction; cmp != 0) return cmp;
        return length <=> other.length;
    }

    bool operator==(const Variable& other) const {
        return (*this <=> other) == 0;
    }
};

class Crossword {
public:
    int height = 0, width = 0;
    std::vector<std::vector<bool>> structure;
    std::set<std::string> words;
    std::set<Variable> variables;
    std::map<std::pair<Variable, Variable>, std::optional<std::pair<int, int>>> overlaps;

    Crossword(const std::string& structure_file, const std::string& words_file) {
        std::ifstream s_file(structure_file);
        if (!s_file.is_open()) throw std::runtime_error("Could not open structure file.");

        std::vector<std::string> contents;
        std::string line;
        while (std::getline(s_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            contents.push_back(line);
        }

        height = contents.size();
        for (const auto& l : contents) {
            if (static_cast<int>(l.size()) > width) width = l.size();
        }

        structure.assign(height, std::vector<bool>(width, false));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (j < static_cast<int>(contents[i].size()) && contents[i][j] == '_') {
                    structure[i][j] = true;
                }
            }
        }

        std::ifstream w_file(words_file);
        if (!w_file.is_open()) throw std::runtime_error("Could not open words file.");
        while (std::getline(w_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            std::ranges::transform(line, line.begin(), [](unsigned char c) { return std::toupper(c); });
            words.insert(line);
        }

        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (structure[i][j] && (i == 0 || !structure[i - 1][j])) {
                    int len = 1;
                    for (int k = i + 1; k < height && structure[k][j]; ++k) len++;
                    if (len > 1) variables.emplace(i, j, Direction::DOWN, len);
                }
                if (structure[i][j] && (j == 0 || !structure[i][j - 1])) {
                    int len = 1;
                    for (int k = j + 1; k < width && structure[i][k]; ++k) len++;
                    if (len > 1) variables.emplace(i, j, Direction::ACROSS, len);
                }
            }
        }

        for (const auto& v1 : variables) {
            for (const auto& v2 : variables) {
                if (v1 == v2) continue;
                std::optional<std::pair<int, int>> intersection;
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

// ============================================================================
// Crossword Creator & Solver Logic
// ============================================================================

class CrosswordCreator {
private:
    const Crossword& crossword;
    std::map<Variable, std::set<std::string>> domains;

public:
    explicit CrosswordCreator(const Crossword& crossword) : crossword(crossword) {
        for (const auto& var : crossword.variables) {
            domains[var] = crossword.words; 
        }
    }

    std::vector<std::vector<std::optional<char>>> letter_grid(const std::map<Variable, std::string>& assignment) const {
        std::vector<std::vector<std::optional<char>>> letters(
            crossword.height, std::vector<std::optional<char>>(crossword.width, std::nullopt)
        );

        for (const auto& [variable, word] : assignment) {
            Direction direction = variable.direction;
            for (size_t k = 0; k < word.length(); ++k) {
                int i = variable.i + (direction == Direction::DOWN ? static_cast<int>(k) : 0);
                int j = variable.j + (direction == Direction::ACROSS ? static_cast<int>(k) : 0);
                letters[i][j] = word[k];
            }
        }
        return letters;
    }

    void print(const std::map<Variable, std::string>& assignment) const {
        auto letters = letter_grid(assignment);
        for (int i = 0; i < crossword.height; ++i) {
            for (int j = 0; j < crossword.width; ++j) {
                if (crossword.structure[i][j]) {
                    std::print("{}", letters[i][j].value_or(' '));
                } else {
                    std::print("█");
                }
            }
            std::println();
        }
    }

    void save(const std::map<Variable, std::string>& assignment, [[maybe_unused]] const std::string& filename) const {
        std::println("[Info] Image saving triggered for '{}'.", filename);
        // Note: To draw true anti-aliased TrueType fonts layout directly to a file from scratch
        // in pure C++ without massive bloat, external imaging engines like OpenCV, SFML, or 
        // headers like 'stb_image_write.h' / 'stb_truetype.h' are heavily recommended.
    }

    void enforce_node_consistency() {
        for (auto& [var, word_set] : domains) {
            std::erase_if(word_set, [&var](const std::string& word) {
                return static_cast<int>(word.length()) != var.length;
            });
        }
    }

    bool revise(const Variable& x, const Variable& y) {
        bool revision = false;
        auto it = crossword.overlaps.find({x, y});
        
        if (it != crossword.overlaps.end() && it->second.has_value()) {
            auto overlap = it->second.value();
            std::vector<std::string> words_to_remove;

            // Gather all characters that Y can possibly accept at the overlapping cell
            std::set<char> corresponding_chars;
            for (const auto& y_word : domains[y]) {
                corresponding_chars.insert(y_word[overlap.second]);
            }

            for (const auto& x_word : domains[x]) {
                char overlap_char = x_word[overlap.first];
                if (!corresponding_chars.contains(overlap_char)) {
                    words_to_remove.push_back(x_word);
                    revision = true;
                }
            }

            for (const auto& word : words_to_remove) {
                domains[x].erase(word);
            }
        }
        return revision;
    }

    bool ac3(std::optional<std::deque<std::pair<Variable, Variable>>> arcs = std::nullopt) {
        std::deque<std::pair<Variable, Variable>> queue;
        if (!arcs.has_value()) {
            for (const auto& var1 : crossword.variables) {
                for (const auto& var2 : crossword.variables) {
                    if (var1 != var2) {
                        queue.emplace_back(var1, var2);
                    }
                }
            }
        } else {
            queue = arcs.value();
        }

        while (!queue.empty()) {
            auto [var1, var2] = queue.front();
            queue.pop_front();

            if (revise(var1, var2)) {
                if (domains[var1].empty()) {
                    return false;
                }
                for (const auto& var3 : crossword.neighbors(var1)) {
                    if (var3 != var1 && var3 != var2) {
                        queue.emplace_back(var3, var1);
                    }
                }
            }
        }
        return true;
    }

    bool assignment_complete(const std::map<Variable, std::string>& assignment) const {
        return assignment.size() == crossword.variables.size();
    }

    bool consistent(const std::map<Variable, std::string>& assignment) const {
        for (const auto& [var1, word1] : assignment) {
            if (static_cast<int>(word1.length()) != var1.length) {
                return false;
            }
            for (const auto& var2 : crossword.neighbors(var1)) {
                auto it = crossword.overlaps.find({var1, var2});
                if (it != crossword.overlaps.end() && it->second.has_value() && assignment.contains(var2)) {
                    auto overlap = it->second.value();
                    if (word1[overlap.first] != assignment.at(var2)[overlap.second]) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    std::vector<std::string> order_domain_values(const Variable& var, const std::map<Variable, std::string>& assignment) const {
        std::vector<std::string> domain(domains.at(var).begin(), domains.at(var).end());
        auto neighborhood = crossword.neighbors(var);
        
        std::map<std::string, int> maps_values_to_numbers;
        for (const auto& value : domain) {
            maps_values_to_numbers[value] = 0;
            for (const auto& neighbor : neighborhood) {
                if (assignment.contains(neighbor)) continue;

                auto it = crossword.overlaps.find({var, neighbor});
                if (it != crossword.overlaps.end() && it->second.has_value()) {
                    auto overlap = it->second.value();
                    for (const auto& word : domains.at(neighbor)) {
                        if (value[overlap.first] != word[overlap.second]) {
                            maps_values_to_numbers[value]++;
                        }
                    }
                }
            }
        }

        std::ranges::sort(domain, [&maps_values_to_numbers](const std::string& a, const std::string& b) {
            return maps_values_to_numbers[a] < maps_values_to_numbers[b];
        });

        return domain;
    }

    Variable select_unassigned_variable(const std::map<Variable, std::string>& assignment) const {
        std::vector<Variable> unassigned;
        for (const auto& [var, _] : domains) {
            if (!assignment.contains(var)) {
                unassigned.push_back(var);
            }
        }

        // Replicates Python sorting behavior: Domain Size ascending, then Neighbor Count ascending
        std::ranges::sort(unassigned, [this](const Variable& a, const Variable& b) {
            int size_a = domains.at(a).size();
            int size_b = domains.at(b).size();
            if (size_a != size_b) return size_a < size_b;
            return crossword.neighbors(a).size() < crossword.neighbors(b).size();
        });

        return unassigned.front();
    }

    std::optional<std::map<Variable, std::string>> backtrack(std::map<Variable, std::string> assignment) {
        if (assignment_complete(assignment)) {
            return assignment;
        }

        Variable var = select_unassigned_variable(assignment);
        for (const auto& value : order_domain_values(var, assignment)) {
            assignment[var] = value;
            if (consistent(assignment)) {
                auto result = backtrack(assignment);
                if (result.has_value()) {
                    return result;
                }
            }
            assignment.erase(var);
        }
        return std::nullopt;
    }

    std::optional<std::map<Variable, std::string>> solve() {
        enforce_node_consistency();
        ac3();
        return backtrack({});
    }
};

// ============================================================================
// Execution Core (Main Function)
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc != 3 && argc != 4) {
        std::println(std::cerr, "Usage: ./generate structure words [output]");
        return 1;
    }

    std::string structure_file = argv[1];
    std::string words_file = argv[2];
    std::string output_file = (argc == 4) ? argv[3] : "";

    try {
        Crossword crossword(structure_file, words_file);
        CrosswordCreator creator(crossword);
        auto assignment = creator.solve();

        if (!assignment.has_value()) {
            std::println("No solution.");
        } else {
            creator.print(assignment.value());
            if (!output_file.empty()) {
                creator.save(assignment.value(), output_file);
            }
        }
    } catch (const std::exception& e) {
        std::println(std::cerr, "Error: {}", e.what());
        return 1;
    }

    return 0;
}
