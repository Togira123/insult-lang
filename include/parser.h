#include "ir.h"

struct temp_expr_tree {
    // keeps track of functions or iterations/control flow statements labeled with thanks
    // true if the current body is enclosed in labeled with thanks statement
    bool thanks_flag = false;
    temp_expr_tree(std::vector<expression_tree>& ets, identifier_scopes** current_scope) : expressions(ets), cur_scope(current_scope) {}
    void add_expression_to_ir(expression_tree& et, bool add_directly = false) {
        if (add_directly) {
            expressions.push_back(std::move(et));
        } else {
            tmp_exp_tree.push_back(std::move(et));
        }
    }
    void add_identifier_to_cur_scope(const std::string& identifier, identifier_detail detail, size_t function_info_ind = 0) {
        added_function_info = detail.type.type == types::FUNCTION_TYPE;
        last_identifier = identifier;

        // add it to the IR as usual
        if ((*cur_scope)->identifiers.count(identifier)) {
            if ((*cur_scope)->identifiers[identifier].type.type == types::FUNCTION_TYPE && added_function_info) {
                // function overload is okay
                (*cur_scope)->identifiers[identifier].type.function_info.push_back(function_info_ind);
                added_overload = identifier;
            } else {
                // already has this identifier stored if it is not treated as comment afterwards return false
                expect_identifier_deletion = true;
            }
        } else {
            (*cur_scope)->identifiers[identifier] = std::move(detail);
            if ((*cur_scope)->identifiers[identifier].type.type == types::FUNCTION_TYPE && added_function_info) {
                // function overload is okay
                (*cur_scope)->identifiers[identifier].type.function_info.push_back(function_info_ind);
                added_overload = identifier;
            }
        }
        // but also store the identifier, in case we have to delete it afterwards due to it being part of a non-please statement
        identifier_names.push_back(identifier);
    }
    void add_assignment_to_cur_scope(std::string& identifier, int ind, expression_tree& identifier_tree) {
        last_assignment = identifier;
        // add to IR as usual
        (*cur_scope)->assignments[identifier].push_back({std::move(identifier_tree), ind});
        last_assignment_ind = (*cur_scope)->assignments[identifier].size() - 1;
        // store identifier name in case we need to delete later
        assignments.push_back(identifier);
    }
    void add_function_calls_to_ir(int ind, args_list& list) {
        // add to IR as usual
        (*cur_scope)->get_ir()->function_calls[ind] = std::move(list);
        // also store key in case we have to delete
        function_calls.push_back(ind);
    }
    void add_array_access_to_ir(int ind, args_list list) {
        // add to IR as usual
        (*cur_scope)->get_ir()->array_accesses[ind] = std::move(list);
        // also store key in case we have to delete
        array_access_indexes.push_back(ind);
    }
    void add_list_to_ir(int ind, args_list& list) {
        // add to IR as usual
        (*cur_scope)->get_ir()->lists[ind] = std::move(list);
        // also store key in case we have to delete
        list_indexes.push_back(ind);
    }
    void add_unary_operator_to_ir(int ind) {
        // add to IR as usual
        (*cur_scope)->get_ir()->unary_operator_indexes.insert(ind);
        // also store the ind in case we have to delete later
        unary_op_indexes.push_back(ind);
    }
    size_t last_exp_index() { return expressions.size() + tmp_exp_tree.size() - 1; }
    const std::string& last_identifier_added() { return last_identifier; }
    const std::pair<std::string&, size_t> last_assignment_added() { return {last_assignment, last_assignment_ind}; }
    // first bool is whether the whole thing went successfully
    // second bool is true if it was really discarded, false if it was accepted
    std::pair<bool, bool> discard_expressions(bool force_discard = false) {
        if (!force_discard && thanks_flag) {
            // don't discard
            return {accept_expressions(), false};
        }
        if (added_function_info) {
            (*cur_scope)->get_ir()->function_info.pop_back();
            added_function_info = false;
        }
        if (added_overload != "") {
            (*cur_scope)->identifiers[added_overload].type.function_info.pop_back();
            added_overload = "";
        }
        for (int i = identifier_names.size() - 1; i >= 0; i--) {
            (*cur_scope)->identifiers.erase(identifier_names[i]);
            identifier_names.pop_back();
        }
        for (int i = assignments.size() - 1; i >= 0; i--) {
            (*cur_scope)->assignments.erase(assignments[i]);
            assignments.pop_back();
        }
        for (int i = function_calls.size() - 1; i >= 0; i--) {
            (*cur_scope)->get_ir()->function_calls.erase(function_calls[i]);
            function_calls.pop_back();
        }
        for (int i = unary_op_indexes.size() - 1; i >= 0; i--) {
            (*cur_scope)->get_ir()->unary_operator_indexes.erase(unary_op_indexes[i]);
            unary_op_indexes.pop_back();
        }
        for (int i = list_indexes.size() - 1; i >= 0; i--) {
            (*cur_scope)->get_ir()->lists.erase(list_indexes[i]);
            list_indexes.pop_back();
        }
        for (int i = array_access_indexes.size() - 1; i >= 0; i--) {
            (*cur_scope)->get_ir()->array_accesses.erase(array_access_indexes[i]);
            array_access_indexes.pop_back();
        }
        expect_identifier_deletion = false;
        tmp_exp_tree.clear();
        return {true, true};
    }
    bool accept_expressions() {
        if (expect_identifier_deletion) {
            return false;
        }
        added_function_info = false;
        added_overload = "";
        identifier_names.clear();
        assignments.clear();
        function_calls.clear();
        unary_op_indexes.clear();
        list_indexes.clear();
        array_access_indexes.clear();
        for (int i = 0; i < (int)tmp_exp_tree.size(); i++) {
            expressions.push_back(std::move(tmp_exp_tree[i]));
        }
        tmp_exp_tree.clear();
        return true;
    }

private:
    // temporarily store expressions here until it is determined whether they are part of a please expression or not
    std::vector<expression_tree> tmp_exp_tree;
    // references the expressions in the IR
    std::vector<expression_tree>& expressions;
    // pointer that points to the pointer pointing to current scope
    identifier_scopes** cur_scope;
    // stores identifiers that were defined in that scope
    std::vector<std::string> identifier_names = {};
    // stores assignments that were made in that scope
    std::vector<std::string> assignments = {};
    // whether a function was defined
    bool added_function_info = false;
    std::string added_overload = "";
    // holds function calls in case they have to be deleted again
    // the int is the key in the intermediate_representation::function_calls map
    std::vector<int> function_calls;
    // holds indexes for unary operators
    std::vector<int> unary_op_indexes;
    // holds indexes for lists
    std::vector<int> list_indexes;
    // holds indexes for array accesses
    std::vector<int> array_access_indexes;
    bool expect_identifier_deletion = false;
    std::string last_identifier = "";
    std::string last_assignment = "";
    size_t last_assignment_ind = -1;
};
