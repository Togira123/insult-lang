#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace std;

const int N = 16;

const vector<string> reserved_words = {
    "true", "false", "int", "double", "string", "bool", "please", "def", "while", "for", "if", "else", "break", "continue", "return", "thanks", "fun",
};

char next_char(vector<char>& buffer, int& index, int& fence) {
    char c = buffer[index];
    // check for file end
    if (c == EOF) {
        return c;
    }
    index = (index + 1) % (2 * N);
    if (index % N == 0) {
        // fill buffer
        for (int i = 0; i < N; i++) {
            if (cin.eof()) {
                buffer[index + i] = EOF;
                break;
            }
            cin >> buffer[index + i];
        }
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

string newline(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '\n') {
        return "";
    }
    string result = "";
    result += cur_char(buffer, index);
    char c = next_char(buffer, index, fence);
    while (c == '\n') {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    return result;
}

string integer(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) != '-' && (cur_char(buffer, index) < '1' || cur_char(buffer, index) > '9')) {
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

string logical_operator(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == '|' && next_char(buffer, index, fence) == '|') {
        return "||";
    }
    rollback(index, fence);
    if (cur_char(buffer, index) == '&' && next_char(buffer, index, fence) == '&') {
        return "&&";
    }
    rollback(index, fence);
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
    rollback(index, fence);
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
    while (c == '_' || (cur_char(buffer, index) >= 'A' && cur_char(buffer, index) <= 'Z') ||
           (cur_char(buffer, index) >= 'a' && cur_char(buffer, index) <= 'z') || (c >= '0' && c <= '9')) {
        result += c;
        c = next_char(buffer, index, fence);
    }
    rollback(index, fence);
    // check if the resulting identifier matches any keyword
    for (const string& e : reserved_words) {
        if (result == e) {
            for (int i = 0; i < result.length() - 1; i++) {
                rollback(index, fence);
            }
            return "";
        }
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
            rollback(index, fence);
            break;
        }
        result += "[]";
    }
    return result;
}

string boolean(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == 't') {
        const vector<char> cs = {'r', 'u', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "true";
    }
    if (cur_char(buffer, index) == 'f') {
        const vector<char> cs = {'a', 'l', 's', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "false";
    }
    return "";
}

int main() {
    int index = 0;
    int fence = 0;
    cin >> noskipws;
    vector<char> buffer(N * 2);
    for (int i = 0; i < N; i++) {
        if (cin.eof()) {
            buffer[i] = EOF;
            break;
        }
        cin >> buffer[i];
    }
    char c = next_char(buffer, index, fence);

    while (c != EOF) {
        // do something
        string result;
        if ((result = whitespace(buffer, index, fence)) != "") {
            // store the result somehow
            cout << "(ws, " << result << ")\n";
        } else if ((result = newline(buffer, index, fence)) != "") {
            // strs
            cout << "(nl, " << result << ")\n";
        } else if ((result = double_token(buffer, index, fence)) != "") {
            // strs
            cout << "(double, " << result << ")\n";
        } else if ((result = integer(buffer, index, fence)) != "") {
            // strs
            cout << "(int, " << result << ")\n";
        }
        c = next_char(buffer, index, fence);
    }
}
