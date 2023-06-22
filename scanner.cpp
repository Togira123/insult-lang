#include "scanner.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace std;

const int N = 128;

ifstream file;
vector<token> token_list;

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
    bool ignore_next = c == '\\';
    while (c != EOF && (c != '"' || ignore_next)) {
        result += c;
        if (c == '\\' && !ignore_next) {
            ignore_next = true;
        } else {
            ignore_next = false;
        }
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
            buffer[i] = EOF;
            break;
        }
        file >> buffer[i];
    }
    char c = next_char(buffer, index, fence);

    int line = 1;
    while (c != EOF) {
        // do something
        string result;
        if ((result = whitespace(buffer, index, fence)) != "") {
            // store the result somehow
            cout << "(ws, " << result << ")\n";
            token_list.push_back({"ws", result});
        } else if ((result = newline(buffer, index, fence, line)) != "") {
            // strs
            cout << "(nl, " << result << ")\n";
            token_list.push_back({"nl", result});
        } else if ((result = double_token(buffer, index, fence)) != "") {
            // strs
            cout << "(double, " << result << ")\n";
            token_list.push_back({"double", result});
        } else if ((result = integer(buffer, index, fence)) != "") {
            // strs
            cout << "(int, " << result << ")\n";
            token_list.push_back({"int", result});
        } else if ((result = arithmetic_operator(buffer, index)) != "") {
            // strs
            cout << "(ao, " << result << ")\n";
            token_list.push_back({"ao", result});
        } else if ((result = logical_operator(buffer, index, fence)) != "") {
            // strs
            cout << "(log_op, " << result << ")\n";
            token_list.push_back({"log_op", result});
        } else if ((result = comparison_operator(buffer, index, fence)) != "") {
            // strs
            cout << "(comp_op, " << result << ")\n";
            token_list.push_back({"comp_op", result});
        } else if ((result = logicalnot(buffer, index)) != "") {
            // strs
            cout << "(log_not, " << result << ")\n";
            token_list.push_back({"log_not", result});
        } else if ((result = string_token(buffer, index, fence)) != "") {
            // strs
            cout << "(str, " << result << ")\n";
            token_list.push_back({"str", result});
        } else if ((result = identifier(buffer, index, fence)) != "") {
            // strs
            if (result == "true" || result == "false") {
                cout << "(bool, " << result << ")\n";
                token_list.push_back({"bool", result});
            } else if (result == "if" || result == "else" || result == "break" || result == "continue" || result == "continue") {
                cout << "(control, " << result << ")\n";
                token_list.push_back({"control", result});
            } else if (result == "for" || result == "while") {
                cout << "(iteration, " << result << ")\n";
                token_list.push_back({"iteration", result});
            } else if (result == "please" || result == "thanks" || result == "def" || result == "fun") {
                cout << "(gen_keyword, " << result << ")\n";
                token_list.push_back({"gen_keyword", result});
            } else {
                cout << "(identifier, " << result << ")\n";
                token_list.push_back({"identifier", result});
            }
        } else if ((result = data_type(buffer, index, fence)) != "") {
            // strs
            cout << "(data_type, " << result << ")\n";
            token_list.push_back({"data_type", result});
        } else if ((result = punctuation(buffer, index)) != "") {
            // strs
            cout << "(punc, " << result << ")\n";
            token_list.push_back({"punc", result});
        } else {
            // replace later with a random program
            throw runtime_error("unknown token at line " + to_string(line) + ": " + string(1, c) + "\n");
        }
        c = next_char(buffer, index, fence);
    }
    file.close();
    return token_list;
}
