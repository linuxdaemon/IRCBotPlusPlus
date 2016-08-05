#include <iostream>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <unordered_map>
#include "IRC.h"
#include "Util.h"

void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::string ReplaceString(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

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

std::vector<std::string> split(std::string const &input) {
    std::istringstream buffer(input);
    std::vector<std::string> ret((std::istream_iterator<std::string>(buffer)),
                                 std::istream_iterator<std::string>());
    return ret;
}

IRC::Bot::Bot(json config) {
    this->config = config;
}

bool IRC::Bot::run() {
    for (json conn : this->config["connections"]) {
        std::cout << conn["name"] << std::endl;
        Connection c(conn["connection"]["host"], conn["connection"]["port"], conn, this);
        this->connections.push_back(c);
    }
    for (Connection &co : this->connections) {
        co.connect();
        co.readLoop();
    }
    return false;
}

IRC::Connection::Connection(std::string _host, unsigned short _port, json config, IRC::Bot *bot) {
    this->host = _host;
    this->port = _port;
    this->config = config;
    this->bot = bot;

    setNick(config["nick"]);
    setUser(config["user"]);
    setRealname(config["realname"]);
    // TODO figure out how to get Clion to stop complaining about the index
    useNickServ(config["nickserv"]["enabled"]);
    if (_useNickServ) {
        setNickServUser(config["nickserv"]["user"]);
        setNickServPass(config["nickserv"]["pass"]);
    }
    //json::iterator c;
    //if ((c = config.find("commands")) != json.end()) {
    std::cout << config["commands"] << std::endl;
    auto coms = config["commands"].get<std::unordered_map<std::string, json>>();
    std::cout << "Registering commands: " << std::endl;
    for (auto c : coms) {
        std::cout << c.first << ": " << c.second << std::endl;
        cmds["-~=" + c.first] = c.second;
    }
}

void IRC::Connection::connect() {
    this->socket = new TCPSocket(host, port);
    this->connected = true;
    send("NICK " + nick);
    cmd("USER", new std::vector<std::string>{user, "3", "*", realname});
}

void IRC::Connection::send(std::string s) {
    if (connected)
    {
        std::cout << s << std::endl;
        socket->send((s + "\r\n").c_str(), s.length() + 2);
    } else {
        std::cout << "Attempted to send line while not connected, ignoring" << std::endl;
    }
}

IRC::Connection::~Connection() {
    std::cout << "Destructor" << std::endl;
    send("QUIT :Shutting Down.");
    if (connected) delete (this->socket);
}

void IRC::Connection::readLoop() {
    char dataBuffer[RCVBUFSIZE + 1];
    int bytesReceived;
    while (true) {
        if ((bytesReceived = (socket->recv(dataBuffer, RCVBUFSIZE))) <= 0) {
            std::cerr << "Unable to read" << std::endl;
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
        log(line);
        ParsedLine pLine = parse(line);
        if (pLine.cmd == "PING") {
            send("PONG " + pLine.params[pLine.params.size() - 1]);
            continue;
        }
        process(pLine);
    }
}

void IRC::Connection::process(ParsedLine line) {
    if (line.cmd == "004") {
        std::cout << "Connected!!" << std::endl;
        cmd("MODE", new std::vector<std::string>{nick, "+x"});

        if (_useNickServ)
            msg("NICKSERV", "IDENTIFY " + ns_user + " " + ns_pass);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        for (std::string chan : config["channels"]) {
            cmd("JOIN", new std::vector<std::string>{chan});
        }
    } else if (line.cmd == "PRIVMSG") {
        CommandLine cLine;
        parseMessage(line, cLine);
        std::cout << cLine.command << std::endl;
        std::map<std::string, std::string>::iterator c;

        if (cLine.command == "-~=quit") {
            shutdown();
        } else if (cLine.command == "-~=raw") {
            std::string r;
            for (int i = 1; i < cLine.args.size(); i++) {
                r += " " + cLine.args[i];
            }
            send(r.substr(1));
        } else if (cLine.command == "-~=addcmd") {
            if (cLine.args.size() > 2) {
                std::string r;
                for (int i = 2; i < cLine.args.size(); i++) {
                    r += " " + cLine.args[i];
                }
                r = r.substr(1);
                cmds["-~=" + cLine.args[1]] = r;
            }
        } else if (cLine.command == "-~=delcmd") {
            if (cLine.args.size() > 1) {
                cmds.erase("-~=" + cLine.args[1]);
            }
        } else if ((c = cmds.find(cLine.command)) != cmds.end()) {
            std::string r;
            for (int i = 1; i < cLine.args.size(); i++) {
                r += " " + cLine.args[i];
            }
            if (r.length() > 0)
                r = r.substr(1);

            send(ReplaceString(c->second, "${msg}", r));
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
    cmd("PRIVMSG", new std::vector<std::string>{target, msg});
}

void IRC::Connection::shutdown() {
    shutdown("Bye!");
}

void IRC::Connection::shutdown(std::string s) {
    cmd("QUIT", new std::vector<std::string>{s});
}

bool IRC::Connection::parseMessage(IRC::ParsedLine line, IRC::CommandLine &out) {
    out.prefix = line.prefix;
    out.cmd = line.cmd;
    out.params = line.params;
    out.channel = out.params[0];
    StringUtils::split(out.params.back().substr(1), ' ', out.args);
    out.command = out.args[0];
    // TODO add proper parsing logic, and return true if the line is valid
    return true;
}

void IRC::Connection::handleCommand(IRC::CommandLine line) {
    std::map<std::string, std::string>::iterator c;
    //std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>>::iterator c;
    if ((c = cmds.find(line.command)) != cmds.end()) {
        //msg()
    }
}

void IRC::Connection::log(std::string s) {
    std::ofstream f;
    std::stringstream sstr;
    sstr << "logs/" << this->nick;
    f.open(sstr.str().c_str(), std::ios_base::app);
    f << s << std::endl;
    f.close();
}
