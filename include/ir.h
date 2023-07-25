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
    long long dimension;
    // only applicable if type == FUNCTION
    function_detail* function_info;
    bool is_assignable_to(const full_type& other);
};

struct parameter {
    std::string name;
    full_type type;
};

struct function_detail {
    full_type return_type;
    std::vector<parameter> parameter_list;
    bool defined_with_thanks_keyword;
    bool is_assignable_to(const function_detail& other) {
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
    }
};

bool full_type::is_assignable_to(const full_type& other) {
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
}

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
    // std::unique_ptr<expression_tree> parent;
    expression_tree(std::string n = "", node_type t = node_type::IDENTIFIER) : node(n), type(t), left(nullptr), right(nullptr) {}
    expression_tree(const expression_tree& other) : node(other.node), type(other.type) {
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
    std::unordered_map<std::string, function_detail> functions;
    identifier_scopes(intermediate_representation* ir_pointer, int l = 0, std::unordered_map<std::string, identifier_detail> i = {},
                      std::unordered_map<std::string, function_detail> f = {})
        : level(l), identifiers(i), functions(f), upper(nullptr), lower({}), index(0), ir(ir_pointer){};
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

private:
    identifier_scopes* upper;
    std::vector<identifier_scopes> lower;
    // index of this scope in the upper scope's "lower" vector
    int index;
    // the ir this struct is (indirectly) referenced from
    intermediate_representation* ir;
};

struct intermediate_representation {
    identifier_scopes scopes;
    // the key is the start of the function call in the token list
    // the first value of the pair is the end of the function call in the token list, inclusive
    // the string is the value of the function as returned by the function_call() function
    std::unordered_map<int, std::pair<int, std::string>> function_calls;
    // the key is the start of the array access in the token list
    // the first value of the pair is the end of the array access in the token list, inclusive
    // the string is the value of the function as returned by the array_access() function
    std::unordered_map<int, std::pair<int, std::string>> array_accesses;
    // the key is the start of the (array) list in the token list
    // the first value of the pair is the end of the (array) list in the token list, inclusive
    // the string is the value of the function as returned by list_expression() function
    std::unordered_map<int, std::pair<int, std::string>> lists;
    // the set includes the indexes of all "-" and "+" unary operators. This helps to distinguish between regular arithmetic operators and these unary
    // arithmetic operators
    std::unordered_set<int> unary_operator_indexes;
    // holds all expressions that appear in the whole program
    // they can easily be accessed by indexes in this vector
    std::vector<expression_tree> expressions;
    intermediate_representation(identifier_scopes s, std::unordered_map<int, std::pair<int, std::string>> f_calls = {},
                                std::unordered_map<int, std::pair<int, std::string>> a_accesses = {},
                                std::unordered_map<int, std::pair<int, std::string>> initial_lists = {},
                                std::unordered_set<int> unary_op_indexes = {}, std::vector<expression_tree> exp = {})
        : scopes(s), function_calls(f_calls), array_accesses(a_accesses), lists(initial_lists), unary_operator_indexes(unary_op_indexes),
          expressions(exp){};
};
