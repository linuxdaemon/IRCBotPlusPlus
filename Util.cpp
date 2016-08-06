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

bool ::StringUtils::Base64Encode(std::string &sIn, std::string &sRet, unsigned int uWrap) {
    const char b64table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    sRet.clear();
    size_t len = sIn.length();
    const unsigned char *input = (const unsigned char *) sIn.c_str();
    unsigned char *output, *p;
    size_t i = 0, mod = len % 3, toalloc;
    toalloc = (len / 3) * 4 + (3 - mod) % 3 + 1 + 8;

    if (uWrap) {
        toalloc += len / 57;
        if (len % 57) {
            toalloc++;
        }
    }

    if (toalloc < len) {
        return 0;
    }

    p = output = new unsigned char[toalloc];

    while (i < len - mod) {
        *p++ = b64table[input[i++] >> 2];
        *p++ = b64table[((input[i - 1] << 4) | (input[i] >> 4)) & 0x3f];
        *p++ = b64table[((input[i] << 2) | (input[i + 1] >> 6)) & 0x3f];
        *p++ = b64table[input[i + 1] & 0x3f];
        i += 2;

        if (uWrap && !(i % 57)) {
            *p++ = '\n';
        }
    }

    if (!mod) {
        if (uWrap && i % 57) {
            *p++ = '\n';
        }
    } else {
        *p++ = b64table[input[i++] >> 2];
        *p++ = b64table[((input[i - 1] << 4) | (input[i] >> 4)) & 0x3f];
        if (mod == 1) {
            *p++ = '=';
        } else {
            *p++ = b64table[(input[i] << 2) & 0x3f];
        }

        *p++ = '=';

        if (uWrap) {
            *p++ = '\n';
        }
    }

    *p = 0;
    sRet = (char *) output;
    delete[] output;
    return true;
}

bool ::StringUtils::Base64Encode(std::string &s, unsigned int uWrap) {
    std::string sCopy(s);
    return ::StringUtils::Base64Encode(sCopy, s, uWrap);
}
