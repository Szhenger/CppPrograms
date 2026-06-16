#include <iostream>
#include <print>
#include <string>

int main()
{
    // Get name from user
    std::print("What's your name? ");
    
    std::string name;
    std::getline(std::cin, name);

    // Prints greeting to user
    std::println("hello, {}", name);
}
