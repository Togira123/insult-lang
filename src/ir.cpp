#include "../include/ir.h"

void check_statement(identifier_scopes* cur, size_t order_index);

full_type evaluate_expression(identifier_scopes* cur_scope, expression_tree& root);

full_type validate_function_call(identifier_scopes* cur_scope, int function_call);

full_type validate_array_access(identifier_scopes* cur_scope, int array_access);

full_type validate_list(identifier_scopes* cur_scope, int list_index);

// makes sure that the type of identifier is set
identifier_detail& get_identifier_definition(identifier_scopes* cur_scope, std::string& name) {
    while (true) {
        if (cur_scope->identifiers.count(name)) {
            // if not a function it had to be defined before
            if (cur_scope->identifiers[name].type.type != types::FUNCTION_TYPE && !cur_scope->identifiers[name].already_defined) {
                throw std::runtime_error("cannot access " + name + " before it is defined");
            }
            if (cur_scope->identifiers[name].type.type == types::UNKNOWN_TYPE) {
                cur_scope->identifiers[name].type =
                    evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[cur_scope->identifiers[name].initializing_expression]);
            }
            return cur_scope->identifiers[name];
        } else if (cur_scope->level == 0) {
            // no definition found searching all the way up to global scope
            throw std::runtime_error("no definition found for " + name);
        } else {
            cur_scope = cur_scope->upper_scope();
        }
    }
}

full_type evaluate_expression(identifier_scopes* cur_scope, expression_tree& root) {
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS:
            return validate_array_access(cur_scope, root.args_array_access_index);
        case node_type::FUNCTION_CALL:
            return validate_function_call(cur_scope, root.args_function_call_index);
        case node_type::IDENTIFIER:
            return get_identifier_definition(cur_scope, root.node).type;
        case node_type::LIST:
            return validate_list(cur_scope, root.args_list_index);
        default:
            return full_type::to_type(root.type);
        }
    } else {
        if (root.left == nullptr) {
            full_type right = evaluate_expression(cur_scope, *root.right);
            // unary operator
            if (root.node == "!") {
                if (right.type == types::BOOL_TYPE) {
                    return right;
                }
                throw std::runtime_error("expected expression to be of type BOOL_TYPE");
            } else if (root.node == "+" || root.node == "-") {
                if (right.type == types::INT_TYPE || right.type == types::DOUBLE_TYPE) {
                    return right;
                }
                throw std::runtime_error("expected expression to be of a numeric type");
            } else {
                // something went wrong
                throw std::runtime_error("something unexpected happened");
            }
        } else {
            full_type left = evaluate_expression(cur_scope, *root.left);
            full_type right = evaluate_expression(cur_scope, *root.right);
            if (root.node == "||" || root.node == "&&") {
                // both left and right must be BOOL_TYPE
                if (left.type == types::BOOL_TYPE && right.type == types::BOOL_TYPE) {
                    return left;
                }
                throw std::runtime_error("expected expression to be of type BOOL_TYPE");
            } else if (root.node == "==" || root.node == "!=" || root.node == ">" || root.node == ">=" || root.node == "<" || root.node == "<=") {
                if (left.is_assignable_to(right)) {
                    return {types::BOOL_TYPE};
                }
                throw std::runtime_error("expected expressions to be assignable");
            } else if (root.node == "+") {
                if (left.type != types::BOOL_TYPE && left.is_assignable_to(right)) {
                    switch (left.type) {
                    case types::INT_TYPE:
                        return right.type == types::INT_TYPE ? full_type{types::INT_TYPE} : full_type{types::DOUBLE_TYPE};
                    default:
                        // left == (ARRAY_TYPE || DOUBLE_TYPE || STRING_TYPE)
                        return left;
                    }
                }
                throw std::runtime_error("expected other types");
            } else if (root.node == "-" || root.node == "*" || root.node == "/" || root.node == "%" || root.node == "^") {
                if ((left.type == types::INT_TYPE || left.type == types::DOUBLE_TYPE) && left.is_assignable_to(right)) {
                    return left.type == types::INT_TYPE && right.type == types::INT_TYPE ? full_type{types::INT_TYPE} : full_type{types::DOUBLE_TYPE};
                }
                throw std::runtime_error("expected expression to be of a numeric type");
            } else {
                // something went wrong
                throw std::runtime_error("something unexpected happened");
            }
        }
    }
}

full_type validate_function_call(identifier_scopes* cur_scope, int function_call) {
    auto& call = cur_scope->get_ir()->function_calls[function_call];
    // get function definition
    std::string& name = call.identifier;
    auto& id = get_identifier_definition(cur_scope, name);
    if (id.type.type != types::FUNCTION_TYPE) {
        throw std::runtime_error(name + " is not a function");
    }
    size_t function_info_ind = id.type.function_info;
    function_detail& fd = cur_scope->get_ir()->function_info[function_info_ind];
    if (fd.parameter_list.size() < call.args.size()) {
        throw std::runtime_error("too many arguments");
    } else if (fd.parameter_list.size() > call.args.size()) {
        throw std::runtime_error("too few arguments");
    }
    // check if expressions match

    // finally return type
    // if return type is unknown have to check whole function and go through it following order vector
    // then mark as checked to not double check later on
    if (fd.return_type.type == types::UNKNOWN_TYPE) {
        // this also sets the return type of the function
        check_statement(fd.body, 0);
        return fd.return_type;
    }
    return fd.return_type;
}

full_type validate_array_access(identifier_scopes* cur_scope, int array_access) {
    auto& access = cur_scope->get_ir()->array_accesses[array_access];
    // get definition of identifier
    std::string& name = access.identifier;
    auto& id = get_identifier_definition(cur_scope, name);
    if (id.type.type != types::ARRAY_TYPE && id.type.type != types::STRING_TYPE) {
        throw std::runtime_error(name + " is not an array nor a string");
    }
    types array_type = id.type.array_type;
    // if the identifier is a string
    if (id.type.type == types::STRING_TYPE) {
        if (access.args.size() == 1 && evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[access.args[0]]).type == types::INT_TYPE) {
            return {types::STRING_TYPE};
        } else {
            throw std::runtime_error("invalid array access");
        }
    }
    int dimension = id.type.dimension;
    if ((size_t)(dimension + (array_type == types::STRING_TYPE ? 1 : 0)) < access.args.size()) {
        throw std::runtime_error("invalid array access");
    }
    for (const size_t& arg : access.args) {
        if (evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[arg]).type != types::INT_TYPE) {
            throw std::runtime_error("array indexes must be integers");
        }
        dimension--;
    }
    if (dimension > 0) {
        return {types::ARRAY_TYPE, array_type, dimension};
    }
    if (dimension == 0) {
        return {array_type};
    }
    // dimension == -1
    // array type is STRING_TYPE
    return {types::STRING_TYPE};
}

full_type validate_list(identifier_scopes* cur_scope, int list_index) {
    auto& list = cur_scope->get_ir()->lists[list_index];
    if (list.args.size() == 0) {
        // unknown array type, assignable to any kind of array
        return {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 0};
    }
    if (list.args.size() == 1) {
        full_type t = evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[0]]);
        if (t.type == types::ARRAY_TYPE) {
            return {types::ARRAY_TYPE, t.array_type, t.dimension + 1};
        }
    }
    const full_type& expected_type = evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[0]]);
    for (size_t i = 1; i < list.args.size(); i++) {
        if (!evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[i]]).is_assignable_to(expected_type)) {
            throw std::runtime_error("array types do not match");
        }
    }
    if (expected_type.type == types::ARRAY_TYPE) {
        return {types::ARRAY_TYPE, expected_type.array_type, expected_type.dimension + 1};
    }
    return {types::ARRAY_TYPE, expected_type.type, 1};
}

void validate_definition(identifier_scopes* cur_scope, identifier_detail& id) {
    id.already_defined = true;
    if (!id.initialized_with_definition) {
        return;
    }
    if (id.type.type == types::UNKNOWN_TYPE) {
        id.type = evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[id.initializing_expression]);
        return;
    }
    if (!id.type.is_assignable_to(evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[id.initializing_expression]))) {
        throw std::runtime_error("expression not assignable to identifier");
    }
}

void handle_for_statement(for_statement_struct& for_statement) {
    auto* const scope = for_statement.body;
    auto& ir = *scope->get_ir();
    if (for_statement.init_statement != "") {
        // check whether definition/assignment is okay
        if (for_statement.init_statement_is_definition) {
            auto& id = scope->identifiers[for_statement.init_statement];
            validate_definition(scope, id);
        } else {
            auto& id = get_identifier_definition(scope, for_statement.init_statement);
            int ind = scope->assignments[for_statement.init_statement][for_statement.init_index];
            if (!id.type.is_assignable_to(evaluate_expression(scope, ir.expressions[ind]))) {
                throw std::runtime_error("expression is not assignable to identifier");
            }
        }
    }
    // make sure condition is a bool value
    if (!evaluate_expression(scope, ir.expressions[for_statement.condition]).is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    if (for_statement.after != "") {
        // make sure assignment is okay
        auto& id = get_identifier_definition(scope, for_statement.after);
        int ind = scope->assignments[for_statement.after][for_statement.after_index];
        if (!id.type.is_assignable_to(evaluate_expression(scope, ir.expressions[ind]))) {
            throw std::runtime_error("expression is not assignable to identifier");
        }
    }
    auto& order = scope->order;
    // check body
    for (size_t order_index = 0; order_index < order.size(); order_index++) {
        check_statement(scope, order_index);
    }
}

void handle_while_statement(while_statement_struct& while_statement) {}

void check_statement(identifier_scopes* cur, size_t order_index) {
    intermediate_representation& ir = *cur->get_ir();
    const auto& cur_statement = cur->order[order_index];
    switch (cur_statement.type) {
    case statement_type::FOR:
        handle_for_statement(ir.for_statements[cur_statement.index]);
    case statement_type::WHILE:
        handle_while_statement(ir.while_statements[cur_statement.index]);
    // for statement_type::RETURN it has to check whether it is in a function and if yes if the types match (or determine the function's type
    // if UNKNOWN)
    default:
        break;
    }
}

void check_ir(intermediate_representation& ir) {
    identifier_scopes* cur = &ir.scopes;
    for (size_t order_index = 0; order_index < cur->order.size(); order_index++) {
        check_statement(cur, order_index);
    }
}
