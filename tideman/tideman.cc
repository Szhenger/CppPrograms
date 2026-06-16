#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm> // Required for std::sort
#include <limits>

// Max number of candidates
constexpr int MAX = 9;

// preferences[i][j] is number of voters who prefer i over j
int preferences[MAX][MAX] = {0};

// locked[i][j] means i is locked in over j
bool locked[MAX][MAX] = {false};

// Each pair has a winner, loser. Capitalized to avoid confusion with std::pair.
struct Pair
{
    int winner;
    int loser;
};

// Array of candidates and pairs
std::string candidates[MAX];
Pair pairs[MAX * (MAX - 1) / 2];

// Helper variables
int pair_count = 0;
int candidate_count = 0;

// Function prototypes
bool vote(int rank, std::string_view name, std::vector<int>& ranks);
void record_preferences(const std::vector<int>& ranks);
void add_pairs();
void sort_pairs();
void lock_pairs();
bool makes_cycle(int start, int end);
void print_winner();

// Safe input helper
int get_voter_count();

int main(int argc, char* argv[])
{
    // Check for invalid usage
    if (argc < 2)
    {
        std::println("Usage: tideman [candidate ...]");
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
        candidates[i] = argv[i + 1];
    }

    // Note: Because we initialized the global preferences and locked 
    // arrays with {0} and {false} at the top of the file, we no longer 
    // need the nested loops here to clear them!

    int voter_count = get_voter_count();

    // Query for votes
    for (int i = 0; i < voter_count; i++)
    {
        // Using std::vector instead of a Variable Length Array (VLA)
        std::vector<int> ranks(candidate_count);

        // Query for each rank
        for (int j = 0; j < candidate_count; j++)
        {
            std::print("Rank {}: ", j + 1);
            
            std::string name;
            // std::ws clears any leftover newline spaces in the input buffer
            std::getline(std::cin >> std::ws, name); 

            // Pass the vector directly
            if (!vote(j, name, ranks))
            {
                std::println("Invalid vote.");
                return 3;
            }
        }

        record_preferences(ranks);
        std::println(); // Prints a blank line
    }

    add_pairs();
    sort_pairs();
    lock_pairs();
    print_winner();
    return 0;
}

// Safely grabs an integer
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

// Update ranks given a new vote (takes vector by reference using &)
bool vote(int rank, std::string_view name, std::vector<int>& ranks)
{
    for (int i = 0; i < candidate_count; i++)
    {
        if (name == candidates[i])
        {
            ranks[rank] = i;
            return true;
        }
    }
    return false;
}

// Update preferences given one voter's ranks (takes vector by const reference)
void record_preferences(const std::vector<int>& ranks)
{
    for (int i = 0; i < candidate_count; i++)
    {
        for (int j = i + 1; j < candidate_count; j++)
        {
            preferences[ranks[i]][ranks[j]]++;
        }
    }
}

// Record pairs of candidates where one is preferred over the other
void add_pairs()
{
    for (int i = 0; i < candidate_count; i++)
    {
        for (int j = i; j < candidate_count; j++)
        {
            if (preferences[i][j] > preferences[j][i])
            {
                pairs[pair_count].winner = i;
                pairs[pair_count].loser = j;
                pair_count++;
            }
            else if (preferences[i][j] < preferences[j][i])
            {
                pairs[pair_count].winner = j;
                pairs[pair_count].loser = i;
                pair_count++;
            }
        }
    }
}

// Sort pairs in decreasing order by strength of victory
void sort_pairs()
{
    // Modern C++ removes the need for manual, error-prone sorting loops!
    std::sort(pairs, pairs + pair_count, [](const Pair& a, const Pair& b) {
        return preferences[a.winner][a.loser] > preferences[b.winner][b.loser];
    });
}

// Lock pairs into the candidate graph in order, without creating cycles
void lock_pairs()
{
    for (int i = 0; i < pair_count; i++)
    {
        if (!makes_cycle(pairs[i].winner, pairs[i].loser))
        {
            locked[pairs[i].winner][pairs[i].loser] = true;
        }
    }
}

// Checks whether pairs of candidates makes a cycle
bool makes_cycle(int start, int end)
{
    if (start == end)
    {
        return true;
    }
    else
    {
        for (int i = 0; i < candidate_count; i++)
        {
            if (locked[end][i])
            {
                if (makes_cycle(start, i))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// Print the winner of the election
void print_winner()
{
    // C++ vector initialized with 'candidate_count' elements, all set to 0
    std::vector<int> candidate_edges(candidate_count, 0);

    for (int i = 0; i < candidate_count; i++)
    {
        for (int j = 0; j < candidate_count; j++)
        {
            if (locked[j][i])
            {
                candidate_edges[i]++;
            }
        }
    }

    int min_edges = candidate_count;

    for (int i = 0; i < candidate_count; i++)
    {
        if (candidate_edges[i] < min_edges)
        {
            min_edges = candidate_edges[i];
        }
    }

    for (int i = 0; i < candidate_count; i++)
    {
        if (candidate_edges[i] == min_edges)
        {
            std::println("{}", candidates[i]);
            break;
        }
    }
}
