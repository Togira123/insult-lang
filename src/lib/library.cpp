#include "../../include/ir.h"
#include <vector>

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name) {
    if (id_name == "print") {
        static const std::vector<types> ts = {types::BOOL_TYPE, types::DOUBLE_TYPE, types::INT_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["message"] = {{ts[i]}, -1, true, true, true};
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::VOID_TYPE}, {"message"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "size") {
        static const std::vector<full_type> ts = {{types::STRING_TYPE}, {types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["container"] = {ts[i], -1, true, true, true};
                fi[i] = ir.function_info.size();
                ir.function_info.push_back({{types::INT_TYPE}, {"container"}, true, cur_scope});
            }
            has_been_added_to_ir = true;
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "copy") {
        static const std::vector<full_type> ts = {
            {types::STRING_TYPE},
        };
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(1);
        if (!has_been_added_to_ir) {
            auto* cur_scope = ir.scopes.new_scope();
            cur_scope->identifiers["container"] = {{types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}, -1, true, true, true};
            fi[0] = ir.function_info.size();
            ir.function_info.push_back({{types::ARRAY_TYPE, types::UNKNOWN_TYPE, -1}, {"container"}, true, cur_scope});
            has_been_added_to_ir = true;
        }
        static identifier_detail id = {{types::FUNCTION_TYPE, types::UNKNOWN_TYPE, 0, fi}, -1, true, true, true};
        return id;
    } else if (id_name == "ref") {
        static const std::vector<types> ts = {types::INT_TYPE, types::DOUBLE_TYPE, types::BOOL_TYPE, types::STRING_TYPE};
        static bool has_been_added_to_ir = false;
        static std::vector<size_t> fi(ts.size());
        if (!has_been_added_to_ir) {
            for (size_t i = 0; i < ts.size(); i++) {
                auto* cur_scope = ir.scopes.new_scope();
                cur_scope->identifiers["container"] = {{ts[i]}, -1, true, true, true};
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
