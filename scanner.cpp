#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace std;

const int N = 32;

ifstream file;

bool last_fill_at_zero = true;

const vector<string> reserved_words = {
    "true", "false", "int", "double", "string", "bool", "please", "def", "while", "for", "if", "else", "break", "continue", "return", "thanks", "fun",
};

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
    while (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
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
            break;
        }
        result += "[]";
    }
    rollback(index, fence);
    return result;
}

string control_flow_keyword(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == 'i') {
        if (next_char(buffer, index, fence) != 'f') {
            rollback(index, fence);
            return "";
        }
        return "if";
    }
    if (cur_char(buffer, index) == 'e') {
        const vector<char> cs = {'l', 's', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "else";
    }
    if (cur_char(buffer, index) == 'b') {
        const vector<char> cs = {'r', 'e', 'a', 'k'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "break";
    }
    if (cur_char(buffer, index) == 'c') {
        const vector<char> cs = {'o', 'n', 't', 'i', 'n', 'u', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "continue";
    }
    if (cur_char(buffer, index) == 'r') {
        const vector<char> cs = {'e', 't', 'u', 'r', 'n'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "return";
    }
    return "";
}

string iteration_keyword(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == 'f') {
        const vector<char> cs = {'o', 'r'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "for";
    }
    if (cur_char(buffer, index) == 'w') {
        const vector<char> cs = {'h', 'i', 'l', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "while";
    }
    return "";
}

string general_keyword(vector<char>& buffer, int& index, int& fence) {
    if (cur_char(buffer, index) == 'p') {
        const vector<char> cs = {'l', 'e', 'a', 's', 'e'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "please";
    }
    if (cur_char(buffer, index) == 'd') {
        const vector<char> cs = {'e', 'f'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "def";
    }
    if (cur_char(buffer, index) == 't') {
        const vector<char> cs = {'h', 'a', 'n', 'k', 's'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "thanks";
    }
    if (cur_char(buffer, index) == 'f') {
        const vector<char> cs = {'u', 'n'};
        for (int i = 0; i < (int)cs.size(); i++) {
            if (next_char(buffer, index, fence) != cs[i]) {
                for (int j = 0; j < i + 1; j++) {
                    rollback(index, fence);
                }
                return "";
            }
        }
        return "fun";
    }
    return "";
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

string punctuation(vector<char>& buffer, int& index) {
    string result = "";
    if (cur_char(buffer, index) == '=') {
        return "=";
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Specify program to compile!\n";
        return 1;
    }
    file = ifstream(argv[1]);
    if (!file.good()) {
        cerr << "There was an error trying to compile the file!\n";
        return 1;
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
        } else if ((result = arithmetic_operator(buffer, index)) != "") {
            // strs
            cout << "(ao, " << result << ")\n";
        } else if ((result = logical_operator(buffer, index, fence)) != "") {
            // strs
            cout << "(log_op, " << result << ")\n";
        } else if ((result = logicalnot(buffer, index)) != "") {
            // strs
            cout << "(log_not, " << result << ")\n";
        } else if ((result = string_token(buffer, index, fence)) != "") {
            // strs
            cout << "(str, " << result << ")\n";
        } else if ((result = identifier(buffer, index, fence)) != "") {
            // strs
            cout << "(identifier, " << result << ")\n";
        } else if ((result = data_type(buffer, index, fence)) != "") {
            // strs
            cout << "(data_type, " << result << ")\n";
        } else if ((result = control_flow_keyword(buffer, index, fence)) != "") {
            // strs
            cout << "(control, " << result << ")\n";
        } else if ((result = iteration_keyword(buffer, index, fence)) != "") {
            // strs
            cout << "(iteration, " << result << ")\n";
        } else if ((result = general_keyword(buffer, index, fence)) != "") {
            // strs
            cout << "(gen_keyword, " << result << ")\n";
        } else if ((result = boolean(buffer, index, fence)) != "") {
            // strs
            cout << "(bool, " << result << ")\n";
        } else if ((result = punctuation(buffer, index)) != "") {
            // strs
            cout << "(punc, " << result << ")\n";
        } else {
            cout << "(unrecognized, " << c << ")\n";
        }
        c = next_char(buffer, index, fence);
    }
    file.close();
}
