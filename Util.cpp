#include <iostream>
#include "Util.h"

void ::StringUtils::split(const std::string &s, char c, std::vector<std::string> &v) {
    std::string::size_type i = 0;
    std::string::size_type j = s.find(c);
    bool first = true;

    while (j != std::string::npos) {
        first = false;
        v.push_back(s.substr(i, j - i));
        i = ++j;
        j = s.find(c, j);

        if (j == std::string::npos)
            v.push_back(s.substr(i, s.length()));
    }

    if (first) {
        v.push_back(s);
    }
}
