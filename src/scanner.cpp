#include "../include/scanner.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace std;

const int N = 128;

ifstream file;
vector<token> token_list = {{END_OF_INPUT, ""}};

// makes sure the buffer is only filled when necessary
bool last_fill_at_zero = true;

char next_char(vector<char>& buffer, int& index, int& fence) {
    char c = buffer[index];
    index = (index + 1) % (2 * N);
    // check for file end
    if (c == EOF) {
        return c;
    }
    if ((index == 0 && !last_fill_at_zero) || (index == N && last_fill_at_zero)) {
        // fill buffer
        for (int i = 0; i < N; i++) {
            if (file.eof()) {
                buffer[index + i - 1] = EOF;
                break;
            }
            file >> buffer[index + i];
        }
        last_fill_at_zero = index == 0;
        fence = (index + N) % (2 * N);
    }
    return c;
}

char cur_char(vector<char>& buffer, int index) {
    if (index == 0) {
        return buffer[N * 2 - 1];
    }
    return buffer[index - 1];
}

void rollback(int& index, int& fence) {
    if (index == fence) {
        throw runtime_error("Cannot rollback");
    }
    if (index == 0) {
        index = 2 * N - 1;
    } else {
        index--;
    }
}

string whitespace(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != ' ' && cur_char(buffer, index) != '\t') {
        return "";
    }
    string result = "";
    result += cur_char(buffer, index);
    char c = next_char(buffer, index, fence);
    while (c == ' ' || c == '\t') {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    return result;
}

string newline(vector<char>& buffer, int& index, int& fence, int& line) {
    if (cur_char(buffer, index) != '\n') {
        return "";
    }
    string result = "";
    result += cur_char(buffer, index);
    line++;
    char c = next_char(buffer, index, fence);
    while (c == '\n') {
        line++;
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    return result;
}

string integer(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '-' && (cur_char(buffer, index) < '0' || cur_char(buffer, index) > '9')) {
        return "";
    }
    string result = "";
    result += cur_char(buffer, index);
    char c = next_char(buffer, index, fence);
    while (c >= '0' && c <= '9') {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    if (result == "-") {
        return "";
    }
    return result;
}

string double_token(vector<char>& buffer, int& index, int& fence) {
    string result = "";
    if ((result = integer(buffer, index, fence)) == "") {
        return "";
    }
    char c = next_char(buffer, index, fence);
    if (c != '.') {
        for (int i = 0; i < (int)result.length(); i++) {
            rollback(index, fence);
        }
        return "";
    }
    result += ".";
    c = next_char(buffer, index, fence);
    if (c < '0' || c > '9') {
        for (int i = 0; i < (int)result.length() + 1; i++) {
            rollback(index, fence);
        }
        return "";
    }
    while (c >= '0' && c <= '9') {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    return result;
}

string arithmetic_operator(vector<char>& buffer, int& index) {
    switch (cur_char(buffer, index)) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
        return string(1, cur_char(buffer, index));
    default:
        return "";
    }
}

string comparison_operator(vector<char>& buffer, int& index, int& fence) {
    switch (cur_char(buffer, index)) {
    case '=': {
        if (next_char(buffer, index, fence) == '=') {
            return "==";
        } else {
            rollback(index, fence);
            return "";
        }
    }
    case '>': {
        if (next_char(buffer, index, fence) == '=') {
            return ">=";
        } else {
            rollback(index, fence);
            return ">";
        }
    }
    case '<': {
        if (next_char(buffer, index, fence) == '=') {
            return "<=";
        } else {
            rollback(index, fence);
            return "<";
        }
    }
    case '!': {
        if (next_char(buffer, index, fence) == '=') {
            return "!=";
        } else {
            rollback(index, fence);
            return "";
        }
    }
    default:
        return "";
    }
}

string logical_operator(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == '|') {
        if (next_char(buffer, index, fence) == '|') {
            return "||";
        } else {
            rollback(index, fence);
        }
    }
    if (cur_char(buffer, index) == '&') {
        if (next_char(buffer, index, fence) == '&') {
            return "&&";
        } else {
            rollback(index, fence);
        }
    }
    return "";
}

string logicalnot(vector<char>& buffer, int& index) { return (cur_char(buffer, index) == '!') ? "!" : ""; }

string string_token(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '"') {
        return "";
    }
    char c = next_char(buffer, index, fence);
    string result = "";
    bool ignore_next = false;
    while (c != EOF && (c != '"' || ignore_next)) {
        if (c == '\\' && !ignore_next) {
            ignore_next = true;
        } else {
            ignore_next = false;
        }
        result += c;
        c = next_char(buffer, index, fence);
    }
    if (c == EOF) {
        return "";
    }
    return result;
}

string identifier(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '_' &&
        (cur_char(buffer, index) < 'A' || (cur_char(buffer, index) > 'Z' && cur_char(buffer, index) < 'a') || cur_char(buffer, index) > 'z')) {
        return "";
    }
    string result = "";
    result += cur_char(buffer, index);
    char c = next_char(buffer, index, fence);
    while (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    if (result == "int") {
        rollback(index, fence);
        rollback(index, fence);
        return "";
    } else if (result == "double") {
        for (int i = 0; i < 5; i++) {
            rollback(index, fence);
        }
        return "";
    } else if (result == "string") {
        for (int i = 0; i < 5; i++) {
            rollback(index, fence);
        }
        return "";
    } else if (result == "bool") {
        for (int i = 0; i < 3; i++) {
            rollback(index, fence);
        }
        return "";
    }
    return result;
}

string data_type(vector<char>& buffer, int& index, int& fence) {
    string result = "";
    if (cur_char(buffer, index) == 'i') {
        const vector<char> cs = {'n', 't'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        result = "int";
    } else if (cur_char(buffer, index) == 'd') {
        const vector<char> cs = {'o', 'u', 'b', 'l', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        result = "double";
    } else if (cur_char(buffer, index) == 's') {
        const vector<char> cs = {'t', 'r', 'i', 'n', 'g'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        result = "string";
    } else if (cur_char(buffer, index) == 'b') {
        const vector<char> cs = {'o', 'o', 'l'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        result = "bool";
    } else {
        return "";
    }
    while (next_char(buffer, index, fence) == '[') {
        if (next_char(buffer, index, fence) != ']') {
            rollback(index, fence);
            break;
        }
        result += "[]";
    }
    rollback(index, fence);
    return result;
}

string punctuation(vector<char>& buffer, int& index) {
    string result = "";
    switch (cur_char(buffer, index)) {
    case '=':
    case ':':
    case '(':
    case ')':
    case '{':
    case '}':
    case ';':
    case ',':
    case '[':
    case ']':
        return string(1, cur_char(buffer, index));
    default:
        return "";
    }
}

string comment(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '/') {
        return "";
    }
    if (next_char(buffer, index, fence) == '/') {
        string result = "//";
        char c = next_char(buffer, index, fence);
        while (c != '\n' && c != EOF) {
            result += c;
            c = next_char(buffer, index, fence);
        }
        rollback(index, fence);
        return result;
    } else if (cur_char(buffer, index) == '*') {
        string result = "/*";
        char c = next_char(buffer, index, fence);
        while (c != EOF) {
            result += c;
            if (c == '*') {
                if ((c = next_char(buffer, index, fence)) == '/') {
                    return result + '/';
                }
            } else {
                c = next_char(buffer, index, fence);
            }
        }
        rollback(index, fence);
        // it's only a comment if it really ends with "*/"
        return "";
    } else {
        rollback(index, fence);
        return "";
    }
}

vector<token>& scan_program(int argc, char* argv[]) {
    if (argc < 2) {
        throw runtime_error("Specify program to compile!");
    }
    file = ifstream(argv[1]);
    if (!file.good()) {
        file.close();
        throw runtime_error("There was an error trying to compile the file!");
    }
    int index = 0;
    int fence = 0;
    file >> noskipws;
    vector<char> buffer(N * 2);
    for (int i = 0; i < N; i++) {
        if (file.eof()) {
            buffer[i - 1] = EOF;
            break;
        }
        file >> buffer[i];
    }
    char c = next_char(buffer, index, fence);

    int line = 1;
    while (c != EOF) {
        string result;
        if ((result = newline(buffer, index, fence, line)) != "") {
            token_list.push_back({NEWLINE, result});
        } else if ((result = comment(buffer, index, fence)) != "") {
            // scanned comment, do not add to token_list
        } else if ((result = double_token(buffer, index, fence)) != "") {
            token_list.push_back({DOUBLE, result});
        } else if ((result = integer(buffer, index, fence)) != "") {
            token_list.push_back({INT, result});
        } else if ((result = arithmetic_operator(buffer, index)) != "") {
            token_list.push_back({ARITHMETIC_OPERATOR, result});
        } else if ((result = logical_operator(buffer, index, fence)) != "") {
            token_list.push_back({LOGICAL_OPERATOR, result});
        } else if ((result = comparison_operator(buffer, index, fence)) != "") {
            token_list.push_back({COMPARISON_OPERATOR, result});
        } else if ((result = logicalnot(buffer, index)) != "") {
            token_list.push_back({LOGICAL_NOT, result});
        } else if ((result = string_token(buffer, index, fence)) != "") {
            token_list.push_back({STRING, result});
        } else if ((result = identifier(buffer, index, fence)) != "") {
            if (result == "true" || result == "false") {
                token_list.push_back({BOOL, result});
            } else if (result == "if" || result == "else" || result == "break" || result == "continue" || result == "return") {
                token_list.push_back({CONTROL, result});
            } else if (result == "for" || result == "while") {
                token_list.push_back({ITERATION, result});
            } else if (result == "please" || result == "thanks" || result == "def" || result == "fun") {
                token_list.push_back({GENERAL_KEYWORD, result});
            } else {
                token_list.push_back({IDENTIFIER, result});
            }
        } else if ((result = data_type(buffer, index, fence)) != "") {
            token_list.push_back({DATA_TYPE, result});
        } else if ((result = punctuation(buffer, index)) != "") {
            token_list.push_back({PUNCTUATION, result});
        } else if ((result = whitespace(buffer, index, fence)) != "") {
            // whitespace – ignore
        } else {
            // replace later with a random program
            throw runtime_error("unknown token at line " + to_string(line) + ": " + string(1, c) + "\n");
        }
        c = next_char(buffer, index, fence);
    }
    file.close();
    token_list.push_back({END_OF_INPUT, ""});
    return token_list;
}
