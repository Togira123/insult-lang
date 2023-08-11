#include "../include/optimize.h"
#include <unordered_map>
#include <vector>

std::string& get_next_identifier_name() {
    static std::string last = "a";
    if (last.back() == 'z') {
        return last += 'a';
    }
    last.back()++;
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

void rename_identifiers(identifier_scopes& scope) {
    static auto& ir = *scope.get_ir();
    static std::unordered_map<identifier_scopes*, std::unordered_map<std::string, std::vector<int>>> new_names_assignment;
    // for each identifier rename it and also rename all references
    std::unordered_map<std::string, identifier_detail> new_names;
    for (auto& [name, id] : scope.identifiers) {
        std::string& new_name = get_next_identifier_name();
        for (int exp : id.references) {
            rename_identifier_in_exp(ir.expressions[exp], name, new_name);
        }
        for (auto& [order_scope, order_ind] : id.order_references) {
            order_scope->order[order_ind].identifier_name = new_name;
        }
        for (identifier_scopes* s : id.assignment_references) {
            new_names_assignment[s][new_name] = std::move(s->assignments[name]);
        }
        for (int i : id.function_call_references) {
            ir.function_calls[i].identifier = new_name;
        }
        for (int i : id.array_access_references) {
            ir.array_accesses[i].identifier = new_name;
        }
        for (int i : id.for_statement_init) {
            ir.for_statements[i].init_statement = new_name;
        }
        for (int i : id.for_statement_after) {
            ir.for_statements[i].after = new_name;
        }
        if (id.parameter_index >= 0) {
            ir.function_info[id.function_info_ind].parameter_list[id.parameter_index] = new_name;
        }
        new_names[new_name] = std::move(id);
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
