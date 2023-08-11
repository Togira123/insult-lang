#include "../include/generate_code.h"

bool has_string = false;
bool has_vector = false;

std::string data_type_to_string(const full_type& type) {
    switch (type.type) {
    case types::BOOL_TYPE:
        return "bool";
    case types::DOUBLE_TYPE:
        return "double";
    case types::INT_TYPE:
        return "int";
    case types::STRING_TYPE:
        has_string = true;
        return "std::string";
    case types::VOID_TYPE:
        return "void";
    case types::ARRAY_TYPE: {
        has_vector = true;
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

/* std::string generate_definition(const std::string& name, identifier_detail& id) {
    std::string result = data_type_to_string(id.type) + ' ' + name;
    if (id.initialized_with_definition) {

    }
}

std::string generate_for_loop(for_statement_struct& for_statement) {
    std::string result = "for(";
    if (for_statement.init_statement != "") {
        if (for_statement.init_statement_is_definition) {

        }
    }
}

std::string check_statement(identifier_scopes* cur, size_t order_index) {
    static auto& ir = *cur->get_ir();
    auto& cur_statement = cur->order[order_index];
    switch (cur_statement.type) {
    case statement_type::FOR:
        in_loop++;
        handle_for_statement(ir.for_statements[cur_statement.index]);
        in_loop--;
        break;
    case statement_type::WHILE:
        in_loop++;
        handle_while_statement(ir.while_statements[cur_statement.index]);
        in_loop--;
        break;
    case statement_type::IF:
        handle_if_statement(ir.if_statements[cur_statement.index]);
        break;
    case statement_type::BREAK:
    case statement_type::CONTINUE:
        if (in_loop == 0) {
            throw std::runtime_error("can only break/continue inside loop");
        }
        break;
    case statement_type::RETURN:
        if (last_func_index == -1) {
            throw std::runtime_error("can only return inside function body");
        }
        handle_return_statement(ir.function_info[last_func_index], cur_statement.index);
        break;
    case statement_type::FUNCTION: {
        int prev_value = last_func_index;
        last_func_index = cur_statement.index;
        handle_function(ir.function_info[cur_statement.index], cur_statement.identifier_name);
        last_func_index = prev_value;
        break;
    }
    case statement_type::ASSIGNMENT:
        handle_assignment(cur, cur_statement.identifier_name, cur_statement.index);
        break;
    case statement_type::INITIALIZATION:
        validate_definition(cur, cur->identifiers[cur_statement.identifier_name]);
        break;
    case statement_type::EXPRESSION:
        evaluate_expression(cur, ir.expressions[cur_statement.index]);
        break;
    }
}
 */
std::string generate_code(intermediate_representation& ir) {
    std::string result = "";
    if (ir.has_strings) {
        result += "#include <string>\n";
    }
    if (ir.has_arrays) {
        result += "#include <vector>\n";
    }
    result += generate_function_declarations(ir);

    return result;
}
