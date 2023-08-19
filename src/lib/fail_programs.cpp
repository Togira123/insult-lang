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

std::string get_random_error_message() {
    const static std::vector<std::string> messages = {
        "Runtime Error: Sense of logic not found. Did you check under your keyboard?",
        "Runtime Error: Did you just slam your head on the keyboard? Try typing, not banging",
        "Runtime Error: Did you expect perfection from a computer? You must be new here",
        "Runtime Error: Roses are red, violets are blue, forget to free memory, your RAM's leaking too",
        "Runtime Error: Did you mean to create abstract art? If so, bravo! Otherwise, back to coding school",
        "Runtime Error: Houston, we have a problem: Your code is lost in space (and syntax)",
        "Runtime Error: This code needs a hug. Or maybe a dozen. It's feeling unloved",
        "Runtime Error: It's like you asked a goldfish for directions. Your code seems lost",
        "Runtime Error: Did you summon the coding poltergeist? things are moving spookily wrong",
        "Runtime Error: Did you use a crystal ball instead of a debugger? I'm seeing issues ahead",
        "Runtime Error: Your code's wilder than a roller coaster. Secure your safety belt!",
        "Runtime Error: Did you unleash a code kraken? Tentacles everywhere (and none of them work)",
        "Runtime Error: Did You Trip and Drop Your Code? It's Scattered Like Confetti",
        "Runtime Error: Teach your code some good behavior, it's doing whatever it wants",
        "Runtime Error: Did you copy-paste from the Upside Down? Your code's a bit stranger than usual",
        "Runtime Error: Looks like you unleashed a code kangaroo. It's hopping mad and bouncing errors",
        "Runtime Error: Did you summon the code genie? Your wishes resulted in more bugs, not less",
        "Runtime Error: Your code's like a Jenga tower. One wrong move and it all comes crashing down",
        "Runtime Error: Did you use a fortune cookie to write this? Your code's full of mystical wisdom (and errors)",
        "Runtime Error: Your code's igniting like a firework. Where's the extinguisher?",
        "Runtime Error: Your code's exploring new frontiers of chaos. Time to reel it back in!"};
    std::vector<std::string> result = {};
    std::sample(messages.begin(), messages.end(), std::back_inserter(result), 1, std::mt19937{std::random_device{}()});
    return result[0];
}
