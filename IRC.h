#ifndef IRCBOT_IRC_H
#define IRCBOT_IRC_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "PracticalSocket.h"
using json = nlohmann::json;

using namespace std;

namespace IRC {
    struct ParsedLine {
        string prefix = "";
        string cmd = "";
        vector<string> params;
    };

    ParsedLine parse(string s);

    class Connection {
    public:
        Connection(string _host, unsigned short _port, json config);

        ~Connection();

        void setNick(string _nick);
        void setUser(string _user);
        void setRealname(string _realname);
        void useNickServ(bool b);
        void setNickServUser(string ns_user);
        void setNickServPass(string pass);

        void connect();

        void shutdown(string s);

        void shutdown();

        void send(string s);

        void cmd(string cmd, vector<string> *args);

        void msg(string target, string msg);

        void readLoop();

        void dataReceived(const char *buffer);

        void process(ParsedLine line);

        TCPSocket *socket;

    private:
        const int RCVBUFSIZE = 1024;
        string host;
        unsigned short port;
        string inputBuffer;

        string nick;
        string user;
        string realname;
        bool _useNickServ;
        string ns_user;
        string ns_pass;
        json config;
    };
}

#endif //IRCBOT_IRC_H

