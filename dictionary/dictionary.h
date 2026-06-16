// C++ header — drop-in replacement for the original dictionary.h

#pragma once
#include <string>

// Maximum length for a word in the dictionary
constexpr int LENGTH = 45;

bool         check(const std::string& word);
unsigned int hash (const std::string& word);
bool         load (const std::string& dictionary);
unsigned int size ();
bool         unload();
