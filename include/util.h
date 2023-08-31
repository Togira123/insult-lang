#ifndef HEADER_UTIL_H
#define HEADER_UTIL_H
#include "language.h"
#include <string>
#include <unordered_map>

// https://stackoverflow.com/a/25829233/14553195
inline std::string& ltrim(std::string& s) {
    const char* t = " \t\n\r\f\v";
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string& rtrim(std::string& s) {
    const char* t = " \t\n\r\f\v";
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline std::string& trim(std::string& s) { return ltrim(rtrim(s)); }

inline bool starts_with(const std::string& s, const std::string& check) { return s.rfind(check, 0) == 0; }

struct token {
    token_type name;
    std::string value;
};

enum class compiler_flag { OPTIMIZE, OUTPUT, FORBID_LIBRARY_NAMES };
#endif
