#include <iostream>
#include <print>
#include <limits>
#include <string_view> // Used for lightweight string passing

// Prototype for our robust input helper
long get_long(std::string_view prompt);

int main()
{
    // Get number
    long number = get_long("Number: ");

    // Implementation of Luhn's Algorithm

    // Initialize checksum, length, and startingdigits
    int checksum = 0;
    int length = 0;
    int startingdigits = 0;
    int digit = 0;

    // Compute checksum, length, and startingdigits of number
    while (number > 0)
    {
        if (startingdigits == 0 && number < 100)
        {
            startingdigits = number;
        }
        digit = number % 10;
        number = number / 10;
        length++;
        
        if (length % 2 == 0)
        {
            if (2 * digit > 9)
            {
                checksum += ((2 * digit) / 10 + (2 * digit) % 10);
            }
            else
            {
                checksum += 2 * digit;
            }
        }
        else
        {
            checksum += digit;
        }
    }

    // Check number and bank
    if (checksum % 10 != 0)
    {
        std::println("INVALID");
    }
    else if (length == 13 || length == 16)
    {
        if (length == 16 && startingdigits > 50 && startingdigits < 56)
        {
            std::println("MASTERCARD");
        }
        else if (startingdigits / 10 == 4)
        {
            std::println("VISA");
        }
        else
        {
            std::println("INVALID");
        }
    }
    else if (length == 15)
    {
        if (startingdigits == 34 || startingdigits == 37)
        {
            std::println("AMEX");
        }
        else
        {
            std::println("INVALID");
        }
    }
    else
    {
        std::println("INVALID");
    }
}

// Safely gets a long integer from the user, rejecting bad input
long get_long(std::string_view prompt)
{
    long value;
    while (true)
    {
        // Print the prompt
        std::print("{}", prompt);
        
        // Attempt to read a long
        if (std::cin >> value)
        {
            // If successful, break out of the infinite loop
            break; 
        }
        
        // If the user typed non-numeric characters, clear the error state
        std::cin.clear();
        // Discard the garbage input up to the newline
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return value;
}
