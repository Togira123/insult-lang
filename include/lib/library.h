#include <string>
#include <unordered_set>

const std::unordered_set<std::string> library_functions = {"print"};

identifier_detail& identifier_detail_of(intermediate_representation& ir, const std::string& id_name);
