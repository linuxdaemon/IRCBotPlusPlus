#ifndef IRCBOT_IRC_H
#define IRCBOT_IRC_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "PracticalSocket.h"
using json = nlohmann::json;

//using namespace std;

namespace IRC {
    struct ParsedLine {
        std::string prefix = "";
        std::string cmd = "";
        std::vector<std::string> params;
    };

    ParsedLine parse(std::string s);

    class Connection {
    public:
        Connection(std::string _host, unsigned short _port, json config);

        ~Connection();

        void setNick(std::string _nick);
        void setUser(std::string _user);
        void setRealname(std::string _realname);
        void useNickServ(bool b);
        void setNickServUser(std::string ns_user);
        void setNickServPass(std::string pass);

        void connect();

        void shutdown(std::string s);

        void shutdown();

        void send(std::string s);

        void cmd(std::string cmd, std::vector<std::string> *args);

        void msg(std::string target, std::string msg);

        void readLoop();

        void dataReceived(const char *buffer);

        void process(ParsedLine line);

        TCPSocket *socket;

    private:
        const int RCVBUFSIZE = 1024;
        std::string host;
        unsigned short port;
        std::string inputBuffer;

        std::string nick;
        std::string user;
        std::string realname;
        bool _useNickServ;
        std::string ns_user;
        std::string ns_pass;
        json config;
    };
}

#endif //IRCBOT_IRC_H

