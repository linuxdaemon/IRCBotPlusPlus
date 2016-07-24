#include "Config.h"

Config::Config(std::string filename) {
    std::string cfg;
    std::string line;
    std::ifstream f(filename);
    if (f.is_open()) {
        while (getline(f, line)) {
            cfg += line;
        }
        f.close();
    }
    else {
        std::cerr << filename << " is not open" << std::endl;
        exit(1);
    }

    this->jsonData = json::parse(cfg);
}