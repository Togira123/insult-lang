#include "../include/optimize.h"
#include "../include/lib/library.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::string& get_next_identifier_name() {
    // list from https://en.cppreference.com/w/cpp/keyword
    static const std::unordered_set<std::string> reserved_cpp_keywords = {
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

void rename_identifier_in_exp(expression_tree& root, const std::string& old_name, const std::string& new_name) {
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS:
        case node_type::FUNCTION_CALL:
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
        rename_identifier_in_exp(*root.right, old_name, new_name);
        if (root.left != nullptr) {
            rename_identifier_in_exp(*root.left, old_name, new_name);
        }
    }
}

bool includes_function_call(expression_tree& root) {
    if (root.type == node_type::OPERATOR) {
        if (root.left == nullptr) {
            return includes_function_call(*root.right);
        }
        return includes_function_call(*root.right) || includes_function_call(*root.left);
    }
    return root.type == node_type::FUNCTION_CALL;
}

void rename_identifiers(identifier_scopes& scope) {
    static auto& ir = *scope.get_ir();
    static std::unordered_map<identifier_scopes*, std::unordered_map<std::string, std::vector<int>>> new_names_assignment;
    // for each identifier rename it and also rename all references
    std::unordered_map<std::string, identifier_detail> new_names;
    for (auto& [name, id] : scope.identifiers) {
        if (library_functions.count(name)) {
            continue;
        }
        size_t references_besides_definition = 0;
        if (id.initialized_with_definition) {
            references_besides_definition = includes_function_call(ir.expressions[id.initializing_expression]) ? 1 : 0;
        }
        std::string& new_name = get_next_identifier_name();
        for (int exp : id.references) {
            rename_identifier_in_exp(ir.expressions[exp], name, new_name);
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
        if (references_besides_definition == 0) {
            for (auto& [order_scope, order_ind] : id.order_references) {
                order_scope->order[order_ind].is_comment = true;
            }
        } else {
            new_names[new_name] = std::move(id);
        }
    }
    scope.assignments = std::move(new_names_assignment[&scope]);
    for (auto& s : scope.get_lower()) {
        rename_identifiers(s);
    }
    scope.identifiers = std::move(new_names);
}

void optimize(intermediate_representation& ir) {
    // rename all identifiers
    rename_identifiers(ir.scopes);
}

// evaluates constant expressions and replaces them with the computed value
void constant_folding(intermediate_representation& ir) {}
