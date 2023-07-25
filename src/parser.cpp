#include "../include/ir.h"
#include "../include/scanner.h"
#include <fstream>
#include <iostream>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

inline bool newline();

struct t {
    const token& next(bool skip_newline = true) {
        if (token_list->at(index).name == token_type::END_OF_INPUT) {
            return token_list->at(index);
        }
        index++;
        if (skip_newline) {
            while (newline()) {
                index++;
            }
        }
        return token_list->at(index);
    }
    const token& previous(bool skip_newline = true) {
        if (index == 0) {
            return token_list->at(index);
        }
        index--;
        if (skip_newline) {
            while (newline()) {
                index--;
            }
        }
        return token_list->at(index);
    }
    const token& current() { return token_list->at(index); }
    int current_index() { return index; }
    t(std::vector<token>* token_list = nullptr) : token_list(token_list) {}
    void init(std::vector<token>& list) { token_list = &list; }

private:
    std::vector<token>* token_list;
    // has to be one because at index 0 we got the END_OF_INPUT token
    int index = 1;
};

t tokens;

intermediate_representation ir;

identifier_scopes* current_scope = &ir.scopes;

long long current_expression_index = 0;

/* full_type get_identifier_type(const std::string& name) {
    auto& cur_scope = current_scope;
    while (true) {
        if (cur_scope->identifiers.count(name)) {
            return cur_scope->identifiers[name].type;
        } else if (cur_scope->level == 0) {
            // no definition found searching all the way up to global scope
            return {types::NONE_TYPE, types::NONE_TYPE, 0, nullptr};
        } else {
            cur_scope = cur_scope->upper_scope();
        }
    }
} */

/* full_type get_function_return_type(const std::string& function_name) {
    auto& cur_scope = current_scope;
    while (true) {
        if (cur_scope.functions.count(function_name)) {
            return cur_scope.functions[function_name].return_type;
        } else if (cur_scope.level == 0) {
            // no function definition found searching all the way up to global scope
            return {types::NONE_TYPE, types::NONE_TYPE, 0};
        } else {
            cur_scope = *cur_scope.upper;
        }
    }
} */

inline bool is_identifier_or_constant(const token& t) {
    switch (t.name) {
    case token_type::DOUBLE:
    case token_type::INT:
    case token_type::STRING:
    case token_type::BOOL:
    case token_type::IDENTIFIER:
        return true;
    default:
        return false;
    }
}

inline node_type token_type_to_node_type(const token_type& type) {
    switch (type) {
    case token_type::DOUBLE:
        return node_type::DOUBLE;
    case token_type::INT:
        return node_type::INT;
    case token_type::STRING:
        return node_type::STRING;
    case token_type::BOOL:
        return node_type::BOOL;
    case token_type::IDENTIFIER:
        return node_type::IDENTIFIER;
    case token_type::ARITHMETIC_OPERATOR:
    case token_type::LOGICAL_OPERATOR:
    case token_type::COMPARISON_OPERATOR:
    case token_type::LOGICAL_NOT:
    case token_type::PUNCTUATION:
        return node_type::OPERATOR;
    case token_type::NEWLINE:
    case token_type::CONTROL:
    case token_type::ITERATION:
    case token_type::GENERAL_KEYWORD:
    case token_type::DATA_TYPE:
    case token_type::COMMENT:
    case token_type::END_OF_INPUT:
        throw std::runtime_error("unable to convert <token_type> to <node_type>");
    }
}

int get_precedence(const std::string& op, bool is_unary) {
    static const std::unordered_map<std::string, int> precedence = {{"||", 1}, {"&&", 2}, {"==", 3}, {"!=", 3}, {">", 4},
                                                                    {">=", 4}, {"<", 4},  {"<=", 4}, {"+", 5},  {"-", 5},
                                                                    {"*", 6},  {"/", 6},  {"%", 6},  {"^", 7},  {"!", 8}};
    if (!is_unary) {
        return precedence.at(op);
    }
    // has same precedence as "!" (logical not) operator
    return precedence.at("!");
}

std::unique_ptr<expression_tree> construct_exp_tree(const int start_index) {
    std::stack<std::unique_ptr<expression_tree>> operands;
    std::stack<std::pair<std::string, int>> operators;
    int actual_index = tokens.current_index();
    while (start_index < tokens.current_index()) {
        tokens.previous();
    }
    while (tokens.current_index() <= actual_index) {
        auto& cur_token = tokens.current();
        int cur_ind = tokens.current_index();
        // check for function calls, array accesses and lists first
        if (ir.function_calls.count(cur_ind)) {
            operands.push(std::make_unique<expression_tree>(expression_tree(ir.function_calls[cur_ind].second, node_type::FUNCTION_CALL)));
            while (tokens.current_index() <= ir.function_calls[cur_ind].first) {
                tokens.next();
            }
        } else if (ir.array_accesses.count(cur_ind)) {
            operands.push(std::make_unique<expression_tree>(expression_tree(ir.array_accesses[cur_ind].second, node_type::ARRAY_ACCESS)));
            while (tokens.current_index() <= ir.array_accesses[cur_ind].first) {
                tokens.next();
            }
        } else if (ir.lists.count(cur_ind)) {
            operands.push(std::make_unique<expression_tree>(expression_tree(ir.lists[cur_ind].second, node_type::LIST)));
            while (tokens.current_index() <= ir.lists[cur_ind].first) {
                tokens.next();
            }
        } else if (is_identifier_or_constant(cur_token)) {
            operands.push(std::make_unique<expression_tree>(expression_tree(cur_token.value, token_type_to_node_type(cur_token.name))));
            tokens.next();
        } else if (cur_token.name == token_type::PUNCTUATION && cur_token.value == "(") {
            operators.push({"(", cur_ind});
            tokens.next();
        } else if (cur_token.name == token_type::PUNCTUATION && cur_token.value == ")") {
            while (!operators.empty() && operators.top().first != "(") {
                auto& p = operators.top();
                std::string op = p.first;
                operators.pop();
                auto right = std::move(operands.top());
                operands.pop();
                auto parent = expression_tree(op, node_type::OPERATOR);
                if (op == "!" || ir.unary_operator_indexes.count(p.second)) {
                    parent.right = std::move(right);
                    operands.push(std::make_unique<expression_tree>(parent));
                    continue;
                }
                auto left = std::move(operands.top());
                operands.pop();
                parent.left = std::move(left);
                parent.right = std::move(right);
                operands.push(std::make_unique<expression_tree>(parent));
            }
            if (!operators.empty()) {
                // pop "(" from stack
                operators.pop();
            }
            tokens.next();
        } else if (token_type_to_node_type(cur_token.name) == node_type::OPERATOR) {
            while (!operators.empty() && get_precedence(operators.top().first, ir.unary_operator_indexes.count(operators.top().second)) >=
                                             get_precedence(cur_token.value, ir.unary_operator_indexes.count(cur_ind))) {
                auto& p = operators.top();
                std::string op = p.first;
                operators.pop();
                auto right = std::move(operands.top());
                operands.pop();
                auto parent = expression_tree(op, node_type::OPERATOR);
                if (op == "!" || ir.unary_operator_indexes.count(p.second)) {
                    parent.right = std::move(right);
                    operands.push(std::make_unique<expression_tree>(parent));
                    continue;
                }
                auto left = std::move(operands.top());
                operands.pop();
                parent.left = std::move(left);
                parent.right = std::move(right);
                operands.push(std::make_unique<expression_tree>(parent));
            }
            operators.push({cur_token.value, cur_ind});
            tokens.next();
        }
    }
    tokens.previous();

    while (!operators.empty()) {
        auto& p = operators.top();
        std::string op = p.first;
        operators.pop();
        auto right = std::move(operands.top());
        operands.pop();
        auto parent = expression_tree(op, node_type::OPERATOR);
        if (op == "!" || ir.unary_operator_indexes.count(p.second)) {
            parent.right = std::move(right);
            operands.push(std::make_unique<expression_tree>(parent));
            continue;
        }
        auto left = std::move(operands.top());
        operands.pop();
        parent.left = std::move(left);
        parent.right = std::move(right);
        operands.push(std::make_unique<expression_tree>(parent));
    }
    return std::move(operands.top());
}

/* void print_in_order(expression_tree* root) {
    if (root) {
        print_in_order(root->left);
        std::cout << root->node << " ";
        print_in_order(root->right);
    }
} */

inline bool newline() { return tokens.current().name == token_type::NEWLINE; }

bool program();
inline bool expression(int iteration = 0);

inline bool identifier() {
    // syntax of identifier already checked when scanning
    return tokens.current().name == token_type::IDENTIFIER;
}

inline bool data_type() {
    // syntax of data_type already checked when scanning
    return tokens.current().name == token_type::DATA_TYPE;
}

inline bool is_valid_int(const std::string& value) {
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
    if (tokens.current().name == token_type::INT) {
        // make sure it's a valid number, meaning no leading zeros
        return is_valid_int(tokens.current().value);
    }
    return false;
}

inline bool double_token() {
    if (tokens.current().name == token_type::DOUBLE) {
        // make sure it's a valid number, meaning no leading zeros
        std::string value = tokens.current().value.substr(0, tokens.current().value.find('.'));
        return is_valid_int(value);
    }
    return false;
}

inline bool string_token() { return tokens.current().name == token_type::STRING; }

inline bool boolean() { return tokens.current().name == token_type::BOOL; }

inline std::pair<bool, std::string> list_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "[") {
        std::string full_name = "[";
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
            full_name += "]";
            ir.lists[cur_ind] = {tokens.current_index(), full_name};
            return {true, full_name};
        }
        if (expression()) {
            full_name += std::to_string(ir.expressions.size() - 1);
            tokens.next();
            while (true) {
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
                    ir.lists[cur_ind] = {tokens.current_index(), full_name};
                    return {true, full_name + "]"};
                }
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                    full_name += ",";
                    tokens.next();
                    if (expression()) {
                        full_name += std::to_string(ir.expressions.size() - 1);
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
    return {false, ""};
}

bool parenthesesed_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
        tokens.next();
        if (expression()) {
            tokens.next();
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool arithmetic_operator() { return tokens.current().name == token_type::ARITHMETIC_OPERATOR; }

inline bool logical_operator() { return tokens.current().name == token_type::LOGICAL_OPERATOR; }

inline bool comparison_operator() { return tokens.current().name == token_type::COMPARISON_OPERATOR; }

std::pair<bool, int> unary_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::ARITHMETIC_OPERATOR && (tokens.current().value == "-" || tokens.current().value == "+")) {
        tokens.next();
        if (expression()) {
            ir.unary_operator_indexes.insert(cur_ind);
            return {true, cur_ind};
        }
        tokens.previous();
    }
    return {false, -1};
}

std::pair<bool, int> comparison_or_arithmetic_expression_or_logical_expression() {
    int cur_ind = tokens.current_index();
    if (expression(1)) {
        tokens.next();
        if (comparison_operator() || arithmetic_operator() || logical_operator()) {
            tokens.next();
            if (expression()) {
                return {true, cur_ind};
            }
        }
    } else if (tokens.current().name == token_type::LOGICAL_NOT) {
        tokens.next();
        if (expression()) {
            return {true, cur_ind};
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, -2};
}

bool body(bool comes_from_for_loop = false) {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "{") {
        tokens.next();
        // if the body is coming from the for loop, a new scope was already created before since the stuff in the parentheses belongs to that new
        // scope
        if (!comes_from_for_loop) {
            // new scope
            current_scope = current_scope->new_scope();
        }
        if (program()) {
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "}") {
                if (!comes_from_for_loop) {
                    current_scope = current_scope->upper_scope();
                }
                return true;
            }
        }
        // make sure to go out of that faulty scope if this didn't actually result in a body and delete the created scope
        if (!comes_from_for_loop) {
            current_scope = current_scope->upper_scope(true);
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool while_statement() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
        tokens.next();
    }
    if (tokens.current().name == token_type::ITERATION && tokens.current().value == "while") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (expression()) {
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
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

bool partial_assignment() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "=") {
        tokens.next();
        if (expression()) {
            tokens.next();
            if (tokens.current().name != token_type::GENERAL_KEYWORD || tokens.current().value != "please") {
                tokens.previous();
            }
            return true;
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

bool assignment() {
    if (identifier()) {
        std::string identifier_name = tokens.current().value;
        tokens.next();
        if (partial_assignment()) {
            // check whether idenfifier exists
            return true;
        }
        tokens.previous();
    }
    return false;
}

// taking both definition and definition_and_assignment into the same function for speed improvement
bool definition_or_definition_and_assignment() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "def") {
        tokens.next();
        if (identifier()) {
            std::string identifier_name = tokens.current().value;
            tokens.next();
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                tokens.next();
                if (data_type()) {
                    tokens.next();
                    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
                        // definition
                        current_scope->identifiers[identifier_name] = {{}, -1, false};
                        return true;
                    } else if (partial_assignment()) {
                        // definition and assignment
                        current_scope->identifiers[identifier_name] = {{}, (int)ir.expressions.size() - 1, false};
                        return true;
                    } else {
                        // definition without please at the end
                        return true;
                    }
                }
            } else if (partial_assignment()) {
                // definition and assignment
                current_scope->identifiers[identifier_name] = {{}, (int)ir.expressions.size() - 1, false};
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool evaluable() { return assignment() || definition_or_definition_and_assignment(); }

bool for_statement() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
        tokens.next();
    }
    if (tokens.current().name == token_type::ITERATION && tokens.current().value == "for") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            current_scope = current_scope->new_scope();
            if (evaluable()) {
                tokens.next();
            }
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ";") {
                tokens.next();
                if (expression()) {
                    tokens.next();
                }
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ";") {
                    tokens.next();
                    if (evaluable()) {
                        tokens.next();
                    }
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                        tokens.next();
                        if (body(true)) {
                            current_scope = current_scope->upper_scope();
                            return true;
                        }
                    }
                }
            }
            // make sure to go out of that faulty scope if this didn't actually result in a body and delete the created scope
            current_scope = current_scope->upper_scope(true);
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

std::pair<bool, std::string> function_call() {
    int cur_ind = tokens.current_index();
    if (identifier()) {
        std::string identifier_name = tokens.current().value;
        std::string full_name = identifier_name;
        tokens.next();
        int iterations = 0;
        int success = false;
        while (true) {
            iterations++;
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
                full_name += "(";
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    full_name += ")";
                    ir.function_calls[cur_ind] = {tokens.current_index(), full_name};
                    return {true, full_name};
                } else if (expression()) {
                    full_name += std::to_string(ir.expressions.size() - 1);
                    tokens.next();
                    while (true) {
                        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                            full_name += ",";
                            tokens.next();
                            if (expression()) {
                                full_name += std::to_string(ir.expressions.size() - 1);
                                tokens.next();
                                continue;
                            }
                            break;
                        } else if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                            full_name += ")";
                            tokens.next();
                            success = true;
                            break;
                        } else {
                            break;
                        }
                    }
                    if (!success) {
                        break;
                    }
                    success = false;
                } else {
                    break;
                }
            } else if (iterations > 1) {
                tokens.previous();
                ir.function_calls[cur_ind] = {tokens.current_index(), full_name};
                return {true, full_name};
            } else {
                break;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, ""};
}

std::pair<bool, std::string> array_access() {
    int cur_ind = tokens.current_index();
    if (identifier()) {
        std::string identifier_name = tokens.current().value;
        std::string full_name = identifier_name;
        tokens.next();
        int iterations = 0;
        while (true) {
            iterations++;
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "[") {
                full_name += "[";
                tokens.next();
                if (expression()) {
                    full_name += std::to_string(ir.expressions.size() - 1);
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
                        full_name += "]";
                        continue;
                    }
                }
                break;
            } else if (iterations > 1) {
                tokens.previous();
                ir.array_accesses[cur_ind] = {tokens.current_index(), full_name};
                return {true, full_name};
            } else {
                break;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, ""};
}

inline bool iteration_statement() { return while_statement() || for_statement(); }

bool if_structure() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "if") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (expression()) {
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
                    if (body()) {
                        tokens.next();
                        if (tokens.current().name == token_type::CONTROL && tokens.current().value == "else") {
                            tokens.next();
                            // else if chain
                            if (body() || if_structure()) {
                                return true;
                            }
                        } else {
                            tokens.previous();
                            // simple if statement
                            return true;
                        }
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

bool if_statement() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value != "please") {
        tokens.next();
    }
    if (if_structure()) {
        return true;
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool break_statement() {
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "break") {
        tokens.next();
        if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
            return true;
        }
        tokens.previous();
        return true;
    }
    return false;
}

inline bool return_statement() {
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "return") {
        tokens.next();
        if (expression()) {
            tokens.next();
        }
        if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
            return true;
        }
        tokens.previous();
        return true;
    }
    return false;
}

inline bool continue_statement() {
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "continue") {
        tokens.next();
        if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
            return true;
        }
        tokens.previous();
        return true;
    }
    return false;
}

inline bool jump_statement() { return break_statement() || return_statement() || continue_statement(); }

bool function_statement() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
        tokens.next();
    }
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "thanks") {
        tokens.next();
    }
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "fun") {
        tokens.next();
        if (identifier()) {
            tokens.next();
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                        tokens.next();
                        if (data_type()) {
                            tokens.next();
                            if (body()) {
                                return true;
                            }
                        }
                    } else if (body()) {
                        return true;
                    }
                } else if (identifier()) {
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                        tokens.next();
                        if (data_type()) {
                            tokens.next();
                            while (true) {
                                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                                    tokens.next();
                                    if (identifier()) {
                                        tokens.next();
                                        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                                            tokens.next();
                                            if (data_type()) {
                                                tokens.next();
                                                continue;
                                            }
                                        }
                                    }
                                } else if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                                    tokens.next();
                                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                                        tokens.next();
                                        if (data_type()) {
                                            tokens.next();
                                        } else {
                                            break;
                                        }
                                    }
                                    if (body()) {
                                        return true;
                                    }
                                }
                                break;
                            }
                        }
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

inline bool expression_statement() {
    if (expression()) {
        tokens.next();
        if (tokens.current().name != token_type::GENERAL_KEYWORD || tokens.current().value != "please") {
            tokens.previous();
        }
        return true;
    }
    return false;
}

inline bool initialization_statement() { return definition_or_definition_and_assignment() || assignment(); }

inline bool statement() {
    return iteration_statement() || if_statement() || jump_statement() || function_statement() || initialization_statement() ||
           expression_statement();
}

inline bool expression(int iteration) {
    std::pair<bool, std::string> type;
    std::pair<bool, int> nested_exp;
    if (iteration == 0 && (nested_exp = comparison_or_arithmetic_expression_or_logical_expression()).first) {
        ir.expressions.push_back(std::move(*construct_exp_tree(nested_exp.second)));
    } else if ((nested_exp = unary_expression()).first) {
        ir.expressions.push_back(std::move(*construct_exp_tree(nested_exp.second)));
    } else if ((type = function_call()).first) {
        ir.expressions.push_back({type.second, node_type::FUNCTION_CALL});
    } else if ((type = array_access()).first) {
        ir.expressions.push_back({type.second, node_type::ARRAY_ACCESS});
    } else if (identifier()) {
        ir.expressions.push_back({tokens.current().value, node_type::IDENTIFIER});
    } else if (double_token()) {
        ir.expressions.push_back({tokens.current().value, node_type::DOUBLE});
    } else if (integer()) {
        ir.expressions.push_back({tokens.current().value, node_type::INT});
    } else if (string_token()) {
        ir.expressions.push_back({tokens.current().value, node_type::STRING});
    } else if (boolean()) {
        ir.expressions.push_back({tokens.current().value, node_type::BOOL});
    } else if ((type = list_expression()).first) {
        ir.expressions.push_back({type.second, node_type::LIST});
    } else if (parenthesesed_expression()) {
        // copied expression_tree on purpose to not invalidate the previous expression_tree by moving its ownership
        ir.expressions.push_back(ir.expressions.back());
    } else {
        return false;
    }

    return true;
}

bool program() {
    if (newline()) {
        tokens.next();
    }
    if (statement()) {
        // check for newlines since they matter here
        tokens.next(false);
        while (true) {
            if (newline()) {
                // can safely be false because the next token cannot be a newline
                tokens.next(false);
                if (statement()) {
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
    tokens.init(token_list);
    if (tokens.current().name == token_type::END_OF_INPUT) {
        // empty file
        std::cout << "success!\n";
        return 0;
    }

    if (program()) {
        if (tokens.next().name == token_type::END_OF_INPUT) {
            std::cout << "success!\n";
            return 0;
        }
    }
    std::cout << "fail!\n";
    return 1;
}
