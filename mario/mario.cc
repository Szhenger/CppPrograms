#include <iostream>
#include <print>
#include <limits> // Needed to clear bad input streams

// Prototypes of Helper Functions
int get_height();
void make_pyramid(int n);

int main()
{
    // Get height of the desired pyramid between 1 and 8
    int height = get_height();

    // Prints the desired pyramid
    make_pyramid(height);
}

// Returns an integer between 1 and 8
int get_height()
{
    int i;
    do
    {
        std::print("Height: ");
        
        // Read input and check if it successfully parsed as an integer
        if (!(std::cin >> i))
        {
            // If it failed (e.g., user typed "apple"), clear the error state
            std::cin.clear();
            // Discard the garbage input up to the newline character
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            // Set i to an invalid number so the loop repeats
            i = 0; 
        }
    }
    while (i < 1 || i > 8);
    
    return i;
}

// Prints the desired pyramid of height n
void make_pyramid(int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n + (i + 3); j++)
        {
            if (j < n - (i + 1) || j > n + (i + 2) || j == n || j == n + 1)
            {
                std::print(" ");
            }
            else
            {
                std::print("#");
            }
        }
        std::println(); // C++23 equivalent of printf("\n")
    }
}
