#include "../include/scanner.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <vector>

using namespace std;

inline bool newline();

struct t {
    const token& next(bool skip_newline = true) {
        if (skip_newline) {
            while (newline()) {
                index++;
            }
        }
        if (token_list->at(index).name != END_OF_INPUT) {
            return token_list->at(++index);
        }
        return token_list->at(index);
    }
    const token& previous(bool skip_newline = true) {
        if (skip_newline) {
            while (newline()) {
                index--;
            }
        }
        if (token_list->at(index).name != END_OF_INPUT) {
            return token_list->at(--index);
        }
        return token_list->at(index);
    }
    const token& current() { return token_list->at(index); }
    int current_index() { return index; }
    t(vector<token>* token_list = nullptr) : token_list(token_list) {}
    void init(vector<token>& list) { token_list = &list; }

private:
    vector<token>* token_list;
    // has to be one because at index 0 we got the END_OF_INPUT token
    int index = 1;
};

t tokens;

inline bool newline() { return tokens.current().name == NEWLINE; }

bool program();
inline bool expression(int iteration = 0);

inline bool identifier() {
    // syntax of identifier already checked when scanning
    return tokens.current().name == IDENTIFIER;
}

inline bool is_valid_int(const string& value) {
    int cur_index = 0;
    if (value[0] == '-') {
        cur_index++;
    }
    if (value.length() - cur_index == 1) {
        return true;
    }
    if (value[cur_index] == '0') {
        return false;
    }
    return true;
}

inline bool integer() {
    if (tokens.current().name == INT) {
        // make sure it's a valid number, meaning no leading zeros
        return is_valid_int(tokens.current().value);
    }
    return false;
}

inline bool double_token() {
    if (tokens.current().name == DOUBLE) {
        // make sure it's a valid number, meaning no leading zeros
        string value = tokens.current().value.substr(0, tokens.current().value.find('.'));
        return is_valid_int(value);
    }
    return false;
}

inline bool number() { return double_token() || integer(); }

inline bool string_token() { return tokens.current().name == STRING; }

inline bool boolean() { return tokens.current().name == BOOL; }

inline bool list_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == PUNCTUATION && tokens.current().value == "[") {
        tokens.next();
        if (tokens.current().name == PUNCTUATION && tokens.current().value == "]") {
            return true;
        }
        if (expression()) {
            tokens.next();
            while (true) {
                if (tokens.current().name == PUNCTUATION && tokens.current().value == "]") {
                    return true;
                }
                if (tokens.current().name == PUNCTUATION && tokens.current().value == ",") {
                    tokens.next();
                    if (expression()) {
                        tokens.next();
                        continue;
                    }
                }
                break;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool parenthesed_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == PUNCTUATION && tokens.current().value == "(") {
        tokens.next();
        if (expression()) {
            tokens.next();
            if (tokens.current().name == PUNCTUATION && tokens.current().value == ")") {
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool arithmetic_operator() { return tokens.current().name == ARITHMETIC_OPERATOR; }

inline bool logical_operator() { return tokens.current().name == LOGICAL_OPERATOR; }

inline bool comparison_operator() { return tokens.current().name == COMPARISON_OPERATOR; }

bool arithmetic_expression() {
    int cur_ind = tokens.current_index();
    if (expression(1)) {
        tokens.next();
        if (arithmetic_operator()) {
            tokens.next();
            if (expression()) {
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool logical_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == LOGICAL_NOT) {
        if (expression(1)) {
            return true;
        }
    } else {
        if (expression(1)) {
            tokens.next();
            if (logical_operator()) {
                tokens.next();
                if (expression()) {
                    return true;
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool comparison_expression() {
    int cur_ind = tokens.current_index();
    if (expression(1)) {
        tokens.next();
        if (comparison_operator()) {
            tokens.next();
            if (expression()) {
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool body() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == PUNCTUATION && tokens.current().value == "{") {
        tokens.next();
        if (program()) {
            if (tokens.current().name == PUNCTUATION && tokens.current().value == "}") {
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool while_statement() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == GENERAL_KEYWORD && tokens.current().value == "please") {
        tokens.next();
    }
    if (tokens.current().name == ITERATION && tokens.current().value == "while") {
        tokens.next();
        if (tokens.current().name == PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (expression()) {
                tokens.next();
                if (tokens.current().name == PUNCTUATION && tokens.current().value == ")") {
                    if (body()) {
                        return true;
                    }
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool function_call() {
    int cur_ind = tokens.current_index();
    if (identifier()) {
        tokens.next();
        if (tokens.current().name == PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (tokens.current().name == PUNCTUATION && tokens.current().value == ")") {
                return true;
            } else if (expression()) {
                tokens.next();
                while (true) {
                    if (tokens.current().name == PUNCTUATION && tokens.current().value == ",") {
                        if (expression()) {
                            continue;
                        }
                        break;
                    } else if (tokens.current().name == PUNCTUATION && tokens.current().value == ")") {
                        return true;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool iteration_statement() { return while_statement() /*  || for_statement() */; }

inline bool expression_statement() {
    if (expression()) {
        tokens.next();
        if (tokens.current().name == GENERAL_KEYWORD && tokens.current().value == "please") {
            return true;
        } else {
            tokens.previous();
            return true;
        }
    }
    return false;
}

inline bool statement() { return iteration_statement() || expression_statement(); }

inline bool expression(int iteration) {
    return (iteration == 0 && (arithmetic_expression() || logical_expression() || comparison_expression())) ||
           (function_call() || identifier() || number() || string_token() || boolean() || list_expression() || parenthesed_expression());
}

bool program() {
    if (statement()) {
        // check for newlines since they matter here
        tokens.next(false);
        while (true) {
            if (newline()) {
                tokens.next();
                if (statement()) {
                    tokens.next(false);
                } else {
                    tokens.previous();
                    tokens.next(false);
                }
            } else {
                break;
            }
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    auto& token_list = scan_program(argc, argv);
    for (auto& el : token_list) {
        cout << el.name << ": " << el.value << "\n";
    }
    tokens.init(token_list);
    if (tokens.current().name == END_OF_INPUT) {
        // empty file
        cout << "success!\n";
        return 0;
    }
    if (program()) {
        if (tokens.next().name == END_OF_INPUT) {
            cout << "success!\n";
            return 0;
        }
    }

    cout << "fail!\n";
}
