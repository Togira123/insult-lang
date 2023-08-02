#include "util.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

// none is applied if no type can be found which will result in an error
// unknown is applid if no type can directly be found but it has the potential to be inferred later
enum class types { BOOL_TYPE, DOUBLE_TYPE, INT_TYPE, STRING_TYPE, FUNCTION_TYPE, ARRAY_TYPE, UNKNOWN_TYPE, NONE_TYPE };

struct function_detail;

struct intermediate_representation;

struct full_type {
    types type;
    // only applicable if type == ARRAY
    types array_type;
    int dimension;
    // only applicable if type == FUNCTION
    // points to an item in  function_info vector in intermediate_representation struct
    function_detail* function_info;
    full_type(types t = types::UNKNOWN_TYPE, types at = types::UNKNOWN_TYPE, int d = 0, function_detail* fi = nullptr)
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
            int i = (int)string_type.size();
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
};

struct function_detail {
    full_type return_type;
    // string is enough because the rest of info can be fetched from identifier_scopes::identifiers
    std::vector<std::string> parameter_list;
    bool defined_with_thanks_keyword;
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

/* bool full_type::is_assignable_to(const full_type& other) {
    if (type == types::BOOL_TYPE || type == types::STRING_TYPE) {
        return type == other.type;
    } else if (type == types::DOUBLE_TYPE || type == types::INT_TYPE) {
        return other.type == types::DOUBLE_TYPE || other.type == types::INT_TYPE;
    } else if (type == types::FUNCTION_TYPE) {
        return other.type == types::FUNCTION_TYPE && function_info->is_assignable_to(*other.function_info);
    } else if (type == types::ARRAY_TYPE) {
        return other.type == types::ARRAY_TYPE && array_type == other.array_type;
    } else {
        // type == UNKNOWN_TYPE || type == NONE_TYPE
        return false;
    }
} */

// instead of having full_type here rather have
// an expression tree which describes this identifier's type
struct identifier_detail {
    // will be unknown at the start and determined later, mostly by accessing the referenced expression used at definition time or if not present by
    // looking at other stuff
    full_type type;
    // references the expression that initialized this identifier. Negative if not applicable yet
    // if initialized_with_definition is false, this will take the value of the first expression that is assigned to this variable
    int initializing_expression;
    // whether the variable has been initialized during definition
    bool initialized_with_definition;
    // whether this identifier is constant or not, meaning whether it keeps its initial value or changes it
    // used to optimize code afterwards
    bool is_constant = true;
};

enum class node_type { IDENTIFIER, PUNCTUATION, OPERATOR, INT, DOUBLE, BOOL, STRING, FUNCTION_CALL, ARRAY_ACCESS, LIST };

struct expression_tree {
    std::string node;
    node_type type;
    std::unique_ptr<expression_tree> left;
    std::unique_ptr<expression_tree> right;
    // is empty unless node_type is one of ARRAY_ACCESS or LIST
    // is a pointer because the actual vector is stored in the intermediate_representation struct
    // in either array_accesses or lists
    std::vector<int>* args;
    // is empty unless node_type is FUNCTION_CALL
    // is a pointer because the actual vector is stored in the intermediate_representation struct
    // in function_calls
    std::vector<std::vector<int>>* enhanced_args;
    // std::unique_ptr<expression_tree> parent;
    expression_tree(std::string n = "", node_type t = node_type::IDENTIFIER, std::vector<int>* a = nullptr)
        : node(n), type(t), left(nullptr), right(nullptr), args(a), enhanced_args(nullptr) {}
    expression_tree(std::string n = "", node_type t = node_type::IDENTIFIER, std::vector<std::vector<int>>* ea = nullptr)
        : node(n), type(t), left(nullptr), right(nullptr), args(nullptr), enhanced_args(ea) {}
    expression_tree(node_type t = node_type::IDENTIFIER, std::string n = "")
        : node(n), type(t), left(nullptr), right(nullptr), args(nullptr), enhanced_args(nullptr) {}
    expression_tree(const expression_tree& other) : node(other.node), type(other.type), args(other.args), enhanced_args(other.enhanced_args) {
        if (other.left) {
            left = std::make_unique<expression_tree>(*other.left);
        }
        if (other.right) {
            right = std::make_unique<expression_tree>(*other.right);
        }
    }
    // TODO: update this to return true/false based on the expression after it is evaluated
    bool operator==(const expression_tree& other) {
        return node == other.node && type == other.type && *left == *other.left && *right == *other.right;
    }
};

struct identifier_scopes {
    // the level of the scope, 0 for global scope
    int level;
    // the identifiers declared on that level, with the identifier name as string
    std::unordered_map<std::string, identifier_detail> identifiers;
    // all assignments happening in the current scope (excluding definitions that are assignments too)
    // key of the map is the identifier being assigned to something
    // value of the map is the index of the expression the identifier is being assigned
    std::unordered_map<std::string, std::vector<int>> assignments;
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
        if (delete_current_scope) {
            identifier_scopes& val = *upper;
            if ((unsigned long)index == val.lower.size() - 1) {
                val.lower.pop_back();
            } else {
                val.lower.erase(val.lower.begin() + index);
            }
        }
        return upper;
    }
    const std::vector<identifier_scopes>& lower_scopes() { return lower; }
    intermediate_representation* get_ir() { return ir; }

private:
    identifier_scopes* upper;
    std::vector<identifier_scopes> lower;
    // index of this scope in the upper scope's "lower" vector
    int index;
    // the ir this struct is (indirectly) referenced from
    intermediate_representation* ir;
};

// the first value of the args_list is the end of the (array) list in the token list, inclusive
// the vector holds numbers that represent indexes in the intermediate_representation::expressions vector
struct args_list {
    int end_of_array;
    std::vector<int> args;
    std::string identifier;
};

struct enhanced_args_list {
    int end_of_array;
    std::vector<std::vector<int>> args;
    std::string identifier;
};

struct intermediate_representation {
    identifier_scopes scopes;
    // the key is the start of the function call in the token list
    std::unordered_map<int, enhanced_args_list> function_calls;
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
    std::vector<function_detail> function_info;
    intermediate_representation(identifier_scopes s, std::unordered_map<int, enhanced_args_list> f_calls = {},
                                std::unordered_map<int, args_list> a_accesses = {}, std::unordered_map<int, args_list> initial_lists = {},
                                std::unordered_set<int> unary_op_indexes = {}, std::vector<expression_tree> exp = {},
                                std::vector<function_detail> fi = {})
        : scopes(s), function_calls(f_calls), array_accesses(a_accesses), lists(initial_lists), unary_operator_indexes(unary_op_indexes),
          expressions(exp), function_info(fi){};
};
