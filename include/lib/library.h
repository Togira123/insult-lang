#include "../ir.h"
#include <string>
#include <unordered_set>

const std::unordered_set<std::string> library_functions = {"print",     "read_line", "size", "to_string", "to_int",
                                                           "to_double", "array",     "push", "pop",       "sqrt"};

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name, full_type type_assigned_to = {},
                                        std::vector<full_type>* arg_types = nullptr);

std::string generate_print();

std::string generate_read_line();

std::string generate_size();

std::string generate_to_int();

std::string generate_to_double();

std::string generate_array();

std::string generate_push();

std::string generate_pop();
