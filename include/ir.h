#ifndef HEADER_IR_H
#define HEADER_IR_H
#include "util.h"
#include <deque>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

// unknown is applied if no type can directly be found but it has the potential to be inferred later or used for arrays where the array type is not
// clear (yet). VOID_TYPE is only used to describe functions that don't return anything
enum class types { BOOL_TYPE, DOUBLE_TYPE, INT_TYPE, STRING_TYPE, VOID_TYPE, FUNCTION_TYPE, ARRAY_TYPE, UNKNOWN_TYPE };

enum class node_type { IDENTIFIER, PUNCTUATION, OPERATOR, INT, DOUBLE, BOOL, STRING, FUNCTION_CALL, ARRAY_ACCESS, LIST };

enum class statement_type { FOR, WHILE, IF, BREAK, CONTINUE, RETURN, FUNCTION, ASSIGNMENT, INITIALIZATION, EXPRESSION };

struct function_detail;

struct intermediate_representation;

struct identifier_scopes;

struct full_type {
    types type;
    // only applicable if type == ARRAY
    types array_type;
    int dimension;
    // only applicable if type == FUNCTION
    // points to an item in function_info vector in intermediate_representation struct
    std::vector<size_t> function_info;
    full_type(types t = types::UNKNOWN_TYPE, types at = types::UNKNOWN_TYPE, int d = 0, std::vector<size_t> fi = {})
        : type(t), array_type(at), dimension(d), function_info(fi) {}
    // bool is_assignable_to(const full_type& other);
    static full_type to_type(const std::string& string_type) {
        types type;
        bool is_array = false;
        if (starts_with(string_type, "int")) {
            type = types::INT_TYPE;
            is_array = string_type.size() != 3;
        } else if (starts_with(string_type, "double")) {
            type = types::DOUBLE_TYPE;
            is_array = string_type.size() != 6;
        } else if (starts_with(string_type, "string")) {
            type = types::STRING_TYPE;
            is_array = string_type.size() != 6;
        } else if (starts_with(string_type, "bool")) {
            type = types::BOOL_TYPE;
            is_array = string_type.size() != 4;
        } else {
            throw std::runtime_error("cannot convert type <" + string_type + "> to <types>");
        }
        full_type ft;
        if (is_array) {
            ft.type = types::ARRAY_TYPE;
            ft.array_type = type;
            int i = (int)string_type.size() - 1;
            for (; i >= 1; i--) {
                if (string_type[i] == ']') {
                    if (string_type[--i] != '[') {
                        throw std::runtime_error("cannot convert type <" + string_type + "> to <types>");
                    }
                } else {
                    break;
                }
            }
            ft.dimension = ((int)string_type.size() - 1 - i) / 2;
        } else {
            ft.type = type;
        }
        return ft;
    }
    bool operator==(const full_type& other) {
        return type == types::ARRAY_TYPE      ? (type == other.type && array_type == other.array_type && dimension == other.dimension)
               : type == types::FUNCTION_TYPE ? (type == other.type && function_info == other.function_info)
                                              : type == other.type;
    }
    bool operator!=(const full_type& other) { return !(*this == other); }
    bool operator<(const full_type& other) const {
        return type == other.type && type == types::ARRAY_TYPE
                   ? array_type == other.array_type ? dimension < other.dimension : array_type < other.array_type
                   : type < other.type;
    }
    //  only converts bool, double, int and string
    static full_type to_type(const node_type node) {
        switch (node) {
        case node_type::BOOL:
            return {types::BOOL_TYPE};
        case node_type::DOUBLE:
            return {types::DOUBLE_TYPE};
        case node_type::INT:
            return {types::INT_TYPE};
        case node_type::STRING:
            return {types::STRING_TYPE};
        default:
            throw std::runtime_error("can only convert the types BOOL, DOUBLE, INT & STRING");
        }
    }
    bool is_assignable_to(const full_type& other) {
        switch (type) {
        case types::BOOL_TYPE:
        case types::STRING_TYPE:
        case types::VOID_TYPE:
            return type == other.type;
        case types::DOUBLE_TYPE:
        case types::INT_TYPE:
            return other.type == types::DOUBLE_TYPE || other.type == types::INT_TYPE;
        case types::ARRAY_TYPE:
            return other.type == types::ARRAY_TYPE &&
                   (array_type == other.array_type || (array_type == types::UNKNOWN_TYPE || other.array_type == types::UNKNOWN_TYPE)) &&
                   ((other.array_type == types::UNKNOWN_TYPE ? dimension >= other.dimension : dimension == other.dimension) || dimension == -1 ||
                    other.dimension == -1);
        default:
            return false;
        }
    }
};

struct function_detail {
    full_type return_type;
    // string is enough because the rest of info can be fetched from identifier_scopes::identifiers
    std::vector<std::string> parameter_list;
    bool defined_with_thanks_keyword;
    identifier_scopes* body;
    /* bool is_assignable_to(const function_detail& other) {
        if (!return_type.is_assignable_to(other.return_type)) {
            return false;
        }
        if (parameter_list.size() != other.parameter_list.size()) {
            return false;
        }
        for (unsigned long i = 0; i < parameter_list.size(); i++) {
            if (!parameter_list[i].type.is_assignable_to(other.parameter_list[i].type)) {
                return false;
            }
        }
        return true;
    } */
};

// instead of having full_type here rather have
// an expression tree which describes this identifier's type
struct identifier_detail {
    // will be unknown at the start and determined later, mostly by accessing the referenced expression used at definition time or if not present by
    // looking at other stuff
    full_type type;
    // references the expression that initialized this identifier. Negative if not applicable yet
    int initializing_expression;
    // whether this identifier is constant or not, meaning whether it keeps its initial value or changes it
    // used to optimize code afterwards
    bool is_constant = true;
    // used to check whether this identifier was already defined when traversing the IR
    bool already_defined = false;
    int parameter_index = -1;
    // index to function_info of the function where this parameter belongs
    int function_info_ind = -1;
    // includes indexes of expression_tree where this variable is used
    std::unordered_set<int> references = {};
    // references to order
    std::vector<std::pair<identifier_scopes*, int>> order_references = {};
    std::unordered_set<identifier_scopes*> assignment_references = {};
    std::vector<int> function_call_references = {};
    std::vector<int> array_access_references = {};
    std::vector<int> for_statement_init = {};
    std::vector<int> for_statement_after = {};
};

struct expression_tree {
    std::string node;
    node_type type;
    std::unique_ptr<expression_tree> left;
    std::unique_ptr<expression_tree> right;
    // is empty unless node_type is one of ARRAY_ACCESS or LIST
    // is a pointer because the actual vector is stored in the intermediate_representation struct
    // in either array_accesses or lists
    // index to refer to
    int args_array_access_index = -1;
    int args_function_call_index = -1;
    int args_list_index = -1;
    bool been_renamed = false;
    int index = -1;
    // std::unique_ptr<expression_tree> parent;
    expression_tree(std::string n = "", node_type t = node_type::IDENTIFIER, int ind = -1) : node(n), type(t), left(nullptr), right(nullptr) {
        switch (t) {
        case node_type::ARRAY_ACCESS:
            args_array_access_index = ind;
            break;
        case node_type::FUNCTION_CALL:
            args_function_call_index = ind;
            break;
        case node_type::LIST:
            args_list_index = ind;
        default:
            break;
        }
    }
    expression_tree(node_type t = node_type::IDENTIFIER, std::string n = "")
        : node(n), type(t), left(nullptr), right(nullptr), args_array_access_index(-1), args_function_call_index(-1), args_list_index(-1) {}
    expression_tree(const expression_tree& other)
        : node(other.node), type(other.type), args_array_access_index(other.args_array_access_index),
          args_function_call_index(other.args_function_call_index), args_list_index(other.args_list_index) {
        if (other.left) {
            left = std::make_unique<expression_tree>(*other.left);
        }
        if (other.right) {
            right = std::make_unique<expression_tree>(*other.right);
        }
    }
    bool operator==(const expression_tree& other) {
        return node == other.node && type == other.type && *left == *other.left && *right == *other.right;
    }
};

struct order_struct {
    size_t index;
    statement_type type;
    std::string identifier_name = "";
    bool is_comment = false;
};

struct identifier_scopes {
    // the level of the scope, 0 for global scope
    int level;
    std::vector<order_struct> order;
    // the identifiers declared on that level, with the identifier name as string
    std::unordered_map<std::string, identifier_detail> identifiers;
    // all assignments happening in the current scope (excluding definitions that are assignments too)
    // key of the map is the identifier being assigned to something
    // value of the map is a pair where the first value is the index of an expression tree which describes the identifier used (might be array
    // access), the second value is the index of the expression the identifier is being assigned
    std::unordered_map<std::string, std::vector<std::pair<int, int>>> assignments;
    identifier_scopes(intermediate_representation* ir_pointer, int l = 0, std::unordered_map<std::string, identifier_detail> i = {})
        : level(l), identifiers(i), upper(nullptr), lower({}), index(0), ir(ir_pointer){};
    // used to create a new scope just below the level of this scope
    // returns a pointer to the newly created scope
    identifier_scopes* new_scope() {
        lower.push_back({ir, level + 1});
        lower.back().upper = this;
        lower.back().index = lower.size() - 1;
        return &lower.back();
    }
    // returns a reference to the upper scope
    identifier_scopes* upper_scope(bool delete_current_scope = false) {
        if (level == 0) {
            throw std::runtime_error("global scope is the outermost scope");
        }
        identifier_scopes& val = *upper;
        if (delete_current_scope) {
            if ((size_t)index == val.lower.size() - 1) {
                val.lower.pop_back();
            } else {
                throw std::runtime_error("can only delete last scope");
            }
        }
        return &val;
    }
    std::list<identifier_scopes>& get_lower() { return lower; }
    identifier_scopes& get_upper() const { return *upper; }
    int get_index() const { return index; }
    intermediate_representation* get_ir() const { return ir; }

private:
    identifier_scopes* upper;
    std::list<identifier_scopes> lower;
    // index of this scope in the upper scope's "lower" vector
    int index;
    // the ir this struct is (indirectly) referenced from
    intermediate_representation* ir;
};

// the first value of the args_list is the end of the (array) list in the token list, inclusive
// the vector holds numbers that represent indexes in the intermediate_representation::expressions vector
struct args_list {
    int end_of_array;
    std::vector<size_t> args;
    std::string identifier;
    full_type type = {};
    int matched_overload = -1;
};

struct if_statement_struct {
    // index for the expression condition stored in intermediate_representation::expressions
    int condition = -1;
    // the scope the program enters if the condition is true
    identifier_scopes* body = nullptr;
    // if there is an else if clause it will be referenced here as the next if statement
    int else_if = -1;
    // if there is an else clause its body will be referenced here
    identifier_scopes* else_body = nullptr;
};

struct while_statement_struct {
    // index for the expression condition stored in intermediate_representation::expressions
    int condition = -1;
    // the scope the program enters if the condition is true
    identifier_scopes* body = nullptr;
};

struct for_statement_struct {
    std::string init_statement = "";
    bool init_statement_is_definition;
    int condition = -1;
    std::string after = "";
    int after_index = -1;
    // the scope the program enters if the condition is true
    // also the scope in which the init_statement and after statements are located
    identifier_scopes* body = nullptr;
    // only important if init_statement_is_definition is false because then this represents the index in the assignments[init_statement] vector
    size_t init_index;
};

struct intermediate_representation {
    identifier_scopes scopes;
    // the key is the start of the function call in the token list
    std::unordered_map<int, args_list> function_calls;
    // the key is the start of the array access in the token list
    std::unordered_map<int, args_list> array_accesses;
    // the key is the start of the (array) list in the token list
    std::unordered_map<int, args_list> lists;
    // the set includes the indexes of all "-" and "+" unary operators. This helps to distinguish between regular arithmetic operators and these unary
    // arithmetic operators
    std::unordered_set<int> unary_operator_indexes;
    // holds all expressions that appear in the whole program
    // they can easily be accessed by indexes in this vector
    std::vector<expression_tree> expressions;
    // holds function info for function declarations
    // this is so that there's no unnecessary overhead when storing normal identifiers as these would need to hold empty fields of the function_detail
    // struct. There's no need to store anything else as these will only be accessed indirectly through the full_type struct
    std::deque<function_detail> function_info;
    // stores if statements that appear across all source code
    std::vector<if_statement_struct> if_statements;
    // stores while statements that appear across all source code
    std::vector<while_statement_struct> while_statements;
    // stores for statements that appear across all source code
    std::vector<for_statement_struct> for_statements;
    // stores return statements that appear across all source code
    // the value represents the index of the expression involved in the return statement (or some negative value if no expression was used)
    std::vector<int> return_statements;
    bool has_arrays = false;
    bool has_fast_exponent = false;
    bool has_add_vectors = false;
    // filled after renaming to prevent clashes with two identifiers named the same
    std::unordered_map<std::string, identifier_scopes*> defining_scope_of_identifier;
    // used to store types of top level expressions
    std::unordered_map<int, full_type> top_level_expression_type;
    // pointer to expression_tree is safe here since the vector it's stored in isn't going to be modified
    std::unordered_map<expression_tree*, full_type> array_addition_result;
    // refers to top-level expression_tree so we can safely use index
    std::unordered_set<int> array_addition_expressions;
    // pointer to expression_tree is safe here since the vector it's stored in isn't going to be modified
    std::unordered_map<expression_tree*, full_type> expression_tree_identifier_types;
    identifier_scopes library_func_scopes = {this};
    // used to store the generic arguments of some library functions for code generation
    std::unordered_map<int, full_type> generic_arg_type;
    intermediate_representation(identifier_scopes s, std::unordered_map<int, args_list> f_calls = {},
                                std::unordered_map<int, args_list> a_accesses = {}, std::unordered_map<int, args_list> initial_lists = {},
                                std::unordered_set<int> unary_op_indexes = {}, std::vector<expression_tree> exp = {},
                                std::deque<function_detail> fi = {})
        : scopes(s), function_calls(f_calls), array_accesses(a_accesses), lists(initial_lists), unary_operator_indexes(unary_op_indexes),
          expressions(exp), function_info(fi){};
};

void check_ir(intermediate_representation& ir);
#endif
