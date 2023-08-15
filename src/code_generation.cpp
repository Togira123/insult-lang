#include "../include/generate_code.h"
#include "../include/lib/fail_programs.h"
#include "../include/lib/library.h"

std::string generate_expression(intermediate_representation& ir, expression_tree& root);

std::string generate_statement(identifier_scopes* cur, size_t order_index);

std::string data_type_to_string(const full_type& type) {
    switch (type.type) {
    case types::BOOL_TYPE:
        return "bool";
    case types::DOUBLE_TYPE:
        return "double";
    case types::INT_TYPE:
        return "int";
    case types::STRING_TYPE:
        return "std::string";
    case types::VOID_TYPE:
        return "void";
    case types::ARRAY_TYPE: {
        std::string result = "";
        for (int i = 0; i < type.dimension; i++) {
            result += "std::vector<";
        }
        result += data_type_to_string(type.array_type);
        for (int i = 0; i < type.dimension; i++) {
            result += '>';
        }
        return result;
    }
    default:
        throw std::runtime_error("cannot convert that type to string");
    }
}

std::string generate_library_functions(intermediate_representation& ir) {
    std::string result = "";
    for (const std::string& func : ir.used_library_functions) {
        if (func == "print") {
            result += generate_print();
        } else if (func == "read_line") {
            result += generate_read_line();
        } else if (func == "size") {
            result += generate_size();
        } else if (func == "to_int") {
            result += generate_to_int();
        } else if (func == "to_double") {
            result += generate_to_double();
        }
    }
    return result;
}

std::string generate_function_declarations(intermediate_representation& ir) {
    std::string result = "";
    for (auto& [name, val] : ir.scopes.identifiers) {
        if (val.type.type == types::FUNCTION_TYPE) {
            for (size_t i : val.type.function_info) {
                auto& info = ir.function_info[i];
                result += data_type_to_string(info.return_type);
                result += ' ' + name + '(';
                if (info.parameter_list.size() >= 1) {
                    result += data_type_to_string(info.body->identifiers[info.parameter_list[0]].type) + ' ';
                    result += info.parameter_list[0];
                    for (size_t i = 1; i < info.parameter_list.size(); i++) {
                        result += ',' + data_type_to_string(info.body->identifiers[info.parameter_list[i]].type) + ' ';
                        result += info.parameter_list[i];
                    }
                }
                result += ");\n";
            }
        }
    }
    return result;
}

std::string generate_pow_function() {
    return "double a(double b,int e){\n"
           "\tif(e<0){\n"
           "\t\tb=1/b;\n"
           "\t\te=-e;\n"
           "\t}\n"
           "\tdouble r=1;\n"
           "\twhile(e>0){\n"
           "\t\tif(e&1){\n"
           "\t\t\tr*=b;\n"
           "\t\t}\n"
           "\t\tb*=b;\n"
           "\t\te>>=1;\n"
           "\t}\n"
           "\treturn r;\n"
           "}\n";
}

std::string generate_array_access(intermediate_representation& ir, int array_access_index) {
    auto& access = ir.array_accesses[array_access_index];
    std::string result = access.identifier;
    for (size_t& arg : access.args) {
        result += '[' + generate_expression(ir, ir.expressions[arg]) + ']';
    }
    return result;
}

std::string generate_function_call(intermediate_representation& ir, int function_call_index) {
    auto& call = ir.function_calls[function_call_index];
    std::string result = (call.identifier == "to_string" ? "std::" : "") + call.identifier + '(';
    if (call.args.size() >= 1) {
        result += generate_expression(ir, ir.expressions[call.args[0]]);
        for (size_t i = 1; i < call.args.size(); i++) {
            result += ',' + generate_expression(ir, ir.expressions[call.args[i]]);
        }
    }
    return result + ')';
}

std::string generate_list(intermediate_representation& ir, int list_index) {
    auto& list = ir.lists[list_index];
    std::string result = "{";
    if (list.args.size() >= 1) {
        result += generate_expression(ir, ir.expressions[list.args[0]]);
        for (size_t i = 1; i < list.args.size(); i++) {
            result += ',' + generate_expression(ir, ir.expressions[list.args[i]]);
        }
    }
    return result + '}';
}

std::string generate_expression(intermediate_representation& ir, expression_tree& root) {
    if (root.type != node_type::OPERATOR) {
        switch (root.type) {
        case node_type::ARRAY_ACCESS:
            return generate_array_access(ir, root.args_array_access_index);
        case node_type::FUNCTION_CALL:
            return generate_function_call(ir, root.args_function_call_index);
        case node_type::IDENTIFIER:
            return root.node;
        case node_type::LIST:
            return generate_list(ir, root.args_list_index);
        case node_type::STRING:
            return "std::string(\"" + root.node + "\")";
        default:
            return root.node;
        }
    } else {
        if (root.left == nullptr) {
            return root.node + generate_expression(ir, *root.right);
        } else {
            std::string left = generate_expression(ir, *root.left);
            std::string right = generate_expression(ir, *root.right);
            if (root.node == "^") {
                return "std::pow(" + left + "," + right + ")";
            } else if (root.node == "#") {
                return "a(" + left + "," + right + ")";
            } else {
                return left + root.node + right;
            }
        }
    }
}

std::string generate_definition(identifier_scopes* cur_scope, const std::string& name, identifier_detail& id) {
    static auto& ir = *cur_scope->get_ir();
    std::string result = cur_scope->level == 0 ? name : data_type_to_string(id.type) + ' ' + name;
    if (id.initialized_with_definition) {
        result += '=' + generate_expression(ir, ir.expressions[id.initializing_expression]);
    } else if (cur_scope->level == 0) {
        return "";
    }
    return result;
}

std::string generate_assignment(intermediate_representation& ir, expression_tree& name, expression_tree& exp) {
    return generate_expression(ir, name) + '=' + generate_expression(ir, exp);
}

std::string generate_for_loop(for_statement_struct& for_statement) {
    static auto& ir = *for_statement.body->get_ir();
    std::string result = "for(";
    if (for_statement.init_statement != "") {
        if (for_statement.init_statement_is_definition) {
            result +=
                generate_definition(for_statement.body, for_statement.init_statement, for_statement.body->identifiers[for_statement.init_statement]);
        } else {
            result +=
                generate_assignment(ir, for_statement.body->assignments[for_statement.init_statement][for_statement.init_index].first,
                                    ir.expressions[for_statement.body->assignments[for_statement.init_statement][for_statement.init_index].second]);
        }
    }
    result += ';' + generate_expression(ir, ir.expressions[for_statement.condition]) + ';';
    if (for_statement.after != "") {
        result += generate_assignment(ir, for_statement.body->assignments[for_statement.after][for_statement.after_index].first,
                                      ir.expressions[for_statement.body->assignments[for_statement.after][for_statement.after_index].second]);
    }
    result += "){\n";
    for (size_t order_index = 0; order_index < for_statement.body->order.size(); order_index++) {
        result += generate_statement(for_statement.body, order_index);
    }
    return result + std::string(for_statement.body->level, '\t') + "}\n";
}

std::string generate_while_loop(while_statement_struct& while_statement) {
    static auto& ir = *while_statement.body->get_ir();
    std::string result = "while(" + generate_expression(ir, ir.expressions[while_statement.condition]) + "){\n";
    for (size_t order_index = 0; order_index < while_statement.body->order.size(); order_index++) {
        result += generate_statement(while_statement.body, order_index);
    }
    return result + std::string(while_statement.body->level, '\t') + "}\n";
}

std::string generate_if_statement(if_statement_struct& if_statement) {
    static auto& ir = *if_statement.body->get_ir();
    std::string result = "if(" + generate_expression(ir, ir.expressions[if_statement.condition]) + "){\n";
    for (size_t order_index = 0; order_index < if_statement.body->order.size(); order_index++) {
        result += generate_statement(if_statement.body, order_index);
    }
    result += std::string(if_statement.body->level, '\t') + "}";
    if (if_statement.else_if >= 0) {
        result += "else " + generate_if_statement(ir.if_statements[if_statement.else_if]);
    } else if (if_statement.else_body != nullptr) {
        result += "else{\n";
        for (size_t order_index = 0; order_index < if_statement.else_body->order.size(); order_index++) {
            result += generate_statement(if_statement.else_body, order_index);
        }
        result += std::string(if_statement.else_body->level, '\t') + "}";
    }
    return result + '\n';
}

std::string generate_return_statement(intermediate_representation& ir, size_t return_statement_index) {
    if (ir.return_statements[return_statement_index] < 0) {
        return "return;\n";
    }
    return "return " + generate_expression(ir, ir.expressions[ir.return_statements[return_statement_index]]) + ";\n";
}

std::string generate_function(function_detail& fd, std::string& function_name) {
    std::string result = data_type_to_string(fd.return_type) + ' ' + function_name + '(';
    if (fd.parameter_list.size() >= 1) {
        result += data_type_to_string(fd.body->identifiers[fd.parameter_list[0]].type) + ' ' + fd.parameter_list[0];
        for (size_t i = 1; i < fd.parameter_list.size(); i++) {
            result += ',' + data_type_to_string(fd.body->identifiers[fd.parameter_list[i]].type) + ' ' + fd.parameter_list[i];
        }
    }
    result += "){\n";
    for (size_t order_index = 0; order_index < fd.body->order.size(); order_index++) {
        result += generate_statement(fd.body, order_index);
    }
    return result + std::string(fd.body->level, '\t') + "}\n";
}

std::string generate_global_var_declarations(intermediate_representation& ir, size_t order_index) {
    std::string result = "";
    auto& cur_statement = ir.scopes.order[order_index];
    if (!cur_statement.is_comment && cur_statement.type == statement_type::INITIALIZATION) {
        result += data_type_to_string(ir.scopes.identifiers[cur_statement.identifier_name].type) + ' ' + cur_statement.identifier_name + ";\n";
    }
    return result;
}

std::string generate_function_definitions(intermediate_representation& ir, size_t order_index) {
    std::string result = "";
    auto& cur_statement = ir.scopes.order[order_index];
    if (!cur_statement.is_comment && cur_statement.type == statement_type::FUNCTION) {
        result += generate_function(ir.function_info[cur_statement.index], cur_statement.identifier_name);
    }
    return result;
}

std::string generate_statement(identifier_scopes* cur, size_t order_index) {
    static auto& ir = *cur->get_ir();
    auto& cur_statement = cur->order[order_index];
    if (cur_statement.is_comment) {
        return "";
    }
    std::string offset = std::string(cur->level + 1, '\t');
    switch (cur_statement.type) {
    case statement_type::FOR:
        return offset + generate_for_loop(ir.for_statements[cur_statement.index]);
    case statement_type::WHILE:
        return offset + generate_while_loop(ir.while_statements[cur_statement.index]);
    case statement_type::IF:
        return offset + generate_if_statement(ir.if_statements[cur_statement.index]);
    case statement_type::BREAK:
        return offset + "break;\n";
    case statement_type::CONTINUE:
        return offset + "continue;\n";
    case statement_type::RETURN:
        return offset + generate_return_statement(ir, cur_statement.index);
    case statement_type::FUNCTION:
        // functions are created afterwards
        return "";
    case statement_type::ASSIGNMENT:
        return offset +
               generate_assignment(ir, cur->assignments[cur_statement.identifier_name][cur_statement.index].first,
                                   ir.expressions[cur->assignments[cur_statement.identifier_name][cur_statement.index].second]) +
               ";\n";
    case statement_type::INITIALIZATION: {
        std::string res;
        if ((res = generate_definition(cur, cur_statement.identifier_name, cur->identifiers[cur_statement.identifier_name])) != "") {
            return offset + res + ";\n";
        }
        return "";
    }
    case statement_type::EXPRESSION:
        return offset + generate_expression(ir, ir.expressions[cur_statement.index]) + ";\n";
    }
}

std::string generate_code(intermediate_representation& ir) {
    std::string result = "";
    if (ir.used_library_functions.count("print") || ir.used_library_functions.count("read_line")) {
        result += "#include <iostream>\n";
    }
    result += "#include <string>\n";
    if (ir.has_arrays) {
        result += "#include <vector>\n";
    }
    // make ints very large
    result += "#define int long long\n"
              "namespace generated_code{\n";
    result += generate_function_declarations(ir);
    result += generate_library_functions(ir);
    if (ir.has_fast_exponent) {
        result += generate_pow_function();
    }
    for (size_t order_index = 0; order_index < ir.scopes.order.size(); order_index++) {
        result += generate_global_var_declarations(ir, order_index);
    }
    for (size_t order_index = 0; order_index < ir.scopes.order.size(); order_index++) {
        result += generate_function_definitions(ir, order_index);
    }
    result += "signed main(){\ntry{\n";
    for (size_t order_index = 0; order_index < ir.scopes.order.size(); order_index++) {
        result += generate_statement(&ir.scopes, order_index);
    }
    return result + "return 0;\n}catch(std::exception& e){\nstd::cout<<\"" + get_random_error_message() +
           "\";\nreturn 1;\n}\n}\n}\nsigned main(){generated_code::main();}";
}
