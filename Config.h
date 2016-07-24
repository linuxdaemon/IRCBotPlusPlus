#ifndef IRCBOT_CONFIG_H
#define IRCBOT_CONFIG_H

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Config {
public:
    Config(std::string filename);


private:
    json jsonData;
};


#endif //IRCBOT_CONFIG_H
