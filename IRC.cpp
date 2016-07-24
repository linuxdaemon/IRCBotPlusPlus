#include <iostream>
#include <thread>
#include "IRC.h"
#include "Util.h"

IRC::ParsedLine IRC::parse(std::string s) {
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


IRC::Connection::Connection(std::string _host, unsigned short _port, json config) {
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
    std::cout << "Connected" << std::endl;
    std::cout << socket->getLocalAddress() << std::endl;
    send("NICK " + nick);
    cmd("USER", new std::vector<std::string> {user, "3", "*", realname});
}

void IRC::Connection::send(std::string s) {
    std::cout << s << std::endl;
    socket->send((s + "\r\n").c_str(), s.length() + 2);
}

IRC::Connection::~Connection() {
    std::cout << "Destructor" << std::endl;
    send("QUIT :Shutting Down.");
    delete(this->socket);
}

void IRC::Connection::readLoop() {
    char dataBuffer[RCVBUFSIZE + 1];
    int bytesReceived;
    while (true) {
        if ((bytesReceived = (socket->recv(dataBuffer, RCVBUFSIZE))) <= 0) {
            std::cerr << "Unable to read";
            return;
        }
        dataBuffer[bytesReceived] = '\0';        // Terminate the string!
        dataReceived(dataBuffer);
    }
}

void IRC::Connection::dataReceived(const char *buffer) {
    inputBuffer.append(buffer);
    std::string line;
    unsigned long pos;

    while ((pos = inputBuffer.find("\r\n")) != std::string::npos) {
        line = inputBuffer.substr(0, pos);
        inputBuffer = inputBuffer.substr(pos + 2);
        std::cout << line << std::endl;
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
        std::cout << "Connected!!" << std::endl;
        cmd("MODE", new std::vector<std::string> {nick, "+x"});

        if (_useNickServ)
            msg("NICKSERV", "IDENTIFY " + ns_user + " " + ns_pass);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        for (std::string chan : config["channels"]) {
            cmd("JOIN", new std::vector<std::string> {chan});
        }
    } else if (line.cmd == "PRIVMSG") {
        if (line.params[line.params.size() - 1].substr(1) == "-~=quit") {
            shutdown();
        }
    }
}

void IRC::Connection::setNick(std::string _nick) {
    nick = _nick;
}

void IRC::Connection::setUser(std::string _user) {
    user = _user;
}

void IRC::Connection::setRealname(std::string _realname) {
    realname = _realname;
}

void IRC::Connection::useNickServ(bool b) {
    _useNickServ = b;
}

void IRC::Connection::setNickServUser(std::string ns_user) {
    this->ns_user = ns_user;
}

void IRC::Connection::setNickServPass(std::string pass) {
    this->ns_pass = pass;
}

void IRC::Connection::cmd(std::string cmd, std::vector<std::string> *args) {
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

void IRC::Connection::msg(std::string target, std::string msg) {
    cmd("PRIVMSG", new std::vector<std::string> {target, msg});
}

void IRC::Connection::shutdown() {
    shutdown("Bye!");
}

void IRC::Connection::shutdown(std::string s) {
    cmd("QUIT", new std::vector<std::string> {s});
}