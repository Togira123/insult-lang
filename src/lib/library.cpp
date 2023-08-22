#include "../../include/lib/library.h"
#include "../../include/ir.h"
#include <set>
#include <vector>

// the last argument, "func_call" is only used to get dynamic overloads of functions with generic arguments, for example push. It is directly used to
// get the parameters it is called with and then construct a matching overload
identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name, full_type type_assigned_to,
                                        std::vector<full_type>* arg_types) {
    if (id_name == "print") {
        // print a value to stdout
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::INT_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["message"] = {{ts[i]}, -1, true, true};
                cur_scope->identifiers["message"].parameter_index = 0;
                cur_scope->identifiers["message"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::VOID_TYPE}, {"message"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["print"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["print"];
    } else if (id_name == "read_line") {
        // reads a line from stdin
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.library_func_scopes.new_scope();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::STRING_TYPE}, {}, true, cur_scope});
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["read_line"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["read_line"];
    } else if (id_name == "size") {
        // obtain the size of a container
        static const std::vector<full_type> ts = {{types::STRING_TYPE}, {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["container"] = {ts[i], -1, true, true};
                cur_scope->identifiers["container"].parameter_index = 0;
                cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::INT_TYPE}, {"container"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["size"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["size"];
    } else if (id_name == "to_string") {
        // convert a number or boolean value to a string
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::INT_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::STRING_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["to_string"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["to_string"];
    } else if (id_name == "to_int") {
        // convert a bool, double or string to an int
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::INT_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["to_int"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["to_int"];
    } else if (id_name == "to_double") {
        // convert a bool, int or string to a double
        static const std::vector<types> ts = {types::BOOL_TYPE, types::INT_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::DOUBLE_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["to_double"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["to_double"];
    } else if (id_name == "array") {
        // create an array with a pre-determined size
        static std::set<full_type> added_overloads;
        static const std::vector<full_type> ts = {
            {types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}, {types::BOOL_TYPE}, {types::DOUBLE_TYPE}, {types::INT_TYPE}, {types::STRING_TYPE}};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size() + 1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.library_func_scopes.new_scope();
            cur_scope->identifiers["size"] = {{types::INT_TYPE}, -1, true, true};
            cur_scope->identifiers["size"].parameter_index = 0;
            cur_scope->identifiers["size"].function_info_ind = ir.function_info.size();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}, {"size"}, true, cur_scope});
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["size"] = {{types::INT_TYPE}, -1, true, true};
                cur_scope->identifiers["size"].parameter_index = 0;
                cur_scope->identifiers["size"].function_info_ind = ir.function_info.size();
                cur_scope->identifiers["container"] = {ts[i], -1, true, true};
                cur_scope->identifiers["container"].parameter_index = 1;
                cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                fi[i + 1] = ir.function_info.size();
                if (ts[i].type == types::ARRAY_TYPE) {
                    ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 2}, {"size", "container"}, true, cur_scope});
                } else {
                    ir.function_info.push_back({{types::ARRAY_TYPE, ts[i].type, 1}, {"size", "container"}, true, cur_scope});
                }
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["array"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, true, true};
        }
        if (type_assigned_to.type == types::ARRAY_TYPE && !added_overloads.count(type_assigned_to)) {
            auto* cur_scope = ir.library_func_scopes.new_scope();
            cur_scope->identifiers["size"] = {{types::INT_TYPE}, -1, true, true};
            cur_scope->identifiers["size"].parameter_index = 0;
            cur_scope->identifiers["size"].function_info_ind = ir.function_info.size();
            ir.library_func_scopes.identifiers["array"].type.function_info.push_back(ir.function_info.size());
            ir.function_info.push_back({type_assigned_to, {"size", "container"}, true, cur_scope});

            full_type container_arg_type = --type_assigned_to.dimension == 0 ? full_type{type_assigned_to.array_type} : type_assigned_to;
            type_assigned_to.dimension++;
            auto* cur_scope2 = ir.library_func_scopes.new_scope();
            cur_scope2->identifiers["size"] = {{types::INT_TYPE}, -1, true, true};
            cur_scope2->identifiers["size"].parameter_index = 0;
            cur_scope2->identifiers["size"].function_info_ind = ir.function_info.size();
            cur_scope2->identifiers["container"] = {container_arg_type, -1, true, true};
            cur_scope2->identifiers["container"].parameter_index = 1;
            cur_scope2->identifiers["container"].function_info_ind = ir.function_info.size();
            ir.library_func_scopes.identifiers["array"].type.function_info.push_back(ir.function_info.size());
            ir.function_info.push_back({type_assigned_to, {"size", "container"}, true, cur_scope2});

            added_overloads.insert(type_assigned_to);
            return ir.library_func_scopes.identifiers["array"];
        }
        return ir.library_func_scopes.identifiers["array"];
    } else if (id_name == "push") {
        // add overloads dynamically, similar to array function
        static std::set<full_type> added_overloads;
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.library_func_scopes.new_scope();
            cur_scope->identifiers["container"] = {{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 2}, -1, true, true};
            cur_scope->identifiers["container"].parameter_index = 0;
            cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
            cur_scope->identifiers["element"] = {{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}, -1, true, true};
            cur_scope->identifiers["element"].parameter_index = 1;
            cur_scope->identifiers["element"].function_info_ind = ir.function_info.size();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::VOID_TYPE}, {"container", "element"}, true, cur_scope});
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["push"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
            added_overloads.insert({types::ARRAY_TYPE, types::UNKNOWN_TYPE, 2});
        }
        if (arg_types == nullptr) {
            throw std::runtime_error("expected a function call pointer");
        }
        if (arg_types->size() == 2) {
            if (arg_types->at(0).type == types::ARRAY_TYPE && arg_types->at(0).array_type != types::UNKNOWN_TYPE) {
                if (added_overloads.count(arg_types->at(0)) == 0) {
                    auto* cur_scope = ir.library_func_scopes.new_scope();
                    cur_scope->identifiers["container"] = {arg_types->at(0), -1, true, true};
                    cur_scope->identifiers["container"].parameter_index = 0;
                    cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                    full_type ft = arg_types->at(0);
                    if (ft.dimension == 1) {
                        ft = {ft.array_type};
                    } else {
                        ft.dimension--;
                    }
                    cur_scope->identifiers["element"] = {ft, -1, true, true};
                    cur_scope->identifiers["element"].parameter_index = 1;
                    cur_scope->identifiers["element"].function_info_ind = ir.function_info.size();

                    ir.library_func_scopes.identifiers["push"].type.function_info.push_back(ir.function_info.size());
                    ir.function_info.push_back({{types::VOID_TYPE}, {"container", "element"}, true, cur_scope});
                    added_overloads.insert(arg_types->at(0));
                }
            } else if (arg_types->at(0).type == types::UNKNOWN_TYPE && arg_types->at(1).type != types::UNKNOWN_TYPE) {
                identifier_scopes* cur_scope = nullptr;
                full_type container_type = arg_types->at(1);
                if (arg_types->at(1).type == types::ARRAY_TYPE && arg_types->at(1).array_type != types::UNKNOWN_TYPE) {
                    cur_scope = ir.library_func_scopes.new_scope();
                    container_type.dimension++;
                } else if (arg_types->at(1).type != types::UNKNOWN_TYPE) {
                    cur_scope = ir.library_func_scopes.new_scope();
                    container_type = {types::ARRAY_TYPE, container_type.type, 1};
                } else {
                    return ir.library_func_scopes.identifiers["push"];
                }
                if (added_overloads.count(container_type) == 0) {
                    cur_scope->identifiers["container"] = {container_type, -1, true, true};
                    cur_scope->identifiers["container"].parameter_index = 0;
                    cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
                    cur_scope->identifiers["element"] = {arg_types->at(1), -1, true, true};
                    cur_scope->identifiers["element"].parameter_index = 1;
                    cur_scope->identifiers["element"].function_info_ind = ir.function_info.size();

                    ir.library_func_scopes.identifiers["push"].type.function_info.push_back(ir.function_info.size());
                    ir.function_info.push_back({{types::VOID_TYPE}, {"container", "element"}, true, cur_scope});
                    added_overloads.insert(container_type);
                }
            }
        }
        return ir.library_func_scopes.identifiers["push"];
    } else if (id_name == "pop") {
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.library_func_scopes.new_scope();
            cur_scope->identifiers["container"] = {{types::ARRAY_TYPE, types::UNKNOWN_TYPE, 1}, -1, true, true};
            cur_scope->identifiers["container"].parameter_index = 0;
            cur_scope->identifiers["container"].function_info_ind = ir.function_info.size();
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::VOID_TYPE}, {"container"}, true, cur_scope});
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["pop"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["pop"];
    } else if (id_name == "sqrt") {
        static const std::vector<types> ts = {types::DOUBLE_TYPE, types::INT_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.library_func_scopes.new_scope();
                cur_scope->identifiers["value"] = {{ts[i]}, -1, true, true};
                cur_scope->identifiers["value"].parameter_index = 0;
                cur_scope->identifiers["value"].function_info_ind = ir.function_info.size();
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::DOUBLE_TYPE}, {"value"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
            ir.library_func_scopes.identifiers["sqrt"] = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true};
        }
        return ir.library_func_scopes.identifiers["sqrt"];
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

std::string generate_push() {
    static std::string result = "template<typename t>"
                                "void push(std::vector<t>&& v,t e){\n"
                                "\tv.push_back(e);\n"
                                "}\n"
                                "template <typename t>"
                                "void push(std::vector<t>& v,t e){\n"
                                "\tv.push_back(e);\n"
                                "}\n";
    return result;
}

std::string generate_pop() {
    static std::string result = "template<typename t>"
                                "void pop(std::vector<t>&& v){\n"
                                "\tv.pop_back();\n"
                                "}\n"
                                "template<typename t>"
                                "void pop(std::vector<t>& v){\n"
                                "\tv.pop_back();\n"
                                "}\n";
    return result;
}
