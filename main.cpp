#include <fstream>
#include <thread>
#include "PracticalSocket.h"
#include "IRC.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    if (argc < 2)
    {
        std::cerr << "Invalid number of args" << std::endl;
        exit(1);
    }
    json config;

    std::string cfg ("");
    std::string line;
    std::ifstream f(argv[1]);
    if (f.is_open()) {
        while (getline(f, line)) {
            cfg += line;
        }
        f.close();
    }
    else {
        std::cerr << argv[1] << " is not open" << std::endl;
        exit(1);
    }

    config = json::parse(cfg);
    json server = config["connections"][0];

    try {
        if (argc >= 3)
            config["connections"][0]["nick"] = argv[2];
        // Establish connection with the server
        IRC::Bot bot(config);
        bot.run();

        std::cout << std::endl;

        // Destructor closes the socket

    } catch (SocketException &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    return 0;
}