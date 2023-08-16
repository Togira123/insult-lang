#include "../include/ir.h"
#include "../include/lib/library.h"
#include <unordered_set>

bool function_returns_in_all_paths(identifier_scopes* cur_scope);

void check_statement(identifier_scopes* cur, size_t order_index);

full_type evaluate_expression(identifier_scopes* cur_scope, expression_tree& root, int exp_ind);

full_type validate_function_call(identifier_scopes* cur_scope, int function_call, int exp_ind);

full_type validate_array_access(identifier_scopes* cur_scope, int array_access, int exp_ind);

full_type validate_list(identifier_scopes* cur_scope, int list_index);

void handle_assignment(identifier_scopes* scope, std::string& name, int index, int order_ind);

// makes sure that the type of identifier is set
identifier_detail& get_identifier_definition(identifier_scopes* cur_scope, std::string& name, int exp_ind) {
    static auto& ir = *cur_scope->get_ir();
    while (true) {
        if (cur_scope->identifiers.count(name)) {
            // if not a function it had to be defined before
            if (cur_scope->identifiers[name].type.type != types::FUNCTION_TYPE && !cur_scope->identifiers[name].already_defined) {
                throw std::runtime_error("cannot access " + name + " before it is defined");
            }
            if (cur_scope->identifiers[name].type.type == types::FUNCTION_TYPE && cur_scope->level != 0) {
                throw std::runtime_error("functions must be defined in global scope");
            }
            if (cur_scope->identifiers[name].type.type == types::UNKNOWN_TYPE) {
                cur_scope->identifiers[name].type =
                    evaluate_expression(cur_scope, ir.expressions[cur_scope->identifiers[name].initializing_expression],
                                        cur_scope->identifiers[name].initializing_expression);
            }
            if (!ir.has_arrays && cur_scope->identifiers[name].type.type == types::ARRAY_TYPE) {
                ir.has_arrays = true;
            }
            if (exp_ind >= 0) {
                cur_scope->identifiers[name].references.push_back(exp_ind);
            }
            return cur_scope->identifiers[name];
        } else if (cur_scope->level == 0) {
            // no definition found searching all the way up to global scope
            // check if it's a std library name
            return identifier_detail_of(ir, name);
        } else {
            cur_scope = cur_scope->upper_scope();
        }
    }
}

full_type evaluate_expression(identifier_scopes* cur_scope, expression_tree& root, int exp_ind) {
    static auto& ir = *cur_scope->get_ir();
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS:
            return validate_array_access(cur_scope, root.args_array_access_index, exp_ind);
        case node_type::FUNCTION_CALL:
            return validate_function_call(cur_scope, root.args_function_call_index, exp_ind);
        case node_type::IDENTIFIER:
            return get_identifier_definition(cur_scope, root.node, exp_ind).type;
        case node_type::LIST:
            return validate_list(cur_scope, root.args_list_index);
        default:
            return full_type::to_type(root.type);
        }
    } else {
        if (root.left == nullptr) {
            full_type right = evaluate_expression(cur_scope, *root.right, exp_ind);
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
            full_type left = evaluate_expression(cur_scope, *root.left, exp_ind);
            full_type right = evaluate_expression(cur_scope, *root.right, exp_ind);
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
            } else if (root.node == "+" || root.node == "$") {
                if (left.type != types::BOOL_TYPE && left.is_assignable_to(right)) {
                    switch (left.type) {
                    case types::INT_TYPE:
                        return right.type == types::INT_TYPE ? full_type{types::INT_TYPE} : full_type{types::DOUBLE_TYPE};
                    case types::ARRAY_TYPE:
                        if (!ir.has_add_vectors) {
                            ir.has_add_vectors = true;
                            // both arguments are arrays which has to be marked in order to handle them correctly when generating code
                            root.node = "$";
                        }
                        // no break on purpose because we still have to return
                    default:
                        // left == (DOUBLE_TYPE || STRING_TYPE)
                        return left;
                    }
                }
                throw std::runtime_error("expected other types");
            } else if (root.node == "%") {
                if (left.type == types::INT_TYPE && right.type == types::INT_TYPE) {
                    return {types::INT_TYPE};
                }
                throw std::runtime_error("expected two integers for % operator");
            } else if (root.node == "-" || root.node == "*" || root.node == "/" || root.node == "^" || root.node == "#") {
                if ((left.type == types::INT_TYPE || left.type == types::DOUBLE_TYPE) && left.is_assignable_to(right)) {
                    if (root.node == "^" && left.type == types::INT_TYPE && right.type == types::INT_TYPE) {
                        // since both arguments are integers we can use exponentation by squaring which is fast
                        // change the node to "#" to let code generation later use that fast approach instead of the std::pow() function
                        ir.has_fast_exponent = true;
                        root.node = "#";
                    }
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

void check_function_body(identifier_scopes* body) {
    static std::unordered_set<identifier_scopes*> checked;
    if (checked.count(body) == 0) {
        for (size_t order_index = 0; order_index < body->order.size(); order_index++) {
            check_statement(body, order_index);
        }
        checked.insert(body);
    }
}

full_type validate_function_call(identifier_scopes* cur_scope, int function_call, int exp_ind) {
    static std::unordered_set<identifier_scopes*> function_calls_set;
    static auto& ir = *cur_scope->get_ir();
    auto& call = ir.function_calls[function_call];
    // get function definition
    std::string& name = call.identifier;
    auto& id = get_identifier_definition(cur_scope, name, exp_ind);
    if (id.type.type != types::FUNCTION_TYPE) {
        throw std::runtime_error(name + " is not a function");
    }
    id.function_call_references.push_back(function_call);
    auto& overloads = id.type.function_info;
    for (auto& overload : overloads) {
        function_detail& fd = ir.function_info[overload];
        if (fd.parameter_list.size() == call.args.size()) {
            // potential match
            bool matched = true;
            for (size_t i = 0; i < call.args.size(); i++) {
                full_type t = evaluate_expression(cur_scope, ir.expressions[call.args[i]], call.args[i]);
                auto& id = fd.body->identifiers[fd.parameter_list[i]];
                if (!t.is_assignable_to(id.type)) {
                    matched = false;
                    break;
                }
            }
            if (!matched) {
                continue;
            }
            if (function_calls_set.count(fd.body)) {
                throw std::runtime_error("(indirect) recursion requires explicitly stated return type");
            }
            function_calls_set.insert(fd.body);
            // finally return type
            // if return type is unknown have to check whole function and go through it following order vector
            // then mark as checked to not double check later on
            if (fd.return_type.type == types::UNKNOWN_TYPE) {
                // this also sets the return type of the function
                check_function_body(fd.body);
                if (fd.return_type.type == types::UNKNOWN_TYPE) {
                    fd.return_type = {types::VOID_TYPE};
                }
            }
            if (!ir.has_arrays && fd.return_type.type == types::ARRAY_TYPE) {
                ir.has_arrays = true;
            }
            function_calls_set.erase(fd.body);
            return fd.return_type;
        }
    }
    throw std::runtime_error("no matching overload found");
}

full_type validate_array_access(identifier_scopes* cur_scope, int array_access, int exp_ind) {
    auto& access = cur_scope->get_ir()->array_accesses[array_access];
    // get definition of identifier
    std::string& name = access.identifier;
    auto& id = get_identifier_definition(cur_scope, name, exp_ind);
    if (id.type.type != types::ARRAY_TYPE && id.type.type != types::STRING_TYPE) {
        throw std::runtime_error(name + " is not an array nor a string");
    }
    id.array_access_references.push_back(array_access);
    types array_type = id.type.array_type;
    // if the identifier is a string
    if (id.type.type == types::STRING_TYPE) {
        if (access.args.size() == 1 &&
            evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[access.args[0]], access.args[0]).type == types::INT_TYPE) {
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
        if (evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[arg], arg).type != types::INT_TYPE) {
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
        full_type t = evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[0]], list.args[0]);
        if (t.type == types::ARRAY_TYPE) {
            return {types::ARRAY_TYPE, t.array_type, t.dimension + 1};
        } else if (t.type == types::BOOL_TYPE || t.type == types::DOUBLE_TYPE || t.type == types::INT_TYPE || t.type == types::STRING_TYPE) {
            return {types::ARRAY_TYPE, t.type, 1};
        } else {
            throw std::runtime_error("unexpected type");
        }
    }
    const full_type& expected_type = evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[0]], list.args[0]);
    for (size_t i = 1; i < list.args.size(); i++) {
        if (!evaluate_expression(cur_scope, cur_scope->get_ir()->expressions[list.args[i]], list.args[i]).is_assignable_to(expected_type)) {
            throw std::runtime_error("array types do not match");
        }
    }
    if (expected_type.type == types::ARRAY_TYPE) {
        return {types::ARRAY_TYPE, expected_type.array_type, expected_type.dimension + 1};
    }
    return {types::ARRAY_TYPE, expected_type.type, 1};
}

void validate_definition(identifier_scopes* cur_scope, identifier_detail& id, int order_ind) {
    static auto& ir = *cur_scope->get_ir();
    if (order_ind >= 0) {
        id.order_references.push_back({cur_scope, order_ind});
    }
    if (!id.initialized_with_definition) {
        return;
    }
    if (id.type.type == types::UNKNOWN_TYPE) {
        id.type = evaluate_expression(cur_scope, ir.expressions[id.initializing_expression], id.initializing_expression);
        if (id.type.type == types::VOID_TYPE || id.type.type == types::FUNCTION_TYPE) {
            throw std::runtime_error("expression not assignable to identifier");
        }
    }
    if (!id.type.is_assignable_to(evaluate_expression(cur_scope, ir.expressions[id.initializing_expression], id.initializing_expression))) {
        throw std::runtime_error("expression not assignable to identifier");
    }
    if (!ir.has_arrays && id.type.type == types::ARRAY_TYPE) {
        ir.has_arrays = true;
    }
    id.already_defined = true;
}

void handle_for_statement(for_statement_struct& for_statement, size_t index) {
    auto* const scope = for_statement.body;
    static auto& ir = *scope->get_ir();
    if (for_statement.init_statement != "") {
        // check whether definition/assignment is okay
        if (for_statement.init_statement_is_definition) {
            auto& id = scope->identifiers[for_statement.init_statement];
            validate_definition(scope, id, -1);
            id.for_statement_init.push_back(index);
        } else {
            handle_assignment(scope, for_statement.init_statement, for_statement.init_index, -1);
            auto& id = get_identifier_definition(scope, for_statement.init_statement, -1);
            id.for_statement_init.push_back(index);
        }
    }
    // make sure condition is a bool value
    if (!evaluate_expression(scope, ir.expressions[for_statement.condition], for_statement.condition).is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    if (for_statement.after != "") {
        // make sure assignment is okay
        handle_assignment(scope, for_statement.after, for_statement.after_index, -1);
        auto& id = get_identifier_definition(scope, for_statement.init_statement, -1);
        id.for_statement_after.push_back(index);
    }
    auto& order = scope->order;
    // check body
    for (size_t order_index = 0; order_index < order.size(); order_index++) {
        check_statement(scope, order_index);
    }
}

void handle_while_statement(while_statement_struct& while_statement) {
    auto* const scope = while_statement.body;
    static auto& ir = *scope->get_ir();
    // make sure condition is a bool value
    if (!evaluate_expression(&scope->get_upper(), ir.expressions[while_statement.condition], while_statement.condition)
             .is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    // check body
    for (size_t order_index = 0; order_index < scope->order.size(); order_index++) {
        check_statement(scope, order_index);
    }
}

void handle_if_statement(if_statement_struct& if_statement) {
    auto* const scope = if_statement.body;
    static auto& ir = *scope->get_ir();
    // make sure condition is a bool value
    if (!evaluate_expression(&scope->get_upper(), ir.expressions[if_statement.condition], if_statement.condition)
             .is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    // check body
    for (size_t order_index = 0; order_index < scope->order.size(); order_index++) {
        check_statement(scope, order_index);
    }
    if (if_statement.else_if >= 0) {
        handle_if_statement(ir.if_statements[if_statement.else_if]);
    } else if (if_statement.else_body != nullptr) {
        for (size_t order_index = 0; order_index < if_statement.else_body->order.size(); order_index++) {
            check_statement(if_statement.else_body, order_index);
        }
    }
}

void handle_return_statement(function_detail& fd, size_t return_statement_index) {
    auto* const scope = fd.body;
    static auto& ir = *scope->get_ir();
    if (fd.return_type.type == types::UNKNOWN_TYPE) {
        // type is not set yet, set it
        if (ir.return_statements[return_statement_index] < 0) {
            fd.return_type = {types::VOID_TYPE};
        } else {
            fd.return_type = evaluate_expression(scope, ir.expressions[ir.return_statements[return_statement_index]],
                                                 ir.return_statements[return_statement_index]);
        }
    } else {
        // make sure return statement returns type that is assignable to function return value
        if (ir.return_statements[return_statement_index] < 0) {
            if (fd.return_type.type != types::VOID_TYPE) {
                throw std::runtime_error("function return type must match with returned type");
            }
        } else {
            if (!fd.return_type.is_assignable_to(evaluate_expression(scope, ir.expressions[ir.return_statements[return_statement_index]],
                                                                     ir.return_statements[return_statement_index]))) {
                throw std::runtime_error("function return type must match with returned type");
            }
        }
    }
}

bool check_if_statement_for_returns(if_statement_struct& if_statement) {
    auto* const scope = if_statement.body;
    static auto& ir = *scope->get_ir();
    // check body
    bool res = function_returns_in_all_paths(if_statement.body);
    if (if_statement.else_if >= 0) {
        return res && check_if_statement_for_returns(ir.if_statements[if_statement.else_if]);
    } else if (if_statement.else_body != nullptr) {
        return res && function_returns_in_all_paths(if_statement.else_body);
    }
    return res;
}

bool function_returns_in_all_paths(identifier_scopes* cur_scope) {
    static intermediate_representation& ir = *cur_scope->get_ir();
    for (size_t order_index = 0; order_index < cur_scope->order.size(); order_index++) {
        const auto& cur_statement = cur_scope->order[order_index];
        switch (cur_statement.type) {
        case statement_type::IF:
            if (check_if_statement_for_returns(ir.if_statements[cur_statement.index])) {
                // all possible if statements return so there's no point in looking further
                return true;
            }
        case statement_type::RETURN:
            return true;
        default:
            continue;
        }
    }
    return false;
}

void handle_function(function_detail& fd, std::string& function_name, int order_index) {
    static std::unordered_set<std::string> checked_functions;
    static auto& ir = *fd.body->get_ir();
    if (fd.body->level != 1) {
        throw std::runtime_error("functions must be defined in global scope");
    }
    check_function_body(fd.body);
    if (fd.return_type.type == types::UNKNOWN_TYPE) {
        fd.return_type = {types::VOID_TYPE};
    }
    if (fd.return_type.type != types::VOID_TYPE && !function_returns_in_all_paths(fd.body)) {
        throw std::runtime_error("function must return a value in all control paths");
    }
    ir.scopes.identifiers[function_name].order_references.push_back({&ir.scopes, order_index});
    if (!checked_functions.count(function_name)) {
        // make sure all overloads are unique
        std::unordered_set<std::string> set;
        for (const size_t& fi : ir.scopes.identifiers[function_name].type.function_info) {
            std::string res = "";
            for (const std::string& s : ir.function_info[fi].parameter_list) {
                types t = ir.function_info[fi].body->identifiers[s].type.type;
                if (t == types::ARRAY_TYPE) {
                    res += "a" + std::to_string(ir.function_info[fi].body->identifiers[s].type.dimension);
                    t = ir.function_info[fi].body->identifiers[s].type.array_type;
                }
                switch (t) {
                case types::BOOL_TYPE:
                    res += "b";
                    break;
                case types::DOUBLE_TYPE:
                    res += "d";
                    break;
                case types::INT_TYPE:
                    res += "i";
                    break;
                case types::STRING_TYPE:
                    res += "s";
                    break;
                default:
                    throw std::runtime_error("invalid type");
                }
            }
            if (set.count(res)) {
                throw std::runtime_error("overload must differ in parameters");
            }
            set.insert(res);
        }
        checked_functions.insert(function_name);
    }
}

void handle_assignment(identifier_scopes* scope, std::string& name, int index, int order_ind) {
    static intermediate_representation& ir = *scope->get_ir();
    int ind = scope->assignments[name][index].second;
    auto& id = get_identifier_definition(scope, name, -1);
    id.assignment_references.insert(scope);
    if (order_ind >= 0) {
        id.order_references.push_back({scope, order_ind});
    }
    if (!evaluate_expression(scope, scope->assignments[name][index].first, -1)
             .is_assignable_to(evaluate_expression(scope, ir.expressions[ind], ind))) {
        throw std::runtime_error("expression is not assignable to identifier");
    }
}

void check_statement(identifier_scopes* cur, size_t order_index) {
    static int last_func_index = -1;
    static int in_loop = 0;
    static intermediate_representation& ir = *cur->get_ir();
    auto& cur_statement = cur->order[order_index];
    switch (cur_statement.type) {
    case statement_type::FOR:
        in_loop++;
        handle_for_statement(ir.for_statements[cur_statement.index], cur_statement.index);
        in_loop--;
        break;
    case statement_type::WHILE:
        in_loop++;
        handle_while_statement(ir.while_statements[cur_statement.index]);
        in_loop--;
        break;
    case statement_type::IF:
        handle_if_statement(ir.if_statements[cur_statement.index]);
        break;
    case statement_type::BREAK:
    case statement_type::CONTINUE:
        if (in_loop == 0) {
            throw std::runtime_error("can only break/continue inside loop");
        }
        break;
    case statement_type::RETURN:
        if (last_func_index == -1) {
            throw std::runtime_error("can only return inside function body");
        }
        handle_return_statement(ir.function_info[last_func_index], cur_statement.index);
        break;
    case statement_type::FUNCTION: {
        int prev_value = last_func_index;
        last_func_index = cur_statement.index;
        handle_function(ir.function_info[cur_statement.index], cur_statement.identifier_name, order_index);
        last_func_index = prev_value;
        break;
    }
    case statement_type::ASSIGNMENT:
        handle_assignment(cur, cur_statement.identifier_name, cur_statement.index, order_index);
        break;
    case statement_type::INITIALIZATION:
        validate_definition(cur, cur->identifiers[cur_statement.identifier_name], order_index);
        break;
    case statement_type::EXPRESSION:
        evaluate_expression(cur, ir.expressions[cur_statement.index], cur_statement.index);
        break;
    }
}

void check_ir(intermediate_representation& ir) {
    identifier_scopes* cur = &ir.scopes;
    for (size_t order_index = 0; order_index < cur->order.size(); order_index++) {
        check_statement(cur, order_index);
    }
}
