#include "language.h"
#include <string>

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

struct token {
    token_type name;
    std::string value;
};
