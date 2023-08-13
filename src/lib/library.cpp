#include "../../include/ir.h"
#include <vector>

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name) {
    if (id_name == "print") {
        // print a value to stdout
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::INT_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["message"] = {{ts[i]}, -1, true, true, true};
                cur_scope->identifiers["message"].parameter_index = 0;
                cur_scope->identifiers["message"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::VOID_TYPE}, {"message"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("print");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "read_line") {
        // reads a line from stdin
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.scopes.new_scope();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::STRING_TYPE}, {}, true, cur_scope});
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("read_line");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "size") {
        // obtain the size of a container
        static const std::vector<full_type> ts = {{types::STRING_TYPE}, {types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["container"] = {ts[i], -1, true, true, true};
                cur_scope->identifiers["container"].parameter_index = 0;
                cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::INT_TYPE}, {"container"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("size");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "copy") {
        // return a copy of the given array instead of passing by reference
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.scopes.new_scope();
            cur_scope->identifiers["container"] = {{types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}, -1, true, true, true};
            cur_scope->identifiers["container"].parameter_index = 0;
            cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}, {"container"}, true, cur_scope});
            has_been_added_to_ir = true;
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "ref") {
        // return a reference to the given value instead of a copy
        static const std::vector<types> ts = {types::INT_TYPE, types::DOUBLE_TYPE, types::BOOL_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["container"] = {{ts[i]}, -1, true, true, true};
                cur_scope->identifiers["container"].parameter_index = 0;
                cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{ts[i]}, {"container"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else {
        throw std::runtime_error("no definition found for " + id_name);
    }
}

std::string generate_print() {
    static std::string result = "void print(bool b){\n\tstd::cout<<b<<std::endl;\n}\n"
                                "void print(double d){\n\tstd::cout<<d<<std::endl;\n}\n"
                                "void print(int i){\n\tstd::cout<<i<<std::endl;\n}\n"
                                "void print(std::string s){\n\tstd::cout<<s<<std::endl;\n}\n";
    return result;
}

std::string generate_read_line() {
    static std::string result = "std::string read_line(){\n\tstd::string s;\n\tstd::getline(std::cin,s);\n\treturn s;\n}\n";
    return result;
};

std::string generate_size() {
    static std::string result = "int size(std::string s){\n\treturn s.length();\n}\n"
                                "template<typename template_array_size>"
                                "int size(std::vector<template_array_size> v){\n\treturn v.size();\n}\n";
    return result;
}

// copy and ref don't require actual functions their functiality will be implemented directly in the generated code
