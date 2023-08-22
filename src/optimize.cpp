#include "../include/optimize.h"
#include "../include/lib/library.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::string& get_next_identifier_name() {
    // list from https://en.cppreference.com/w/cpp/keyword
    static const std::unordered_set<std::string> reserved_cpp_keywords = {
        "add_vectors", // needed in inslt std library
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "atomic_cancel",
        "atomic_commit",
        "atomic_noexcept",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const",
        "consteval",
        "constexpr",
        "constinit",
        "const_cast",
        "continue",
        "co_await",
        "co_return",
        "co_yield",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "reflexpr",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "synchronized",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
        "final",
        "final",
        "override",
        "transaction_safe",
        "transaction_safe_dynamic",
        "import",
        "module",
    };
    static std::string last = "a";
    if (last.back() == 'z') {
        last += 'a';
        if (library_functions.count(last) || reserved_cpp_keywords.count(last)) {
            return get_next_identifier_name();
        }
        return last;
    }
    last.back()++;
    if (library_functions.count(last) || reserved_cpp_keywords.count(last)) {
        return get_next_identifier_name();
    }
    return last;
}

void rename_identifier_in_exp(intermediate_representation& ir, expression_tree& root, const std::string& old_name, const std::string& new_name) {
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS: {
            auto& access = ir.array_accesses[root.args_array_access_index];
            for (const size_t& arg : access.args) {
                rename_identifier_in_exp(ir, ir.expressions[arg], old_name, new_name);
            }
            if (!root.been_renamed && root.node == old_name) {
                root.node = new_name;
                root.been_renamed = true;
            }
            return;
        }
        case node_type::FUNCTION_CALL: {
            auto& call = ir.function_calls[root.args_function_call_index];
            for (const size_t& arg : call.args) {
                rename_identifier_in_exp(ir, ir.expressions[arg], old_name, new_name);
            }
            if (!root.been_renamed && root.node == old_name) {
                root.node = new_name;
                root.been_renamed = true;
            }
            return;
        }
        case node_type::LIST: {
            auto& list = ir.lists[root.args_list_index];
            for (const size_t& arg : list.args) {
                rename_identifier_in_exp(ir, ir.expressions[arg], old_name, new_name);
            }
            return;
        }
        case node_type::IDENTIFIER:
            if (!root.been_renamed) {
                if (root.node == old_name) {
                    root.node = new_name;
                    root.been_renamed = true;
                }
            }
            return;
        default:
            return;
        }
    } else {
        rename_identifier_in_exp(ir, *root.right, old_name, new_name);
        if (root.left != nullptr) {
            rename_identifier_in_exp(ir, *root.left, old_name, new_name);
        }
    }
}

bool includes_function_call(intermediate_representation& ir, expression_tree& root) {
    if (root.type == node_type::OPERATOR) {
        if (root.left == nullptr) {
            return includes_function_call(ir, *root.right);
        }
        return includes_function_call(ir, *root.right) || includes_function_call(ir, *root.left);
    } else if (root.type == node_type::LIST) {
        auto& list = ir.lists[root.args_list_index];
        for (const size_t& arg : list.args) {
            if (includes_function_call(ir, ir.expressions[arg])) {
                return true;
            }
        }
    } else if (root.type == node_type::ARRAY_ACCESS) {
        auto& access = ir.lists[root.args_array_access_index];
        for (const size_t& arg : access.args) {
            if (includes_function_call(ir, ir.expressions[arg])) {
                return true;
            }
        }
    }
    return root.type == node_type::FUNCTION_CALL;
}

void rename_identifiers(identifier_scopes& scope, const bool should_optimize) {
    static auto& ir = *scope.get_ir();
    static std::unordered_map<identifier_scopes*, std::unordered_map<std::string, std::vector<std::pair<int, int>>>> new_names_assignment;
    // for each identifier rename it and also rename all references
    std::unordered_map<std::string, identifier_detail> new_names;
    for (auto& [name, id] : scope.identifiers) {
        if (library_functions.count(name)) {
            continue;
        }
        size_t references_besides_definition = 0;
        if (id.initializing_expression > -1) {
            references_besides_definition = includes_function_call(ir, ir.expressions[id.initializing_expression]) ? 1 : 0;
        }
        std::string& new_name = get_next_identifier_name();
        for (int exp : id.references) {
            rename_identifier_in_exp(ir, ir.expressions[exp], name, new_name);
        }
        references_besides_definition += id.references.size();
        for (auto& [order_scope, order_ind] : id.order_references) {
            order_scope->order[order_ind].identifier_name = new_name;
            if (order_scope->order[order_ind].type != statement_type::INITIALIZATION &&
                order_scope->order[order_ind].type != statement_type::FUNCTION) {
                references_besides_definition++;
            }
        }
        for (identifier_scopes* s : id.assignment_references) {
            new_names_assignment[s][new_name] = std::move(s->assignments[name]);
            for (auto& v : new_names_assignment[s][new_name]) {
                if (!ir.expressions[v.first].been_renamed) {
                    ir.expressions[v.first].node = new_name;
                    ir.expressions[v.first].been_renamed = true;
                }
            }
        }
        references_besides_definition += id.assignment_references.size();
        for (int i : id.function_call_references) {
            ir.function_calls[i].identifier = new_name;
        }
        references_besides_definition += id.function_call_references.size();
        for (int i : id.array_access_references) {
            ir.array_accesses[i].identifier = new_name;
        }
        references_besides_definition += id.array_access_references.size();
        for (int i : id.for_statement_init) {
            ir.for_statements[i].init_statement = new_name;
            if (!ir.for_statements[i].init_statement_is_definition) {
                references_besides_definition++;
            }
        }
        for (int i : id.for_statement_after) {
            ir.for_statements[i].after = new_name;
        }
        references_besides_definition += id.for_statement_after.size();
        if (id.parameter_index >= 0) {
            ir.function_info[id.function_info_ind].parameter_list[id.parameter_index] = new_name;
            // keep parameters
            references_besides_definition++;
        }
        if (references_besides_definition == 0 && should_optimize) {
            for (auto& [order_scope, order_ind] : id.order_references) {
                order_scope->order[order_ind].is_comment = true;
            }
        } else {
            new_names[new_name] = std::move(id);
            ir.defining_scope_of_identifier[new_name] = &scope;
        }
    }
    scope.assignments = std::move(new_names_assignment[&scope]);
    for (auto& s : scope.get_lower()) {
        rename_identifiers(s, should_optimize);
    }
    scope.identifiers = std::move(new_names);
}

bool is_constant_value(const expression_tree& node) {
    switch (node.type) {
    case node_type::BOOL:
    case node_type::DOUBLE:
    case node_type::INT:
    case node_type::STRING:
        return true;
    default:
        return false;
    }
}

bool to_bool(const std::string& s) { return s == "true"; }

void fold_tree(expression_tree& root) {
    if (root.type == node_type::OPERATOR) {
        fold_tree(*root.right);
        bool is_constant_expr = is_constant_value(*root.right);
        if (root.left == nullptr) {
            if (is_constant_expr) {
                if (root.node == "!") {
                    root.node = root.right->node == "true" ? "false" : "true";
                    root.type = node_type::BOOL;
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                } else if (root.node == "+") {
                    root.type = root.right->type;
                    root.node = std::move(root.right->node);
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                } else if (root.node == "-") {
                    root.type = root.right->type;
                    root.node = '-' + std::move(root.right->node);
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                }
            }
        } else {
            fold_tree(*root.left);
            if (is_constant_expr && is_constant_value(*root.left)) {
                if (root.node == "||") {
                    root.type = node_type::BOOL;
                    if (root.right->node == "false") {
                        root.node = root.left->node == "true" ? "true" : "false";
                    } else {
                        root.node = "true";
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "&&") {
                    root.type = node_type::BOOL;
                    if (root.right->node == "false") {
                        root.node = "false";
                    } else {
                        root.node = root.left->node == "true" ? "true" : "false";
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "==") {
                    root.type = node_type::BOOL;
                    root.node = root.right->node == root.left->node ? "true" : "false";
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "!=") {
                    root.type = node_type::BOOL;
                    root.node = root.right->node == root.left->node ? "false" : "true";
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == ">") {
                    switch (root.left->type) {
                    case node_type::BOOL:
                        root.type = node_type::BOOL;
                        root.node = to_bool(root.left->node) > to_bool(root.right->node) ? "true" : "false";
                        break;
                    case node_type::DOUBLE:
                        root.type = node_type::BOOL;
                        root.node = std::stod(root.left->node) >
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::INT:
                        root.type = node_type::BOOL;
                        root.node = std::stoll(root.left->node) >
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::STRING:
                        root.type = node_type::BOOL;
                        root.node = root.left->node > root.right->node ? "true" : "false";
                        break;
                    default:
                        return;
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == ">=") {
                    switch (root.left->type) {
                    case node_type::BOOL:
                        root.type = node_type::BOOL;
                        root.node = to_bool(root.left->node) >= to_bool(root.right->node) ? "true" : "false";
                        break;
                    case node_type::DOUBLE:
                        root.type = node_type::BOOL;
                        root.node = std::stod(root.left->node) >=
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::INT:
                        root.type = node_type::BOOL;
                        root.node = std::stoll(root.left->node) >=
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::STRING:
                        root.type = node_type::BOOL;
                        root.node = root.left->node >= root.right->node ? "true" : "false";
                        break;
                    default:
                        return;
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "<") {
                    switch (root.left->type) {
                    case node_type::BOOL:
                        root.type = node_type::BOOL;
                        root.node = to_bool(root.left->node) < to_bool(root.right->node) ? "true" : "false";
                        break;
                    case node_type::DOUBLE:
                        root.type = node_type::BOOL;
                        root.node = std::stod(root.left->node) <
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::INT:
                        root.type = node_type::BOOL;
                        root.node = std::stoll(root.left->node) <
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::STRING:
                        root.type = node_type::BOOL;
                        root.node = root.left->node < root.right->node ? "true" : "false";
                        break;
                    default:
                        return;
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "<=") {
                    switch (root.left->type) {
                    case node_type::BOOL:
                        root.type = node_type::BOOL;
                        root.node = to_bool(root.left->node) <= to_bool(root.right->node) ? "true" : "false";
                        break;
                    case node_type::DOUBLE:
                        root.type = node_type::BOOL;
                        root.node = std::stod(root.left->node) <=
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::INT:
                        root.type = node_type::BOOL;
                        root.node = std::stoll(root.left->node) <=
                                            (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node))
                                        ? "true"
                                        : "false";
                        break;
                    case node_type::STRING:
                        root.type = node_type::BOOL;
                        root.node = root.left->node <= root.right->node ? "true" : "false";
                        break;
                    default:
                        return;
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "+") {
                    switch (root.left->type) {
                    case node_type::DOUBLE:
                        root.type = node_type::DOUBLE;
                        root.node =
                            std::to_string(std::stod(root.left->node) +
                                           (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node)));
                        break;
                    case node_type::INT:
                        root.type = root.right->type == node_type::DOUBLE ? node_type::DOUBLE : node_type::INT;
                        if (root.right->type == node_type::INT) {
                            root.node = std::to_string(std::stoll(root.left->node) + std::stoll(root.right->node));
                        } else {
                            root.node = std::to_string(std::stoll(root.left->node) + std::stod(root.right->node));
                        }
                        break;
                    case node_type::STRING:
                        root.type = node_type::STRING;
                        root.node = std::move(root.left->node) + std::move(root.right->node);
                        break;
                    default:
                        return;
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "-") {
                    if (root.left->type == node_type::DOUBLE) {
                        root.type = node_type::DOUBLE;
                        root.node =
                            std::to_string(std::stod(root.left->node) -
                                           (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node)));
                    } else {
                        if (root.right->type == node_type::DOUBLE) {
                            root.type = node_type::DOUBLE;
                            root.node = std::to_string(std::stoll(root.left->node) - std::stod(root.right->node));
                        } else {
                            root.type = node_type::INT;
                            root.node = std::to_string(std::stoll(root.left->node) - std::stoll(root.right->node));
                        }
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "*") {
                    if (root.left->type == node_type::DOUBLE) {
                        root.type = node_type::DOUBLE;
                        root.node =
                            std::to_string(std::stod(root.left->node) *
                                           (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node)));
                    } else {
                        if (root.right->type == node_type::DOUBLE) {
                            root.type = node_type::DOUBLE;
                            root.node = std::to_string(std::stoll(root.left->node) * std::stod(root.right->node));
                        } else {
                            root.type = node_type::INT;
                            root.node = std::to_string(std::stoll(root.left->node) * std::stoll(root.right->node));
                        }
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "/") {
                    if (root.left->type == node_type::DOUBLE) {
                        root.type = node_type::DOUBLE;
                        root.node =
                            std::to_string(std::stod(root.left->node) /
                                           (root.right->type == node_type::DOUBLE ? std::stod(root.right->node) : std::stoll(root.right->node)));
                    } else {
                        if (root.right->type == node_type::DOUBLE) {
                            root.type = node_type::DOUBLE;
                            root.node = std::to_string(std::stoll(root.left->node) / std::stod(root.right->node));
                        } else {
                            root.type = node_type::INT;
                            root.node = std::to_string(std::stoll(root.left->node) / std::stoll(root.right->node));
                        }
                    }
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "%") {
                    root.type = node_type::INT;
                    root.node = std::to_string(std::stoll(root.left->node) % std::stoll(root.right->node));
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "^") {
                    // has to be two ints because otherwise this would've been "#"
                    root.type = node_type::DOUBLE;
                    double b = std::stoll(root.left->node);
                    long long e = std::stoll(root.right->node);
                    if (e < 0) {
                        b = 1 / b;
                        e = -e;
                    }
                    double r = 1;
                    while (e > 0) {
                        if (e & 1) {
                            r *= b;
                        }
                        b *= b;
                        e >>= 1;
                    }
                    root.node = std::to_string(r);
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                } else if (root.node == "#") {
                    root.type = node_type::DOUBLE;
                    root.node = std::to_string(std::pow(std::stod(root.left->node), std::stod(root.right->node)));
                    // delete folded part (pointers are indeed deleted because of unique_ptr) https://stackoverflow.com/a/15070946/14553195
                    root.right = nullptr;
                    root.left = nullptr;
                }
            }
        }
    }
}

// evaluates constant expressions and replaces them with the computed value
void constant_folding(intermediate_representation& ir) {
    for (auto& tree : ir.expressions) {
        fold_tree(tree);
    }
}

void optimize(intermediate_representation& ir, bool should_optimize) {
    // rename all identifiers
    rename_identifiers(ir.scopes, should_optimize);
    if (should_optimize) {
        constant_folding(ir);
    }
}
