// Implement a program that identifies to whom a sequence of DNA belongs
// C++23 translation of dna.py

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

// A database row maps column names to either a name (string) or an STR count (int).
using FieldValue = std::variant<std::string, int>;
using Row        = std::map<std::string, FieldValue>;

// ---------------------------------------------------------------------------
// longest_match
//   Returns the length of the longest consecutive run of `subsequence`
//   found inside `sequence`.
// ---------------------------------------------------------------------------
int longest_match(const std::string& sequence, const std::string& subsequence)
{
    int longest_run       = 0;
    const int subseq_len  = static_cast<int>(subsequence.size());
    const int seq_len     = static_cast<int>(sequence.size());

    for (int i = 0; i < seq_len; ++i)
    {
        int count = 0;
        while (true)
        {
            int start = i + count * subseq_len;
            int end   = start + subseq_len;

            if (end <= seq_len &&
                sequence.substr(start, subseq_len) == subsequence)
            {
                ++count;
            }
            else
            {
                break;
            }
        }
        longest_run = std::max(longest_run, count);
    }
    return longest_run;
}

// ---------------------------------------------------------------------------
// parse_csv
//   Reads a CSV file and returns a vector of rows.  Each row maps header
//   names to FieldValue (int when the cell is purely numeric, string otherwise).
// ---------------------------------------------------------------------------
std::vector<Row> parse_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Error: cannot open file '" << filename << "'\n";
        std::exit(1);
    }

    std::vector<Row>         database;
    std::vector<std::string> headers;
    std::string              line;

    // ---- header row --------------------------------------------------------
    if (!std::getline(file, line))
        return database;

    {
        std::istringstream ss(line);
        std::string        field;
        while (std::getline(ss, field, ','))
            headers.push_back(field);
    }

    // ---- data rows ---------------------------------------------------------
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string        field;
        Row                row;
        int                col = 0;

        while (std::getline(ss, field, ','))
        {
            if (col >= static_cast<int>(headers.size()))
                break;

            // Mirror Python's try/except int() conversion.
            try
            {
                std::size_t pos;
                int value = std::stoi(field, &pos);
                if (pos == field.size())   // entire field is numeric
                    row[headers[col]] = value;
                else
                    row[headers[col]] = field;
            }
            catch (...)
            {
                row[headers[col]] = field;
            }
            ++col;
        }
        database.push_back(std::move(row));
    }
    return database;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Check for command-line usage
    if (argc != 3)
    {
        std::cerr << "Usage: dna data.csv sequence.txt\n";
        return 1;
    }

    // Read database file
    std::vector<Row> database = parse_csv(argv[1]);
    if (database.empty())
    {
        std::cerr << "Error: database is empty.\n";
        return 1;
    }

    // Read DNA sequence file into a string
    std::ifstream seq_file(argv[2]);
    if (!seq_file)
    {
        std::cerr << "Error: cannot open file '" << argv[2] << "'\n";
        return 1;
    }
    std::string sequence(std::istreambuf_iterator<char>(seq_file),
                         std::istreambuf_iterator<char>{});

    // Find longest match of each STR in DNA sequence.
    // STR columns are those whose first-row value is an int.
    std::map<std::string, int> sequence_data;
    for (const auto& [header, value] : database[0])
    {
        if (std::holds_alternative<int>(value))
            sequence_data[header] = longest_match(sequence, header);
    }

    // Check database for matching profiles
    for (const auto& profile : database)
    {
        bool match = std::ranges::all_of(sequence_data,
            [&](const auto& kv) -> bool
            {
                const auto& [header, expected] = kv;
                auto it = profile.find(header);
                if (it == profile.end()) return false;
                const auto* actual = std::get_if<int>(&it->second);
                return actual && *actual == expected;
            });

        if (match)
        {
            if (auto it = profile.find("name"); it != profile.end())
                std::cout << std::get<std::string>(it->second) << '\n';
            return 0;
        }
    }

    std::cout << "No match\n";
    return 0;
}
