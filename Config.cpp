#include "Config.h"

Config::Config(string filename) {
    string cfg;
    string line;
    ifstream f(filename);
    if (f.is_open()) {
        while (getline(f, line)) {
            cfg += line;
        }
        f.close();
    }
    else {
        cerr << filename << " is not open" << endl;
        exit(1);
    }

    this->jsonData = json::parse(cfg);
}