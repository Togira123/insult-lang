#include "language.h"
#include <string>

using namespace std;

// https://stackoverflow.com/a/25829233/14553195
inline string& ltrim(string& s) {
    const char* t = " \t\n\r\f\v";
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline string& rtrim(string& s) {
    const char* t = " \t\n\r\f\v";
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline string& trim(string& s) { return ltrim(rtrim(s)); }

struct token {
    token_type name;
    string value;
};
