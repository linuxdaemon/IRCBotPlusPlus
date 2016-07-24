#include <iostream>
#include <thread>
#include "IRC.h"
#include "Util.h"

IRC::ParsedLine IRC::parse(string s) {
    IRC::ParsedLine p;

    // Prefix
    if (s.front() == ':') {
        p.prefix = s.substr(0, s.find(' '));
        s = s.substr((s.find(' ') + 1));
    }

    // Command
    p.cmd = s.substr(0, s.find(' '));
    s = s.substr((s.find(' ') + 1));

    // Params
    StringUtils::split(s, ' ', p.params);
    int trailStart = -1;

    for (int i = 0; i < p.params.size(); i++) {
        if (p.params[i].front() == ':' && trailStart < 0) {
            trailStart = i;
            break;
        }
    }

    if (trailStart >= 0) {
        while ((p.params.size() - 1) > trailStart) {
            p.params[trailStart] += " " + p.params[trailStart + 1];
            p.params.erase(p.params.begin() + trailStart + 1);
        }
    }

    return p;
}


IRC::Connection::Connection(string _host, unsigned short _port, json config) {
    this->host = _host;
    this->port = _port;
    this->config = config;
    setNick(config["nick"]);
    setUser(config["user"]);
    setRealname(config["realname"]);
    useNickServ(config["nickserv"]["enabled"]);
    if (_useNickServ) {
        setNickServUser(config["nickserv"]["user"]);
        setNickServPass(config["nickserv"]["pass"]);
    }
}

void IRC::Connection::connect() {
    this->socket = new TCPSocket(host, port);
    cout << "Connected" << endl;
    cout << socket->getLocalAddress() << endl;
    send("NICK " + nick);
    cmd("USER", new vector<string> {user, "3", "*", realname});
}

void IRC::Connection::send(string s) {
    cout << s << endl;
    socket->send((s + "\r\n").c_str(), s.length() + 2);
}

IRC::Connection::~Connection() {
    cout << "Destructor" << endl;
    send("QUIT :Shutting Down.");
    delete(this->socket);
}

void IRC::Connection::readLoop() {
    char dataBuffer[RCVBUFSIZE + 1];
    int bytesReceived;
    while (true) {
        if ((bytesReceived = (socket->recv(dataBuffer, RCVBUFSIZE))) <= 0) {
            cerr << "Unable to read";
            return;
        }
        dataBuffer[bytesReceived] = '\0';        // Terminate the string!
        dataReceived(dataBuffer);
    }
}

void IRC::Connection::dataReceived(const char *buffer) {
    inputBuffer.append(buffer);
    string line;
    unsigned long pos;

    while ((pos = inputBuffer.find("\r\n")) != string::npos) {
        line = inputBuffer.substr(0, pos);
        inputBuffer = inputBuffer.substr(pos + 2);
        cout << line << endl;
        ParsedLine pLine = parse(line);
        if (pLine.cmd == "PING")
        {
            send("PONG " + pLine.params[pLine.params.size() - 1]);
            continue;
        }
        process(pLine);
    }
}

void IRC::Connection::process(ParsedLine line) {
    if (line.cmd == "004") {
        cout << "Connected!!" << endl;
        cmd("MODE", new vector<string> {nick, "+x"});

        if (_useNickServ)
            msg("NICKSERV", "IDENTIFY " + ns_user + " " + ns_pass);
        this_thread::sleep_for(chrono::milliseconds(500));

        for (string chan : config["channels"]) {
            cmd("JOIN", new vector<string> {chan});
        }
    } else if (line.cmd == "PRIVMSG") {
        if (line.params[line.params.size() - 1].substr(1) == "-~=quit") {
            shutdown();
        }
    }
}

void IRC::Connection::setNick(string _nick) {
    nick = _nick;
}

void IRC::Connection::setUser(string _user) {
    user = _user;
}

void IRC::Connection::setRealname(string _realname) {
    realname = _realname;
}

void IRC::Connection::useNickServ(bool b) {
    _useNickServ = b;
}

void IRC::Connection::setNickServUser(string ns_user) {
    this->ns_user = ns_user;
}

void IRC::Connection::setNickServPass(string pass) {
    this->ns_pass = pass;
}

void IRC::Connection::cmd(string cmd, vector<string> *args) {
    if (args->size() > 0) {
        for (int i = 0; i < args->size(); i++) {
            cmd += " ";
            // Add : before the last parameter
            cmd += (i == (args->size() - 1) ? ":" : "") + (*args)[i];
        }
    }

    send(cmd);
    delete args;
}

void IRC::Connection::msg(string target, string msg) {
    cmd("PRIVMSG", new vector<string> {target, msg});
}

void IRC::Connection::shutdown() {
    shutdown("Bye!");
}

void IRC::Connection::shutdown(string s) {
    cmd("QUIT", new vector<string> {s});
}