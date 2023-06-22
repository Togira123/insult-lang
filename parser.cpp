#include "scanner.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <vector>

using namespace std;

struct t {
    const token& next() { return token_list[++index]; }
    const token& current() { return token_list[index]; }
    t(vector<token>* token_list = nullptr) : token_list(*token_list) {}
    void init(vector<token>& list) { token_list = list; }

private:
    vector<token>& token_list;
    int index = 0;
};
t tokens;

inline bool newline() { return tokens.current().name == "newline"; }

inline bool newline_and_or_whitespace() { return }

int main(int argc, char* argv[]) {
    vector<token>& token_list = scan_program(argc, argv);
    tokens.init(token_list);
    if (newline_and_or_whitespace() /*||*/) {
    }
}
