#ifndef IRCBOT_UTIL_H
#define IRCBOT_UTIL_H

#include <string>
#include <vector>

namespace StringUtils {
    void split(const std::string &s, char c, std::vector<std::string> &v);
};

#endif //IRCBOT_UTIL_H
