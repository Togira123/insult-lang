#include "../../include/lib/fail_programs.h"
#include <algorithm>
#include <random>
#include <vector>

std::string get_random_program() {
    const static std::vector<std::string> programs = {
        "#include <iostream>\n"
        "int main(){\n"
        "\tstd::cout<<\"Hello World! (just guessing LOL)\";\n"
        "}\n",
        "#include <iostream>\n"
        "int main(){\n"
        "\tstd::cout<<\"Input a positive integer:\"<<std::endl;\n"
        "\tlong long i;\n"
        "\tstd::cin>>i;\n"
        "\tif(i<=1000){\n"
        "\t\ti=10000000;\n"
        "\t\tstd::cout<<\"Your chosen number was quite small, so I chose a bigger one :)\";\n"
        "\t}\n"
        "\tfor(long long c = 1;c<i;c++){\n"
        "\t\tstd::cout<<c<<\", \";\n"
        "\t}\n"
        "\tstd::cout<<i<<\"\\n\";\n"
        "\tstd::cout<<\"That's what counting to \" + std::to_string(i) + \" looks like!\";\n"
        "}\n",
        "#include <iostream>\n"
        "int main(){\n"
        "\tstd::cout<<\"I give up...\";\n"
        "}\n",
        "#include <iostream>\n"
        "int main(){\n"
        "\tstd::cout<<\"Infinite loop:\\n\";\n"
        "\twhile(true){}\n"
        "}\n",
    };
    std::vector<std::string> result = {};
    std::sample(programs.begin(), programs.end(), std::back_inserter(result), 1, std::mt19937{std::random_device{}()});
    return result[0];
}
