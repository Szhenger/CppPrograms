// Implements a dictionary's functionality
// C++ translation of dictionary.c

#include "dictionary.h"

#include <algorithm>   // std::equal, std::transform
#include <array>
#include <cctype>      // std::tolower
#include <fstream>
#include <memory>      // std::unique_ptr
#include <string>

// ── internal linkage ────────────────────────────────────────────────────────
namespace
{

// Represents a node in the hash table.
// std::unique_ptr<Node> owns the next node, so the whole chain is freed
// automatically when the bucket is reset — no manual unload needed.
struct Node
{
    std::string            word;
    std::unique_ptr<Node>  next;
};

// Number of buckets in the hash table
constexpr unsigned int N = 28;

// Hash table: each bucket owns its singly-linked chain
std::array<std::unique_ptr<Node>, N> table;

// Number of words currently loaded
unsigned int word_count = 0;

} // namespace

// ── public API ──────────────────────────────────────────────────────────────

// Hashes word to a bucket index.
unsigned int hash(const std::string& word)
{
    unsigned long sum = 0;
    for (char c : word)
        sum += static_cast<unsigned char>(std::tolower(c));
    return static_cast<unsigned int>(sum % N);
}

// Returns true if word is in the dictionary, else false.
// Comparison is case-insensitive.
bool check(const std::string& word)
{
    for (const Node* ptr = table[hash(word)].get(); ptr != nullptr; ptr = ptr->next.get())
    {
        if (std::equal(word.begin(), word.end(),
                       ptr->word.begin(), ptr->word.end(),
                       [](char a, char b)
                       {
                           return std::tolower(static_cast<unsigned char>(a))
                               == std::tolower(static_cast<unsigned char>(b));
                       }))
        {
            return true;
        }
    }
    return false;
}

// Loads dictionary into memory; returns true on success, false on failure.
bool load(const std::string& dictionary)
{
    std::ifstream file(dictionary);
    if (!file)
        return false;

    std::string word;
    while (file >> word)
    {
        auto n    = std::make_unique<Node>();
        n->word   = word;

        unsigned int bucket = hash(word);
        n->next             = std::move(table[bucket]);   // prepend to chain
        table[bucket]       = std::move(n);
        ++word_count;
    }
    return true;
}

// Returns number of words loaded, or 0 if not yet loaded.
unsigned int size()
{
    return word_count;
}

// Unloads the dictionary from memory; returns true.
// Resetting each unique_ptr destructs the entire owned chain automatically.
bool unload()
{
    for (auto& bucket : table)
        bucket.reset();
    word_count = 0;
    return true;
}
