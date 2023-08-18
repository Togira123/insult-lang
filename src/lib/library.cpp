#include "../../include/lib/library.h"
#include "../../include/ir.h"
#include <vector>

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name, full_type array_call_container_arg) {
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
    } else if (id_name == "to_string") {
        // convert a number or boolean value to a string
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::INT_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::STRING_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("to_string");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "to_int") {
        // convert a bool, double or string to an int
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::INT_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("to_int");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "to_double") {
        // convert a bool, int or string to a double
        static const std::vector<types> ts = {types::BOOL_TYPE, types::INT_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::DOUBLE_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("to_double");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "array") {
        // create an array with a pre-determined size
        static const std::vector<full_type> ts = {
            {types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}, {types::BOOL_TYPE}, {types::DOUBLE_TYPE}, {types::INT_TYPE}, {types::STRING_TYPE}};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        static identifier_scopes* last_scope;
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.scopes.new_scope();
            cur_scope->identifiers["size"] = {{types::INT_TYPE}, -1, true, true, true};
            cur_scope->identifiers["size"].parameter_index = 0;
            cur_scope->identifiers["size"].function_info_ind = ir.function_info.size();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}, {"size"}, true, cur_scope});
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["size"] = {{types::INT_TYPE}, -1, true, true, true};
                cur_scope->identifiers["size"].parameter_index = 0;
                cur_scope->identifiers["size"].function_info_ind = ir.function_info.size();
                cur_scope->identifiers["container"] = {ts[i], -1, true, true, true};
                cur_scope->identifiers["container"].parameter_index = 1;
                cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                fi[i + 1] = ir.function_info.size();
                if (ts[i].type == types::ARRAY_TYPE) {
                    ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 2}, {"size", "container"}, true, cur_scope});
                } else {
                    ir.function_info.push_back({{types::ARRAY_TYPE, ts[i].type, 1}, {"size", "container"}, true, cur_scope});
                }
                last_scope = cur_scope;
            }
            has_been_added_to_ir = true;
            ir.used_library_functions.insert("array");
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, true, true, true};
        if (id.type.function_info.size() > 6) {
            id.type.function_info.pop_back();
        }
        if (array_call_container_arg.type == types::ARRAY_TYPE) {
            array_call_container_arg.dimension++;
            id.type.function_info.push_back(ir.function_info.size());
            ir.function_info.push_back({array_call_container_arg, {"size", "container"}, true, last_scope});
            return id;
        }
        return id;
    } else {
        throw std::runtime_error("no definition found for " + id_name);
    }
}

std::string generate_print() {
    static std::string result = "void print(bool b){\n\tstd::cout<<b<<std::endl;\n}\n"
                                "void print(double d){\n\tstd::cout<<d<<std::endl;\n}\n"
                                "void print(int i){\n\tstd::cout<<i<<std::endl;\n}\n"
                                "void print(const std::string& s){\n\tstd::cout<<s<<std::endl;\n}\n";
    return result;
}

std::string generate_read_line() {
    static std::string result = "std::string read_line(){\n\tstd::string s;\n\tstd::getline(std::cin,s);\n\treturn s;\n}\n";
    return result;
};

std::string generate_size() {
    static std::string result = "int size(const std::string& s){\n\treturn s.length();\n}\n"
                                "template<typename template_array_size>"
                                "int size(const std::vector<template_array_size>& v){\n\treturn v.size();\n}\n";
    return result;
}

std::string generate_to_int() {
    static std::string result = "int to_int(bool b){\n\treturn (int)b;\n}\n"
                                "int to_int(double d){\n\treturn (int)d;\n}\n"

                                "int to_int(const std::string& s){\n"
                                "\tsize_t p=0;\n"
                                "\tint i=std::stoll(s,&p);\n"
                                "\tif(p!=s.length()){\n"
                                "\t\tthrow std::runtime_error(\"cannot convert\");\n"
                                "\t}\n"
                                "\treturn i;\n"
                                "}\n";
    return result;
}

std::string generate_to_double() {
    static std::string result = "double to_double(bool b){\n\treturn (double)b;\n}\n"
                                "double to_double(int i){\n\treturn (double)i;\n}\n"

                                "double to_double(const std::string& s){\n"
                                "\tsize_t p=0;\n"
                                "\tdouble d=std::stod(s,&p);\n"
                                "\tif(p!=s.length()){\n"
                                "\t\tthrow std::runtime_error(\"cannot convert\");\n"
                                "\t}\n"
                                "\treturn d;\n"
                                "}\n";
    return result;
}

std::string generate_array() {
    static std::string result = "template<typename v>"
                                "std::vector<v> array(int i,v c){\n"
                                "\treturn std::vector<v>(i, c);\n"
                                "}\n"
                                "template<typename v>"
                                "std::vector<v> array(int i){\n"
                                "\treturn std::vector<v>(i);\n"
                                "}\n";
    return result;
}
