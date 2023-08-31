#include "../include/ir.h"
#include "../include/lib/library.h"
#include <unordered_set>

bool function_returns_in_all_paths(identifier_scopes* cur_scope);

void check_statement(identifier_scopes* cur, size_t order_index, bool has_forbid_library_names_flag);

full_type evaluate_expression(identifier_scopes* cur_scope, int this_exp_ind, int exp_ind, bool has_forbid_library_names_flag,
                              full_type type_assigned_to);

full_type evaluate_expression_recursive(identifier_scopes* cur_scope, expression_tree& root, int this_exp_ind, int exp_ind,
                                        bool has_forbid_library_names_flag, full_type type_assigned_to);

full_type& validate_function_call(identifier_scopes* cur_scope, int function_call, int exp_ind, full_type type_assigned_to,
                                  bool has_forbid_library_names_flag);

full_type validate_array_access(identifier_scopes* cur_scope, int array_access, int exp_ind, full_type type_assigned_to,
                                bool has_forbid_library_names_flag);

full_type validate_list(identifier_scopes* cur_scope, int list_index, int exp_ind, full_type type_assigned_to, bool has_forbid_library_names_flag);

void handle_assignment(identifier_scopes* scope, std::string& name, int index, int order_ind, bool has_forbid_library_names_flag);

// makes sure that the type of identifier is set
// the last argument, "func_call" is only used to get dynamic overloads of functions with generic arguments, for example push. It is directly used to
// get the parameters it is called with and then construct a matching overload
identifier_detail& get_identifier_definition(identifier_scopes* cur_scope, std::string& name, int exp_ind, bool has_forbid_library_names_flag,
                                             full_type type_assigned_to = {}, std::vector<full_type>* func_call = nullptr) {
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
                    evaluate_expression(cur_scope, cur_scope->identifiers[name].initializing_expression,
                                        cur_scope->identifiers[name].initializing_expression, has_forbid_library_names_flag, type_assigned_to);
            }
            if (!ir.has_arrays && cur_scope->identifiers[name].type.type == types::ARRAY_TYPE) {
                ir.has_arrays = true;
            }
            if (exp_ind >= 0) {
                cur_scope->identifiers[name].references.insert(exp_ind);
            }
            if (has_forbid_library_names_flag && library_functions.count(name)) {
                // flag does not allow library names to be used for regular identifiers
                throw std::runtime_error("forbid_library_names flag disallows library function names for identifiers");
            }
            return cur_scope->identifiers[name];
        } else if (cur_scope->level == 0) {
            // no definition found searching all the way up to global scope
            // check if it's a std library name
            auto& id = identifier_detail_of(ir, name, type_assigned_to, func_call);
            ir.library_func_scopes.identifiers[name].references.insert(exp_ind);
            return id;
        } else {
            cur_scope = cur_scope->upper_scope();
        }
    }
}

full_type evaluate_expression(identifier_scopes* cur_scope, int this_exp_ind, int exp_ind, bool has_forbid_library_names_flag,
                              full_type type_assigned_to = {}) {
    static auto& ir = *cur_scope->get_ir();
    expression_tree& root = ir.expressions[this_exp_ind];
    root.index = this_exp_ind;
    full_type t = evaluate_expression_recursive(cur_scope, root, this_exp_ind, exp_ind, has_forbid_library_names_flag, type_assigned_to);
    if (&root == &ir.expressions[exp_ind]) {
        if (type_assigned_to.type != types::UNKNOWN_TYPE && !type_assigned_to.is_assignable_to(t)) {
            throw std::runtime_error("cannot assign expression");
        }
        if (ir.top_level_expression_type.count(exp_ind) && !ir.top_level_expression_type[exp_ind].is_assignable_to(t)) {
            throw std::runtime_error("expression included differs from new one");
        }
        return ir.top_level_expression_type[exp_ind] =
                   type_assigned_to.type == types::UNKNOWN_TYPE ||
                           (type_assigned_to.type == types::ARRAY_TYPE && type_assigned_to.array_type == types::UNKNOWN_TYPE)
                       ? t
                       : type_assigned_to;
    }
    return t;
}

full_type evaluate_expression_recursive(identifier_scopes* cur_scope, expression_tree& root, int this_exp_ind, int exp_ind,
                                        bool has_forbid_library_names_flag, full_type type_assigned_to = {}) {
    static auto& ir = *cur_scope->get_ir();
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS:
            return validate_array_access(cur_scope, root.args_array_access_index, exp_ind, type_assigned_to, has_forbid_library_names_flag);
        case node_type::FUNCTION_CALL:
            return validate_function_call(cur_scope, root.args_function_call_index, exp_ind, type_assigned_to, has_forbid_library_names_flag);
        case node_type::IDENTIFIER:
            return ir.expression_tree_identifier_types[&root] =
                       get_identifier_definition(cur_scope, root.node, exp_ind, has_forbid_library_names_flag, type_assigned_to).type;
        case node_type::LIST:
            return validate_list(cur_scope, root.args_list_index, exp_ind, type_assigned_to, has_forbid_library_names_flag);
        default:
            return full_type::to_type(root.type);
        }
    } else {
        if (root.left == nullptr) {
            full_type right =
                evaluate_expression_recursive(cur_scope, *root.right, this_exp_ind, exp_ind, has_forbid_library_names_flag, type_assigned_to);
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
            full_type left =
                evaluate_expression_recursive(cur_scope, *root.left, this_exp_ind, exp_ind, has_forbid_library_names_flag, type_assigned_to);
            full_type right =
                evaluate_expression_recursive(cur_scope, *root.right, this_exp_ind, exp_ind, has_forbid_library_names_flag, type_assigned_to);
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
                if (left.type != types::BOOL_TYPE && (left.is_assignable_to(right) || right.is_assignable_to(left))) {
                    switch (left.type) {
                    case types::INT_TYPE:
                        return right.type == types::INT_TYPE ? full_type{types::INT_TYPE} : full_type{types::DOUBLE_TYPE};
                    case types::ARRAY_TYPE:
                        ir.has_add_vectors = true;
                        // both arguments are arrays which has to be marked in order to handle them correctly when generating code
                        root.node = "$";
                        ir.array_addition_expressions.insert(exp_ind);
                        return left.array_type == types::UNKNOWN_TYPE
                                   ? right.array_type == types::UNKNOWN_TYPE ? left.dimension > right.dimension ? left : right : right
                                   : left;
                    default:
                        // left == (ARRAY_TYPE || DOUBLE_TYPE || STRING_TYPE)
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

void check_function_body(identifier_scopes* body, bool has_forbid_library_names_flag) {
    static std::unordered_set<identifier_scopes*> checked;
    if (checked.count(body) == 0) {
        for (size_t order_index = 0; order_index < body->order.size(); order_index++) {
            check_statement(body, order_index, has_forbid_library_names_flag);
        }
        checked.insert(body);
    }
}

full_type& validate_function_call(identifier_scopes* cur_scope, int function_call, int exp_ind, full_type type_assigned_to,
                                  bool has_forbid_library_names_flag) {
    static std::unordered_set<identifier_scopes*> function_calls_set;
    static auto& ir = *cur_scope->get_ir();
    auto& call = ir.function_calls[function_call];
    // get function definition
    std::string& name = call.identifier;
    std::vector<full_type> arg_types(call.args.size());
    for (size_t i = 0; i < call.args.size(); i++) {
        arg_types[i] = evaluate_expression(cur_scope, call.args[i], exp_ind, has_forbid_library_names_flag, {types::UNKNOWN_TYPE});
    }
    auto& id = get_identifier_definition(cur_scope, name, exp_ind, has_forbid_library_names_flag, type_assigned_to, &arg_types);
    if (id.type.type != types::FUNCTION_TYPE) {
        throw std::runtime_error(name + " is not a function");
    }
    id.function_call_references.push_back(function_call);
    auto& overloads = id.type.function_info;
    int indirect_match = -1;
    for (size_t i = 0; i < overloads.size(); i++) {
        size_t overload = overloads[i];
        function_detail& fd = ir.function_info[overload];
        if (fd.parameter_list.size() == call.args.size()) {
            // potential match
            int matched = 1;
            for (size_t i = 0; i < call.args.size(); i++) {
                full_type t = evaluate_expression(cur_scope, call.args[i], exp_ind, has_forbid_library_names_flag,
                                                  fd.body->identifiers[fd.parameter_list[i]].type);
                ir.top_level_expression_type[call.args[i]] = t;
                auto& id = fd.body->identifiers[fd.parameter_list[i]];
                if (t != id.type) {
                    if (t.is_assignable_to(id.type)) {
                        matched = 0;
                        continue;
                    }
                    matched = -1;
                    break;
                }
            }
            if (matched <= 0) {
                if (matched == 0) {
                    indirect_match = overload;
                }
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
                check_function_body(fd.body, has_forbid_library_names_flag);
                if (fd.return_type.type == types::UNKNOWN_TYPE) {
                    fd.return_type = {types::VOID_TYPE};
                }
            }
            if (!ir.has_arrays && fd.return_type.type == types::ARRAY_TYPE) {
                ir.has_arrays = true;
            }
            function_calls_set.erase(fd.body);
            call.matched_overload = overload;
            if (fd.return_type.type == types::ARRAY_TYPE && fd.return_type.array_type == types::UNKNOWN_TYPE && name == "array" &&
                ir.library_func_scopes.identifiers.count("array") && call.args.size() == 2) {
                // is library array function. Since it has a unique generic argument type handle separately
                full_type t = evaluate_expression(cur_scope, call.args[1], exp_ind, has_forbid_library_names_flag,
                                                  fd.body->identifiers[fd.parameter_list[1]].type);
                if (t.type == types::ARRAY_TYPE) {
                    t.dimension++;
                    return call.type = std::move(t);
                } else {
                    return call.type = {types::ARRAY_TYPE, t.type, 1};
                }
            }
            return call.type = fd.return_type;
        }
    }
    if (indirect_match > -1) {
        function_detail& fd = ir.function_info[indirect_match];
        if (function_calls_set.count(fd.body)) {
            throw std::runtime_error("(indirect) recursion requires explicitly stated return type");
        }
        function_calls_set.insert(fd.body);
        // finally return type
        // if return type is unknown have to check whole function and go through it following order vector
        // then mark as checked to not double check later on
        if (fd.return_type.type == types::UNKNOWN_TYPE) {
            // this also sets the return type of the function
            check_function_body(fd.body, has_forbid_library_names_flag);
            if (fd.return_type.type == types::UNKNOWN_TYPE) {
                fd.return_type = {types::VOID_TYPE};
            }
        }
        if (!ir.has_arrays && fd.return_type.type == types::ARRAY_TYPE) {
            ir.has_arrays = true;
        }
        function_calls_set.erase(fd.body);
        call.matched_overload = indirect_match;
        if (fd.return_type.type == types::ARRAY_TYPE && fd.return_type.array_type == types::UNKNOWN_TYPE && name == "array" &&
            ir.library_func_scopes.identifiers.count("array") && call.args.size() == 2) {
            // is library array function. Since it has a unique generic argument type handle separately
            full_type t =
                evaluate_expression(cur_scope, call.args[1], exp_ind, has_forbid_library_names_flag, fd.body->identifiers[fd.parameter_list[1]].type);
            if (t.type == types::ARRAY_TYPE) {
                t.dimension++;
                return call.type = std::move(t);
            } else {
                return call.type = {types::ARRAY_TYPE, t.type, 1};
            }
        }
        return call.type = fd.return_type;
    }
    throw std::runtime_error("no matching overload found");
}

full_type validate_array_access(identifier_scopes* cur_scope, int array_access, int exp_ind, full_type type_assigned_to,
                                bool has_forbid_library_names_flag) {
    auto& access = cur_scope->get_ir()->array_accesses[array_access];
    // get definition of identifier
    std::string& name = access.identifier;
    auto& id = get_identifier_definition(cur_scope, name, exp_ind, has_forbid_library_names_flag, type_assigned_to);
    if (id.type.type != types::ARRAY_TYPE && id.type.type != types::STRING_TYPE) {
        throw std::runtime_error(name + " is not an array nor a string");
    }
    id.array_access_references.push_back(array_access);
    types array_type = id.type.array_type;
    // if the identifier is a string
    if (id.type.type == types::STRING_TYPE) {
        if (access.args.size() == 1 &&
            evaluate_expression(cur_scope, access.args[0], exp_ind, has_forbid_library_names_flag, {types::INT_TYPE}).type == types::INT_TYPE) {
            return access.type = {types::STRING_TYPE};
        } else {
            throw std::runtime_error("invalid array access");
        }
    }
    int dimension = id.type.dimension;
    if ((size_t)(dimension + (array_type == types::STRING_TYPE ? 1 : 0)) < access.args.size()) {
        throw std::runtime_error("invalid array access");
    }
    for (const size_t& arg : access.args) {
        if (evaluate_expression(cur_scope, arg, exp_ind, has_forbid_library_names_flag, {types::INT_TYPE}).type != types::INT_TYPE) {
            throw std::runtime_error("array indexes must be integers");
        }
        dimension--;
    }
    if (dimension > 0) {
        return access.type = {types::ARRAY_TYPE, array_type, dimension};
    }
    if (dimension == 0) {
        return access.type = {array_type};
    }
    // dimension == -1
    // array type is STRING_TYPE
    return access.type = {types::STRING_TYPE};
}

full_type validate_list(identifier_scopes* cur_scope, int list_index, int exp_ind, full_type type_assigned_to, bool has_forbid_library_names_flag) {
    auto& list = cur_scope->get_ir()->lists[list_index];
    if (list.args.size() == 0) {
        // unknown array type, assignable to any kind of array
        return list.type = {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1};
    }
    if (list.args.size() == 1) {
        if (type_assigned_to.type == types::ARRAY_TYPE) {
            type_assigned_to = --type_assigned_to.dimension == 0 ? full_type{type_assigned_to.array_type} : type_assigned_to;
        }
        full_type t = evaluate_expression(cur_scope, list.args[0], exp_ind, has_forbid_library_names_flag, type_assigned_to);
        if (t.type == types::ARRAY_TYPE) {
            return list.type = {types::ARRAY_TYPE, t.array_type, t.dimension + 1};
        } else if (t.type == types::BOOL_TYPE || t.type == types::DOUBLE_TYPE || t.type == types::INT_TYPE || t.type == types::STRING_TYPE) {
            return list.type = {types::ARRAY_TYPE, t.type, 1};
        } else {
            throw std::runtime_error("unexpected type");
        }
    }
    if (type_assigned_to.type == types::ARRAY_TYPE) {
        type_assigned_to = --type_assigned_to.dimension == 0 ? full_type{type_assigned_to.array_type} : type_assigned_to;
    }
    full_type expected_type = evaluate_expression(cur_scope, list.args[0], exp_ind, has_forbid_library_names_flag, type_assigned_to);
    if (expected_type.type == types::ARRAY_TYPE) {
        // find non-unknown type
        for (size_t i = 1; i < list.args.size() && expected_type.array_type == types::UNKNOWN_TYPE; i++) {
            full_type tmp;
            if (!(tmp = evaluate_expression(cur_scope, list.args[i], exp_ind, has_forbid_library_names_flag, type_assigned_to))
                     .is_assignable_to(expected_type)) {
                throw std::runtime_error("array types do not match");
            }
            expected_type = tmp;
        }
    }

    for (size_t i = 0; i < list.args.size(); i++) {
        if (!evaluate_expression(cur_scope, list.args[i], exp_ind, has_forbid_library_names_flag, type_assigned_to).is_assignable_to(expected_type)) {
            throw std::runtime_error("array types do not match");
        }
    }
    if (expected_type.type == types::ARRAY_TYPE) {
        return list.type = {types::ARRAY_TYPE, expected_type.array_type, expected_type.dimension + 1};
    }
    return list.type = {types::ARRAY_TYPE, expected_type.type, 1};
}

void validate_definition(identifier_scopes* cur_scope, identifier_detail& id, int order_ind, bool has_forbid_library_names_flag,
                         const std::string& id_name) {
    static auto& ir = *cur_scope->get_ir();
    if (has_forbid_library_names_flag && library_functions.count(id_name)) {
        // flag does not allow library names to be used for regular identifiers
        throw std::runtime_error("forbid_library_names flag disallows library function names for identifiers");
    }
    if (order_ind >= 0) {
        id.order_references.push_back({cur_scope, order_ind});
    }
    if (id.initializing_expression < 0) {
        return;
    }
    if (id.type.type == types::UNKNOWN_TYPE) {
        id.type = evaluate_expression(cur_scope, id.initializing_expression, id.initializing_expression, has_forbid_library_names_flag, id.type);
        if (id.type.type == types::VOID_TYPE || id.type.type == types::FUNCTION_TYPE) {
            throw std::runtime_error("expression not assignable to identifier");
        }
    }
    if (!id.type.is_assignable_to(
            evaluate_expression(cur_scope, id.initializing_expression, id.initializing_expression, has_forbid_library_names_flag, id.type))) {
        throw std::runtime_error("expression not assignable to identifier");
    }
    if (id.type.type == types::ARRAY_TYPE && id.type.array_type == types::UNKNOWN_TYPE) {
        throw std::runtime_error("array type must be set");
    }
    if (!ir.has_arrays && id.type.type == types::ARRAY_TYPE) {
        ir.has_arrays = true;
    }
    id.already_defined = true;
}

void handle_for_statement(for_statement_struct& for_statement, size_t index, bool has_forbid_library_names_flag) {
    auto* const scope = for_statement.body;
    if (for_statement.init_statement != "") {
        // check whether definition/assignment is okay
        if (for_statement.init_statement_is_definition) {
            auto& id = scope->identifiers[for_statement.init_statement];
            validate_definition(scope, id, -1, has_forbid_library_names_flag, for_statement.init_statement);
            id.for_statement_init.push_back(index);
        } else {
            handle_assignment(scope, for_statement.init_statement, for_statement.init_index, -1, has_forbid_library_names_flag);
            auto& id = get_identifier_definition(scope, for_statement.init_statement, -1, has_forbid_library_names_flag);
            id.for_statement_init.push_back(index);
        }
    }
    // make sure condition is a bool value
    if (!evaluate_expression(scope, for_statement.condition, for_statement.condition, has_forbid_library_names_flag)
             .is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    if (for_statement.after != "") {
        // make sure assignment is okay
        handle_assignment(scope, for_statement.after, for_statement.after_index, -1, has_forbid_library_names_flag);
        auto& id = get_identifier_definition(scope, for_statement.init_statement, -1, has_forbid_library_names_flag);
        id.for_statement_after.push_back(index);
    }
    auto& order = scope->order;
    // check body
    for (size_t order_index = 0; order_index < order.size(); order_index++) {
        check_statement(scope, order_index, has_forbid_library_names_flag);
    }
}

void handle_while_statement(while_statement_struct& while_statement, bool has_forbid_library_names_flag) {
    auto* const scope = while_statement.body;
    // make sure condition is a bool value
    if (!evaluate_expression(&scope->get_upper(), while_statement.condition, while_statement.condition, has_forbid_library_names_flag)
             .is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    // check body
    for (size_t order_index = 0; order_index < scope->order.size(); order_index++) {
        check_statement(scope, order_index, has_forbid_library_names_flag);
    }
}

void handle_if_statement(if_statement_struct& if_statement, bool has_forbid_library_names_flag) {
    auto* const scope = if_statement.body;
    static auto& ir = *scope->get_ir();
    // make sure condition is a bool value
    if (!evaluate_expression(&scope->get_upper(), if_statement.condition, if_statement.condition, has_forbid_library_names_flag)
             .is_assignable_to({types::BOOL_TYPE})) {
        throw std::runtime_error("condition must be a bool value");
    }
    // check body
    for (size_t order_index = 0; order_index < scope->order.size(); order_index++) {
        check_statement(scope, order_index, has_forbid_library_names_flag);
    }
    if (if_statement.else_if >= 0) {
        handle_if_statement(ir.if_statements[if_statement.else_if], has_forbid_library_names_flag);
    } else if (if_statement.else_body != nullptr) {
        for (size_t order_index = 0; order_index < if_statement.else_body->order.size(); order_index++) {
            check_statement(if_statement.else_body, order_index, has_forbid_library_names_flag);
        }
    }
}

void handle_return_statement(function_detail& fd, size_t return_statement_index, bool has_forbid_library_names_flag) {
    auto* const scope = fd.body;
    static auto& ir = *scope->get_ir();
    if (fd.return_type.type == types::UNKNOWN_TYPE) {
        // type is not set yet, set it
        if (ir.return_statements[return_statement_index] < 0) {
            fd.return_type = {types::VOID_TYPE};
        } else {
            fd.return_type = evaluate_expression(scope, ir.return_statements[return_statement_index], ir.return_statements[return_statement_index],
                                                 has_forbid_library_names_flag);
        }
    } else {
        // make sure return statement returns type that is assignable to function return value
        if (ir.return_statements[return_statement_index] < 0) {
            if (fd.return_type.type != types::VOID_TYPE) {
                throw std::runtime_error("function return type must match with returned type");
            }
        } else {
            if (!fd.return_type.is_assignable_to(evaluate_expression(scope, ir.return_statements[return_statement_index],
                                                                     ir.return_statements[return_statement_index], has_forbid_library_names_flag))) {
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

void handle_function(function_detail& fd, std::string& function_name, int order_index, bool has_forbid_library_names_flag) {
    static std::unordered_set<std::string> checked_functions;
    static auto& ir = *fd.body->get_ir();
    if (fd.body->level != 1) {
        throw std::runtime_error("functions must be defined in global scope");
    }
    check_function_body(fd.body, has_forbid_library_names_flag);
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

void handle_assignment(identifier_scopes* scope, std::string& name, int index, int order_ind, bool has_forbid_library_names_flag) {
    int ind = scope->assignments[name][index].second;
    auto& id = get_identifier_definition(scope, name, -1, has_forbid_library_names_flag);
    id.assignment_references.insert(scope);
    if (order_ind >= 0) {
        id.order_references.push_back({scope, order_ind});
    }
    full_type type_of_assignee =
        evaluate_expression(scope, scope->assignments[name][index].first, scope->assignments[name][index].first, has_forbid_library_names_flag);
    if (!type_of_assignee.is_assignable_to(evaluate_expression(scope, ind, ind, has_forbid_library_names_flag, type_of_assignee))) {
        throw std::runtime_error("expression is not assignable to identifier");
    }
}

void check_statement(identifier_scopes* cur, size_t order_index, bool has_forbid_library_names_flag) {
    static int last_func_index = -1;
    static int in_loop = 0;
    static intermediate_representation& ir = *cur->get_ir();
    auto& cur_statement = cur->order[order_index];
    switch (cur_statement.type) {
    case statement_type::FOR:
        in_loop++;
        handle_for_statement(ir.for_statements[cur_statement.index], cur_statement.index, has_forbid_library_names_flag);
        in_loop--;
        break;
    case statement_type::WHILE:
        in_loop++;
        handle_while_statement(ir.while_statements[cur_statement.index], has_forbid_library_names_flag);
        in_loop--;
        break;
    case statement_type::IF:
        handle_if_statement(ir.if_statements[cur_statement.index], has_forbid_library_names_flag);
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
        handle_return_statement(ir.function_info[last_func_index], cur_statement.index, has_forbid_library_names_flag);
        break;
    case statement_type::FUNCTION: {
        int prev_value = last_func_index;
        last_func_index = cur_statement.index;
        handle_function(ir.function_info[cur_statement.index], cur_statement.identifier_name, order_index, has_forbid_library_names_flag);
        last_func_index = prev_value;
        break;
    }
    case statement_type::ASSIGNMENT:
        handle_assignment(cur, cur_statement.identifier_name, cur_statement.index, order_index, has_forbid_library_names_flag);
        break;
    case statement_type::INITIALIZATION:
        validate_definition(cur, cur->identifiers[cur_statement.identifier_name], order_index, has_forbid_library_names_flag,
                            cur_statement.identifier_name);
        break;
    case statement_type::EXPRESSION:
        evaluate_expression(cur, cur_statement.index, cur_statement.index, has_forbid_library_names_flag);
        break;
    }
}

full_type get_array_type(intermediate_representation& ir, expression_tree& root) {
    switch (root.type) {
    case node_type::ARRAY_ACCESS:
        return ir.array_accesses[root.args_array_access_index].type;
    case node_type::FUNCTION_CALL:
        return ir.function_calls[root.args_function_call_index].type;
    case node_type::IDENTIFIER:
        return ir.expression_tree_identifier_types[&root];
    case node_type::LIST:
        return ir.lists[root.args_list_index].type;
    case node_type::OPERATOR:
        if (root.node == "$") {
            auto t = get_array_type(ir, *root.left);
            if (t.array_type == types::UNKNOWN_TYPE) {
                return get_array_type(ir, *root.right);
            }
            return t;
        }
        throw std::runtime_error("unexpected type");
    default:
        throw std::runtime_error("unexpected type");
    }
}

void check_expressions_for_unknown_array(intermediate_representation& ir, expression_tree& root, const full_type& expected_type) {
    switch (root.type) {
    case node_type::ARRAY_ACCESS: {
        auto& access = ir.array_accesses[root.args_array_access_index];
        for (const size_t& arg : access.args) {
            check_expressions_for_unknown_array(ir, ir.expressions[arg], {types::INT_TYPE});
        }
        break;
    }
    case node_type::FUNCTION_CALL: {
        auto& call = ir.function_calls[root.args_function_call_index];
        if (root.node == "array" && ir.library_func_scopes.identifiers.count("array")) {
            check_expressions_for_unknown_array(ir, ir.expressions[call.args[0]], {types::INT_TYPE});
            if (call.args.size() == 2) {
                if (call.type.dimension == 1) {
                    check_expressions_for_unknown_array(ir, ir.expressions[call.args[1]], {call.type.array_type});
                } else {
                    full_type t = call.type;
                    t.dimension--;
                    check_expressions_for_unknown_array(ir, ir.expressions[call.args[1]], t);
                }
            }
            if (call.type.array_type == types::UNKNOWN_TYPE) {
                if (expected_type.array_type == types::UNKNOWN_TYPE) {
                    if (call.args.size() == 2 && ir.top_level_expression_type[call.args[1]].type != types::UNKNOWN_TYPE) {
                        if (ir.top_level_expression_type[call.args[1]].type == types::ARRAY_TYPE) {
                            call.type = ir.top_level_expression_type[call.args[1]];
                            if (call.type.array_type == types::UNKNOWN_TYPE) {
                                call.type.array_type = types::BOOL_TYPE;
                            }
                            call.type.dimension++;
                        } else {
                            call.type.type = types::ARRAY_TYPE;
                            call.type.array_type = ir.top_level_expression_type[call.args[1]].type;
                            call.type.dimension = 1;
                        }
                    } else {
                        // default to bool type if the type doesn't matter since bool require the least space
                        call.type.array_type = types::BOOL_TYPE;
                        call.type.dimension = expected_type.dimension;
                    }
                } else {
                    call.type = expected_type;
                }
            }
        } else if (root.node == "size" && ir.library_func_scopes.identifiers.count("size") &&
                   ir.function_info[call.matched_overload].body->identifiers[ir.function_info[call.matched_overload].parameter_list[0]].type.type ==
                       types::ARRAY_TYPE) {
            check_expressions_for_unknown_array(ir, ir.expressions[call.args[0]], {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1});
            if (ir.top_level_expression_type[call.args[0]].array_type == types::UNKNOWN_TYPE) {
                if (ir.top_level_expression_type[call.args[0]].dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {types::BOOL_TYPE};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE, types::BOOL_TYPE,
                                                                          ir.top_level_expression_type[call.args[0]].dimension - 1};
                }
            } else {
                if (ir.top_level_expression_type[call.args[0]].dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {ir.top_level_expression_type[call.args[0]].array_type};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE, ir.top_level_expression_type[call.args[0]].array_type,
                                                                          ir.top_level_expression_type[call.args[0]].dimension - 1};
                }
            }
        } else if (root.node == "push" && ir.library_func_scopes.identifiers.count("push")) {
            function_detail& fd = ir.function_info[call.matched_overload];
            check_expressions_for_unknown_array(ir, ir.expressions[call.args[0]], fd.body->identifiers[fd.parameter_list[0]].type);
            check_expressions_for_unknown_array(ir, ir.expressions[call.args[1]], fd.body->identifiers[fd.parameter_list[1]].type);
            if (fd.body->identifiers[fd.parameter_list[0]].type.array_type == types::UNKNOWN_TYPE) {
                if (fd.body->identifiers[fd.parameter_list[0]].type.dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {types::BOOL_TYPE};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE, types::BOOL_TYPE,
                                                                          fd.body->identifiers[fd.parameter_list[0]].type.dimension - 1};
                }
            } else {
                if (fd.body->identifiers[fd.parameter_list[0]].type.dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {fd.body->identifiers[fd.parameter_list[0]].type.array_type};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE,
                                                                          fd.body->identifiers[fd.parameter_list[0]].type.array_type,
                                                                          fd.body->identifiers[fd.parameter_list[0]].type.dimension - 1};
                }
            }
        } else if (root.node == "pop" && ir.library_func_scopes.identifiers.count("pop")) {
            check_expressions_for_unknown_array(ir, ir.expressions[call.args[0]], {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1});
            if (ir.top_level_expression_type[call.args[0]].array_type == types::UNKNOWN_TYPE) {
                if (ir.top_level_expression_type[call.args[0]].dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {types::BOOL_TYPE};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE, types::BOOL_TYPE,
                                                                          ir.top_level_expression_type[call.args[0]].dimension - 1};
                }
            } else {
                if (ir.top_level_expression_type[call.args[0]].dimension == 1) {
                    ir.generic_arg_type[root.args_function_call_index] = {ir.top_level_expression_type[call.args[0]].array_type};
                } else {
                    ir.generic_arg_type[root.args_function_call_index] = {types::ARRAY_TYPE, ir.top_level_expression_type[call.args[0]].array_type,
                                                                          ir.top_level_expression_type[call.args[0]].dimension - 1};
                }
            }
        } else {
            function_detail& fd = ir.function_info[call.matched_overload];
            for (size_t i = 0; i < call.args.size(); i++) {
                check_expressions_for_unknown_array(ir, ir.expressions[call.args[i]], fd.body->identifiers[fd.parameter_list[i]].type);
            }
        }
        break;
    }
    case node_type::LIST: {
        auto& list = ir.lists[root.args_list_index];
        full_type et = expected_type.array_type == types::UNKNOWN_TYPE ? list.type : expected_type;
        if (--et.dimension == 0) {
            et = {et.array_type};
        }
        for (size_t i = 0; i < list.args.size(); i++) {
            check_expressions_for_unknown_array(ir, ir.expressions[list.args[i]], et);
        }
        break;
    }
    case node_type::OPERATOR: {
        if (root.left == nullptr) {
            check_expressions_for_unknown_array(ir, *root.right, expected_type);
        } else {
            // stands for "+" with two arrays
            if (root.node == "$") {
                check_expressions_for_unknown_array(ir, *root.left, expected_type);
                check_expressions_for_unknown_array(ir, *root.right, expected_type);
                // arrays can be obtained in 4 ways: list, array access, identifier and function call
                auto t1 = get_array_type(ir, *root.left);
                auto t2 = get_array_type(ir, *root.right);
                if (expected_type.array_type == types::UNKNOWN_TYPE) {
                    if (t1.type == types::UNKNOWN_TYPE && t2.type == types::UNKNOWN_TYPE) {
                        full_type t;
                        t.array_type = types::BOOL_TYPE;
                        t.dimension = expected_type.dimension;
                        ir.array_addition_result[&root] = std::move(t);
                        if (root.index >= 0) {
                            ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                        }
                    } else {
                        if (t1.type == types::UNKNOWN_TYPE) {
                            if (t2.type == types::ARRAY_TYPE) {
                                full_type t = t2;
                                if (t.array_type == types::UNKNOWN_TYPE) {
                                    t.array_type = types::BOOL_TYPE;
                                }
                                t.dimension++;
                                ir.array_addition_result[&root] = std::move(t);
                                if (root.index >= 0) {
                                    ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                                }
                            } else {
                                full_type t;
                                t.array_type = t2.type;
                                t.dimension = 1;
                                ir.array_addition_result[&root] = std::move(t);
                                if (root.index >= 0) {
                                    ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                                }
                            }
                        } else {
                            if (t1.type == types::ARRAY_TYPE) {
                                full_type t = t1;
                                if (t.array_type == types::UNKNOWN_TYPE) {
                                    t.array_type = types::BOOL_TYPE;
                                }
                                t.dimension++;
                                ir.array_addition_result[&root] = std::move(t);
                                if (root.index >= 0) {
                                    ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                                }
                            } else {
                                full_type t;
                                t.array_type = t1.type;
                                t.dimension = 1;
                                ir.array_addition_result[&root] = std::move(t);
                                if (root.index >= 0) {
                                    ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                                }
                            }
                        }
                    }
                } else {
                    ir.array_addition_result[&root] = expected_type;
                    if (root.index >= 0) {
                        ir.top_level_expression_type[root.index] = ir.array_addition_result[&root];
                    }
                }
            } else {
                check_expressions_for_unknown_array(ir, *root.right, full_type{types::UNKNOWN_TYPE});
                check_expressions_for_unknown_array(ir, *root.left, full_type{types::UNKNOWN_TYPE});
            }
            break;
        }
    }
    default:
        return;
    }
}

void check_ir(intermediate_representation& ir, const std::unordered_map<compiler_flag, std::string>& flags) {
    identifier_scopes* cur = &ir.scopes;
    for (size_t order_index = 0; order_index < cur->order.size(); order_index++) {
        check_statement(cur, order_index, flags.count(compiler_flag::FORBID_LIBRARY_NAMES));
    }
    // handle generic types stuff with standard library
    if (ir.library_func_scopes.identifiers.count("array")) {
        for (auto& ind : ir.library_func_scopes.identifiers["array"].references) {
            check_expressions_for_unknown_array(ir, ir.expressions[ind], ir.top_level_expression_type[ind]);
        }
    }
    for (auto& ind : ir.array_addition_expressions) {
        check_expressions_for_unknown_array(ir, ir.expressions[ind], ir.top_level_expression_type[ind]);
    }
    if (ir.library_func_scopes.identifiers.count("size")) {
        for (auto& ind : ir.library_func_scopes.identifiers["size"].references) {
            check_expressions_for_unknown_array(ir, ir.expressions[ind], {types::VOID_TYPE});
        }
    }
    if (ir.library_func_scopes.identifiers.count("push")) {
        for (auto& ind : ir.library_func_scopes.identifiers["push"].references) {
            check_expressions_for_unknown_array(ir, ir.expressions[ind], {types::VOID_TYPE});
        }
    }
    if (ir.library_func_scopes.identifiers.count("pop")) {
        for (auto& ind : ir.library_func_scopes.identifiers["pop"].references) {
            check_expressions_for_unknown_array(ir, ir.expressions[ind], {types::VOID_TYPE});
        }
    }
}
