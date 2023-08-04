#include "../include/parser.h"
#include "../include/scanner.h"
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

inline bool newline();

struct t {
    const token& next(bool skip_newline = true) {
        if (it->name == token_type::END_OF_INPUT) {
            return *it;
        }
        it++;
        index++;
        if (skip_newline) {
            while (newline()) {
                it++;
                index++;
            }
        }
        return *it;
    }
    const token& previous(bool skip_newline = true) {
        if (it == token_list->begin()) {
            return *it;
        }
        it--;
        index--;
        if (skip_newline) {
            while (newline()) {
                it--;
                index--;
            }
        }
        return *it;
    }
    const token& current() { return *it; }
    int current_index() { return index; }
    std::list<token>::iterator iterator_at_current_pos() { return it; }
    void insert_at(std::list<token>::iterator it2, token t) { token_list->insert(it2, std::move(t)); }
    t(std::list<token>* token_list = nullptr) : token_list(token_list) {}
    void init(std::list<token>& list) {
        token_list = &list;
        it = token_list->begin();
        // skip first element as it is END_OF_INPUT
        it++;
        index = 1;
    }

private:
    std::list<token>* token_list;
    std::list<token>::iterator it;
    int index;
};

t tokens;

// need second argument just to prevent ambiguity
intermediate_representation ir({&ir}, {});

identifier_scopes* current_scope = &ir.scopes;

temp_expr_tree tmp_exp_tree(ir.expressions, &current_scope);

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
            operands.push(
                std::make_unique<expression_tree>(expression_tree{ir.function_calls[cur_ind].identifier, node_type::FUNCTION_CALL, cur_ind}));
            while (tokens.current_index() <= ir.function_calls[cur_ind].end_of_array) {
                tokens.next();
            }
        } else if (ir.array_accesses.count(cur_ind)) {
            operands.push(
                std::make_unique<expression_tree>(expression_tree{ir.array_accesses[cur_ind].identifier, node_type::ARRAY_ACCESS, cur_ind}));
            while (tokens.current_index() <= ir.array_accesses[cur_ind].end_of_array) {
                tokens.next();
            }
        } else if (ir.lists.count(cur_ind)) {
            operands.push(std::make_unique<expression_tree>(expression_tree("", node_type::LIST, cur_ind)));
            while (tokens.current_index() <= ir.lists[cur_ind].end_of_array) {
                tokens.next();
            }
        } else if (is_identifier_or_constant(cur_token)) {
            operands.push(std::make_unique<expression_tree>(expression_tree{token_type_to_node_type(cur_token.name), cur_token.value}));
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
                expression_tree parent = {node_type::OPERATOR, op};
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
                expression_tree parent = {node_type::OPERATOR, op};
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
        expression_tree parent = {node_type::OPERATOR, op};
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
// passing "1" for add_to_ir will add it to the ir, passing "2" will add it directly (not to temp tree)
inline bool expression(int iteration = 0, int add_to_ir = 0);

inline bool identifier() {
    // syntax of identifier already checked when scanning
    return tokens.current().name == token_type::IDENTIFIER;
}

inline bool data_type() {
    // syntax of data_type already checked when scanning
    return tokens.current().name == token_type::DATA_TYPE;
}

inline bool is_valid_int(const std::string& value) {
    if (value.length() == 1) {
        return true;
    }
    if (value[0] == '0') {
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
        return is_valid_int(tokens.current().value.substr(0, tokens.current().value.find('.')));
    }
    return false;
}

inline bool string_token() { return tokens.current().name == token_type::STRING; }

inline bool boolean() { return tokens.current().name == token_type::BOOL; }

inline std::pair<bool, int> list_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "[") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
            args_list al = {tokens.current_index(), std::vector<int>{}, ""};
            tmp_exp_tree.add_list_to_ir(cur_ind, al);
            // ir.lists[cur_ind] = {tokens.current_index(), std::vector<int>{}, ""};
            return {true, cur_ind};
        }
        if (expression(0, 1)) {
            std::vector<int> args = {tmp_exp_tree.last_exp_index()};
            tokens.next();
            while (true) {
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
                    args_list al = {tokens.current_index(), std::move(args), ""};
                    tmp_exp_tree.add_list_to_ir(cur_ind, al);
                    // ir.lists[cur_ind] = {tokens.current_index(), std::move(args), ""};
                    return {true, cur_ind};
                }
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                    tokens.next();
                    if (expression(0, 1)) {
                        args.push_back(tmp_exp_tree.last_exp_index());
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
    return {false, -1};
}

bool parenthesesed_expression() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
        tokens.next();
        if (expression(0, 1)) {
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
            tmp_exp_tree.add_unary_operator_to_ir(cur_ind);
            // ir.unary_operator_indexes.insert(cur_ind);
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

std::pair<bool, identifier_scopes*> body(bool comes_from_for_loop_or_function = false) {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "{") {
        tokens.next();
        // if the body is coming from the for loop or a function, a new scope was already created before since the stuff in the parentheses belongs to
        // that new scope
        if (!comes_from_for_loop_or_function) {
            // new scope
            current_scope = current_scope->new_scope();
        }
        identifier_scopes* val = current_scope;
        if (program()) {
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "}") {
                if (!comes_from_for_loop_or_function) {
                    current_scope = current_scope->upper_scope();
                }
                return {true, val};
            }
        }
        // make sure to go out of that faulty scope if this didn't actually result in a body and delete the created scope
        if (!comes_from_for_loop_or_function) {
            current_scope = current_scope->upper_scope(true);
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, nullptr};
}

bool while_statement() {
    int cur_ind = tokens.current_index();
    int prev_was_thanks = tmp_exp_tree.thanks_flag;
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "thanks") {
        tokens.next();
        tmp_exp_tree.thanks_flag = true;
    } else {
        tmp_exp_tree.thanks_flag = false;
    }
    if (tokens.current().name == token_type::ITERATION && tokens.current().value == "while") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (expression(0, 2)) {
                int condition = tmp_exp_tree.last_exp_index();
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
                    std::pair<bool, identifier_scopes*> p;
                    if ((p = body()).first) {
                        ir.while_statements.push_back({condition, p.second});
                        // leaving while loop, set thanks_flag to previous value
                        tmp_exp_tree.thanks_flag = prev_was_thanks;
                        return true;
                    }
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    // leaving while loop, set thanks_function_flag to previous value
    tmp_exp_tree.thanks_flag = prev_was_thanks;
    return false;
}

bool partial_assignment() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "=") {
        tokens.next();
        if (expression(0, 1)) {
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
            tmp_exp_tree.add_assignment_to_cur_scope(identifier_name, tmp_exp_tree.last_exp_index());
            // current_scope->assignments[identifier_name].push_back(tmp_exp_tree.last_exp_index());
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
                    full_type type = full_type::to_type(tokens.current().value);
                    tokens.next();
                    if (partial_assignment()) {
                        // definition and assignment
                        tmp_exp_tree.add_identifier_to_cur_scope(identifier_name, {type, tmp_exp_tree.last_exp_index(), true});
                        // current_scope->identifiers[identifier_name] = {{}, tmp_exp_tree.last_exp_index(), true};
                        return true;
                    } else {
                        tokens.previous();
                        tmp_exp_tree.add_identifier_to_cur_scope(identifier_name, {type, -1, false});
                        // current_scope->identifiers[identifier_name] = {{}, -1, false};
                        return true;
                    }
                }
            } else if (partial_assignment()) {
                // definition and assignment
                tmp_exp_tree.add_identifier_to_cur_scope(identifier_name, {{}, tmp_exp_tree.last_exp_index(), true});
                // current_scope->identifiers[identifier_name] = {{}, tmp_exp_tree.last_exp_index(), true};
                return true;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

// second bool is true if evaluable was a definition, false otherwise
inline std::pair<bool, bool> evaluable() {
    int cur_ind = tokens.current_index();
    if (definition_or_definition_and_assignment() && tmp_exp_tree.accept_expressions()) {
        return {true, true};
    }
    if (assignment() && tmp_exp_tree.accept_expressions()) {
        return {true, false};
    }
    // no need to check for return value since we're returning false anyway
    tmp_exp_tree.discard_expressions();
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, false};
}

bool for_statement() {
    int cur_ind = tokens.current_index();
    int prev_was_thanks = tmp_exp_tree.thanks_flag;
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "thanks") {
        tokens.next();
        tmp_exp_tree.thanks_flag = true;
    } else {
        tmp_exp_tree.thanks_flag = false;
    }
    if (tokens.current().name == token_type::ITERATION && tokens.current().value == "for") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            current_scope = current_scope->new_scope();
            std::pair<bool, bool> res = {false, false};
            ir.for_statements.push_back({});
            if ((res = evaluable()).first) {
                if (res.second) {
                    ir.for_statements.back() = {tmp_exp_tree.last_identifier_added(), true, -1, "", -1, current_scope};
                } else {
                    auto& p = tmp_exp_tree.last_assignment_added();
                    ir.for_statements.back() = {p.first, false, -1, "", p.second, current_scope};
                }
                tokens.next();
            }
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ";") {
                tokens.next();
                if (expression(0, 2)) {
                    ir.for_statements.back().condition = tmp_exp_tree.last_exp_index();
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ";") {
                        tokens.next();
                        if (assignment()) {
                            auto& p = tmp_exp_tree.last_assignment_added();
                            ir.for_statements.back().after = p.first;
                            ir.for_statements.back().after_index = p.second;
                            tokens.next();
                        }
                        // accept assignment
                        if (tmp_exp_tree.accept_expressions()) {
                            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                                tokens.next();
                                if (body(true).first) {
                                    current_scope = current_scope->upper_scope();
                                    // leaving for loop, set thanks_flag to previous value
                                    tmp_exp_tree.thanks_flag = prev_was_thanks;
                                    return true;
                                }
                            }
                        } else {
                            tmp_exp_tree.discard_expressions();
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
    // leaving for loop, set thanks_function_flag to previous value
    tmp_exp_tree.thanks_flag = prev_was_thanks;
    return false;
}

std::pair<std::string, int> function_call() {
    int cur_ind = tokens.current_index();
    if (identifier()) {
        std::string identifier_name = tokens.current().value;
        tokens.next();
        int iterations = 0;
        int success = false;
        std::vector<std::vector<int>> calls;
        while (true) {
            iterations++;
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    calls.push_back({});
                    continue;
                } else if (expression(0, 1)) {
                    calls.push_back({tmp_exp_tree.last_exp_index()});
                    tokens.next();
                    while (true) {
                        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                            tokens.next();
                            if (expression(0, 1)) {
                                calls.back().push_back(tmp_exp_tree.last_exp_index());
                                tokens.next();
                                continue;
                            }
                            break;
                        } else if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
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
                enhanced_args_list eal = {tokens.current_index(), std::move(calls), identifier_name};
                tmp_exp_tree.add_function_calls_to_ir(cur_ind, eal);
                return {identifier_name, cur_ind};
            } else {
                break;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {"", -1};
}

std::pair<std::string, int> array_access() {
    int cur_ind = tokens.current_index();
    if (identifier()) {
        std::string identifier_name = tokens.current().value;
        tokens.next();
        std::vector<int> args;
        int iterations = 0;
        while (true) {
            iterations++;
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "[") {
                tokens.next();
                if (expression(0, 1)) {
                    args.push_back(tmp_exp_tree.last_exp_index());
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "]") {
                        continue;
                    }
                }
                break;
            } else if (iterations > 1) {
                tokens.previous();
                tmp_exp_tree.add_array_access_to_ir(cur_ind, {tokens.current_index(), std::move(args), identifier_name});
                return {identifier_name, cur_ind};
            } else {
                break;
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {"", -1};
}

inline bool iteration_statement() { return while_statement() || for_statement(); }

std::pair<bool, int> if_structure() {
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "if") {
        tokens.next();
        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
            tokens.next();
            if (expression(0, 2)) {
                int condition = tmp_exp_tree.last_exp_index();
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
                    std::pair<bool, identifier_scopes*> p;
                    if ((p = body()).first) {
                        ir.if_statements.push_back({condition, p.second});
                        int val = ir.if_statements.size() - 1;
                        tokens.next();
                        if (tokens.current().name == token_type::CONTROL && tokens.current().value == "else") {
                            tokens.next();
                            // else if chain
                            if ((p = body()).first) {
                                // else statement
                                ir.if_statements[val].else_body = p.second;
                                return {true, val};
                            }
                            std::pair<bool, int> p2;
                            if ((p2 = if_structure()).first) {
                                ir.if_statements[val].else_if = p2.second;
                                return {true, val};
                            }
                        } else {
                            tokens.previous();
                            // simple if statement
                            return {true, val};
                        }
                    }
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return {false, -1};
}

bool if_statement() {
    int cur_ind = tokens.current_index();
    int prev_was_thanks = tmp_exp_tree.thanks_flag;
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "thanks") {
        tmp_exp_tree.thanks_flag = true;
        tokens.next();
    } else {
        tmp_exp_tree.thanks_flag = false;
    }
    if (if_structure().first) {
        // leaving if statement, set thanks_flag to previous value
        tmp_exp_tree.thanks_flag = prev_was_thanks;
        return true;
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    // leaving if statement, set thanks_flag to previous value
    tmp_exp_tree.thanks_flag = prev_was_thanks;
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
    int cur_ind = tokens.current_index();
    if (tokens.current().name == token_type::CONTROL && tokens.current().value == "return") {
        tokens.next();
        bool has_exp;
        if ((has_exp = expression(0, 1))) {
            tokens.next();
        }
        if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "please") {
            if (has_exp) {
                // expression has please, accept it
                if (!tmp_exp_tree.accept_expressions()) {
                    while (cur_ind < tokens.current_index()) {
                        tokens.previous();
                    }
                    return false;
                }
            }
            return true;
        }
        if (has_exp) {
            // expression has no please, discard
            if (!tmp_exp_tree.discard_expressions()) {
                while (cur_ind < tokens.current_index()) {
                    tokens.previous();
                }
                return false;
            }
        }
        tokens.previous();
        return true;
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
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
    int prev_was_thanks = tmp_exp_tree.thanks_flag;
    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "thanks") {
        tokens.next();
        tmp_exp_tree.thanks_flag = true;
    } else {
        tmp_exp_tree.thanks_flag = false;
    }

    if (tokens.current().name == token_type::GENERAL_KEYWORD && tokens.current().value == "fun") {
        tokens.next();
        if (identifier()) {
            std::string function_name = tokens.current().value;
            tokens.next();
            if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == "(") {
                tokens.next();
                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                        tokens.next();
                        if (data_type()) {
                            full_type return_type = full_type::to_type(tokens.current().value);
                            tokens.next();
                            if (body().first) {
                                ir.function_info.push_back({return_type, {}, tmp_exp_tree.thanks_flag});
                                tmp_exp_tree.add_identifier_to_cur_scope(
                                    function_name, {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, ir.function_info.size() - 1}, -1, true});
                                if (tmp_exp_tree.accept_expressions()) {
                                    // leaving function, set thanks_function_flag to previous value
                                    tmp_exp_tree.thanks_flag = prev_was_thanks;
                                    return true;
                                }
                            }
                        }
                    } else if (body().first) {
                        ir.function_info.push_back({{}, {}, tmp_exp_tree.thanks_flag});
                        tmp_exp_tree.add_identifier_to_cur_scope(
                            function_name, {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, ir.function_info.size() - 1}, -1, true});
                        if (tmp_exp_tree.accept_expressions()) {
                            // leaving function, set thanks_function_flag to previous value
                            tmp_exp_tree.thanks_flag = prev_was_thanks;
                            return true;
                        }
                    }
                } else if (identifier()) {
                    current_scope = current_scope->new_scope();
                    std::vector<std::string> params;
                    const std::string& identifier_name = tokens.current().value;
                    tokens.next();
                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                        tokens.next();
                        if (data_type()) {
                            full_type type = full_type::to_type(tokens.current().value);
                            // since it's a parameter it's always marked as "initialized_with_definition"
                            tmp_exp_tree.add_identifier_to_cur_scope(identifier_name, {type, -1, true});
                            params.push_back(identifier_name);
                            tokens.next();
                            while (true) {
                                if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ",") {
                                    tokens.next();
                                    if (identifier()) {
                                        const std::string& identifier_name = tokens.current().value;
                                        tokens.next();
                                        if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                                            tokens.next();
                                            if (data_type()) {
                                                full_type type = full_type::to_type(tokens.current().value);
                                                // since it's a parameter it's always marked as "initialized_with_defnition"
                                                tmp_exp_tree.add_identifier_to_cur_scope(identifier_name, {type, -1, true});
                                                params.push_back(identifier_name);
                                                tokens.next();
                                                continue;
                                            }
                                        }
                                    }
                                } else if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ")") {
                                    tokens.next();
                                    full_type return_type = {};
                                    if (tokens.current().name == token_type::PUNCTUATION && tokens.current().value == ":") {
                                        tokens.next();
                                        if (data_type()) {
                                            return_type = full_type::to_type(tokens.current().value);
                                            tokens.next();
                                        } else {
                                            break;
                                        }
                                    }
                                    if (body(true).first) {
                                        ir.function_info.push_back({std::move(return_type), std::move(params), tmp_exp_tree.thanks_flag});
                                        tmp_exp_tree.add_identifier_to_cur_scope(
                                            function_name, {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, ir.function_info.size() - 1}, -1, true});
                                        if (tmp_exp_tree.accept_expressions()) {
                                            // leaving function, set thanks_function_flag to previous value
                                            tmp_exp_tree.thanks_flag = prev_was_thanks;
                                            current_scope = current_scope->upper_scope();
                                            return true;
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // make sure to go out of that faulty scope if this didn't actually result in a body and delete the created scope
                    current_scope = current_scope->upper_scope(true);
                }
            }
        }
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    // no need to check for return value since we're returning false anyway
    tmp_exp_tree.discard_expressions();
    // leaving function, set thanks_function_flag to previous value
    tmp_exp_tree.thanks_flag = prev_was_thanks;
    return false;
}

inline bool expression_statement() {
    auto it = tokens.iterator_at_current_pos();
    int cur_ind = tokens.current_index();
    if (expression(0, 1)) {
        tokens.next();
        if (tokens.current().name != token_type::GENERAL_KEYWORD || tokens.current().value != "please") {
            // expression has no please, discard
            if (!tmp_exp_tree.discard_expressions()) {
                while (cur_ind < tokens.current_index()) {
                    tokens.previous();
                }
                return false;
            }
            tokens.previous();
        } else {
            // expression has please, accept expressions
            if (!tmp_exp_tree.accept_expressions()) {
                while (cur_ind < tokens.current_index()) {
                    tokens.previous();
                }
                return false;
            }
            if (tmp_exp_tree.thanks_flag) {
                // has thanks but still please, repeat statement
                std::queue<token*> token_queue;
                // only "<" and not "<=" to not repeat the "please" and have an infinite loop
                for (int i = cur_ind; i < tokens.current_index(); i++) {
                    token_queue.push(&(*it++));
                }
                tokens.insert_at(++it, {token_type::NEWLINE, "\n"});
                while (!token_queue.empty()) {
                    tokens.insert_at(it, *token_queue.front());
                    token_queue.pop();
                }
            }
        }
        return true;
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool initialization_statement() {
    auto it = tokens.iterator_at_current_pos();
    int cur_ind = tokens.current_index();
    if (definition_or_definition_and_assignment() || assignment()) {
        tokens.next();
        if (tokens.current().name != token_type::GENERAL_KEYWORD || tokens.current().value != "please") {
            // expression has no please, discard
            if (!tmp_exp_tree.discard_expressions()) {
                while (cur_ind < tokens.current_index()) {
                    tokens.previous();
                }
                return false;
            }
            tokens.previous();
        } else {
            // expression has please, accept expressions
            if (!tmp_exp_tree.accept_expressions()) {
                while (cur_ind < tokens.current_index()) {
                    tokens.previous();
                }
                return false;
            }
            if (tmp_exp_tree.thanks_flag) {
                // has thanks but still please, repeat statement
                std::queue<token*> token_queue;
                // only "<" and not "<=" to not repeat the "please" and have an infinite loop
                for (int i = cur_ind; i < tokens.current_index(); i++) {
                    token_queue.push(&(*it++));
                }
                tokens.insert_at(++it, {token_type::NEWLINE, "\n"});
                while (!token_queue.empty()) {
                    tokens.insert_at(it, *token_queue.front());
                    token_queue.pop();
                }
            }
        }
        return true;
    }
    while (cur_ind < tokens.current_index()) {
        tokens.previous();
    }
    return false;
}

inline bool statement() {
    return iteration_statement() || if_statement() || jump_statement() || function_statement() || initialization_statement() ||
           expression_statement();
}

inline bool expression(int iteration, int add_to_ir) {
    std::pair<std::string, int> type;
    std::pair<bool, int> nested_exp;
    std::pair<std::string, std::vector<std::vector<int>>*> type_for_func;
    std::pair<std::string, std::vector<int>*> type_for_array_and_list;
    if (iteration == 0 && (nested_exp = comparison_or_arithmetic_expression_or_logical_expression()).first) {
        if (add_to_ir > 0) {
            tmp_exp_tree.add_expression_to_ir(*construct_exp_tree(nested_exp.second), add_to_ir == 2);
        }
    } else if ((nested_exp = unary_expression()).first) {
        if (add_to_ir > 0) {
            tmp_exp_tree.add_expression_to_ir(*construct_exp_tree(nested_exp.second), add_to_ir == 2);
        }
    } else if ((type = function_call()).first != "") {
        if (add_to_ir > 0) {
            expression_tree et = {type.first, node_type::FUNCTION_CALL, type.second};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if ((type = array_access()).first != "") {
        if (add_to_ir > 0) {
            expression_tree et = {type.first, node_type::ARRAY_ACCESS, type.second};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (identifier()) {
        if (add_to_ir > 0) {
            expression_tree et = {node_type::IDENTIFIER, tokens.current().value};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (double_token()) {
        if (add_to_ir > 0) {
            expression_tree et = {node_type::DOUBLE, tokens.current().value};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (integer()) {
        if (add_to_ir > 0) {
            expression_tree et = {node_type::INT, tokens.current().value};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (string_token()) {
        if (add_to_ir > 0) {
            expression_tree et = {node_type::STRING, tokens.current().value};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (boolean()) {
        if (add_to_ir > 0) {
            expression_tree et = {node_type::BOOL, tokens.current().value};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if ((nested_exp = list_expression()).first) {
        if (add_to_ir > 0) {
            expression_tree et = {"", node_type::LIST, nested_exp.second};
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
    } else if (parenthesesed_expression()) {
        if (add_to_ir > 0) {
            // copied expression_tree on purpose to not invalidate the previous expression_tree by moving its ownership
            expression_tree et = ir.expressions.back();
            tmp_exp_tree.add_expression_to_ir(et, add_to_ir == 2);
        }
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
                } else {
                    break;
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
