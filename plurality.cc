#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <limits>

// Max number of candidates
constexpr int MAX = 9;

// Candidates have name and vote count. Capitalized to follow C++ conventions.
struct Candidate
{
    std::string name;
    int votes = 0; // Modern C++ allows default initialization right here!
};

// Array of candidates
Candidate candidates[MAX];

// Number of candidates
int candidate_count;

// Function prototypes
bool vote(std::string_view name);
void print_winner();

// Safe input helper
int get_voter_count();

int main(int argc, char* argv[])
{
    // Check for invalid usage
    if (argc < 2)
    {
        std::println("Usage: plurality [candidate ...]");
        return 1;
    }

    // Populate array of candidates
    candidate_count = argc - 1;
    if (candidate_count > MAX)
    {
        std::println("Maximum number of candidates is {}", MAX);
        return 2;
    }
    
    for (int i = 0; i < candidate_count; i++)
    {
        candidates[i].name = argv[i + 1];
        // Note: We no longer need to explicitly set candidates[i].votes = 0 
        // because the struct automatically initializes it!
    }

    int voter_count = get_voter_count();

    // Loop over all voters
    for (int i = 0; i < voter_count; i++)
    {
        std::print("Vote: ");
        std::string name;
        
        // std::ws clears leftover whitespace/newlines from the input stream
        std::getline(std::cin >> std::ws, name);

        // Check for invalid vote
        if (!vote(name))
        {
            std::println("Invalid vote.");
        }
    }

    // Display winner of election
    print_winner();
    return 0;
}

// Safely grabs an integer for voter count
int get_voter_count()
{
    int count;
    while (true)
    {
        std::print("Number of voters: ");
        if (std::cin >> count && count > 0)
        {
            return count;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

// Update vote totals given a new vote
bool vote(std::string_view name)
{
    for (int i = 0; i < candidate_count; i++)
    {
        // No more strcmp! C++ strings can be directly compared using ==
        if (name == candidates[i].name)
        {
            candidates[i].votes++;
            return true;
        }
    }
    return false;
}

// Print the winner (or winners) of the election
void print_winner()
{
    int max_
