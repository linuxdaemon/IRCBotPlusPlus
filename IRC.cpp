#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "IRC.h"
#include "Util.h"

void ReplaceStringInPlace(std::string &subject, const std::string &search,
                          const std::string &replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::string ReplaceString(std::string subject, const std::string &search,
                          const std::string &replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

namespace IRC {
    ParsedLine parse(std::string s) {
        ParsedLine p;

        // Prefix
        if (s.front() == ':') {
            IRC::parsePrefix(s.substr(0, s.find(' ')), p.prefix);
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

    bool parsePrefix(std::string s, Prefix &p) {
        size_t i = 0;
        size_t j;
        if ((j = s.find("!")) != std::string::npos) {
            p.nick = s.substr(i, j - 1);
            if ((j = s.find("@")) != std::string::npos) {
                p.user = s.substr(i, j - 1);
                p.host = s.substr(j + 1, s.length());
                p.mask = s;
            }
        } else {
            p.nick = s;
            p.mask = s;
        }
        // TODO better parsing logic
        return true;
    }

    Bot::Bot(json config) {
        this->config = config;
    }

    bool Bot::run() {
        for (json conn : this->config["connections"]) {
            std::cout << conn["name"] << std::endl;
            Connection c(conn["connection"]["host"], conn["connection"]["port"], conn, this);
            this->connections.push_back(c);
            if (conn["sasl"]["enabled"]) {
                Modules::SASL *sasl = new Modules::SASL(&this->connections.back(), conn);
                this->connections.back().registerModule(sasl);
            }
        }
        for (Connection &co : this->connections) {
            co.connect();
            co.readLoop();
        }
        return false;
    }

    Connection::Connection(std::string _host, unsigned short _port, json config, IRC::Bot *bot) {
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
        std::cout << "Registering commands: " << std::endl;
        std::cout << config["commands"] << std::endl;
        auto coms = config["commands"].get<std::unordered_map<std::string, json>>();
        for (auto c : coms) {
            std::cout << c.first << ": " << c.second << std::endl;
            cmds["-~=" + c.first] = c.second;
        }
    }

    void Connection::connect() {
        this->socket = new TCPSocket(host, port);
        this->connected = true;
        log("Connecting to " + socket->getForeignAddress());
        if (this->config["connection"]["cap_negotiation"])
            send("CAP LS");
        send("NICK " + nick);
        cmd("USER", new std::vector<std::string>{user, "3", "*", realname});
    }

    void Connection::send(std::string s) {
        if (connected) {
            //std::cout << s << std::endl;
            IRC::Connection::log(">> " + s);
            socket->send((s + "\r\n").c_str(), s.length() + 2);
        } else {
            std::cout << "Attempted to send line while not connected, ignoring" << std::endl;
        }
    }

    Connection::~Connection() {
        std::cout << "Destructor" << std::endl;
        if (connected) {
            send("QUIT :Shutting Down.");
            delete (this->socket);
        }
    }

    void Connection::readLoop() {
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

    void Connection::dataReceived(const char *buffer) {
        inputBuffer.append(buffer);
        std::string line;
        unsigned long pos;

        while ((pos = inputBuffer.find("\r\n")) != std::string::npos) {
            line = inputBuffer.substr(0, pos);
            inputBuffer = inputBuffer.substr(pos + 2);
            //std::cout << line << std::endl;
            IRC::Connection::log(line);
            ParsedLine pLine = parse(line);
            if (pLine.cmd == "PING") {
                send("PONG " + pLine.params[pLine.params.size() - 1]);
                continue;
            }
            process(pLine);
        }
    }

    void Connection::process(ParsedLine line) {
        if (line.cmd == "004") {
            std::cout << "Connected!!" << std::endl;
            if (_useNickServ)
                msg("NICKSERV", "IDENTIFY " + ns_user + " " + ns_pass);
        } else if (line.cmd == "376") {
            for (std::string chan : config["channels"])
                cmd("JOIN", new std::vector<std::string>{chan});
        } else if (line.cmd == "CAP") {
            if (line.params[1] == "LS") {
                std::vector<std::string> v;
                ::StringUtils::split(line.params[2], ' ', v);
                bool wantedCap;
                for (std::string s : v) {
                    wantedCap = false;
                    for (Module *mod : modules) {
                        if (mod->OnServerCapAvailable(s)) {
                            wantedCap = true;
                        }
                    }
                    if (wantedCap) {
                        capQueue.push_back(s);
                    }
                }
            } else if (line.params[1] == "ACK" || line.params[1] == "NAK") {
                std::vector<std::string> v;
                std::string str = line.params[2];
                if (str.front() == ':') str = str.substr(1);
                ::StringUtils::split(str, ' ', v);
                bool state = line.params[1] == "ACK";
                for (std::string s : v) {
                    for (Module *mod : modules) {
                        mod->OnServerCapResult(s, state);
                    }
                }
            }
            SendNextCap();
        } else if (line.cmd == "PRIVMSG") {
            CommandLine cLine;
            parseMessage(line, cLine);
            std::string chan = cLine.params[0];
            if (chan == this->nick)
                chan = cLine.prefix.nick;

            std::map<std::string, std::string>::iterator c;
            std::string r;
            for (int i = 1; i < cLine.args.size(); i++)
                r += " " + cLine.args[i];

            if (r.length() > 0)
                r = r.substr(1);
            std::map<std::string, std::string> fmt = {
                    {"${msg}",  r},
                    {"${chan}", chan},
                    {"${nick}", cLine.prefix.nick}
            };

            if (cLine.command == "-~=quit") {
                shutdown();
            } else if (cLine.command == "-~=addcmd") {
                if (cLine.args.size() > 2) {
                    std::string s;
                    for (int i = 2; i < cLine.args.size(); i++) {
                        s += " " + cLine.args[i];
                    }
                    s = s.substr(1);
                    cmds["-~=" + cLine.args[1]] = s;
                }
            } else if (cLine.command == "-~=delcmd") {
                if (cLine.args.size() > 1)
                    cmds.erase("-~=" + cLine.args[1]);
            } else if ((c = cmds.find(cLine.command)) != cmds.end()) {
                std::string s = c->second;
                int replaced = 0;
                do {
                    replaced = 0;
                    for (std::pair<std::string, std::string> f : fmt) {
                        std::cout << f.first << ":" << f.second << std::endl;
                        std::cout << s << std::endl;
                        if (s.find(f.first) != std::string::npos) {
                            replaced++;
                            ReplaceStringInPlace(s, f.first, f.second);
                        }
                    }
                } while (replaced > 0);
                send(s);
            }
        }
        for (Module *mod : modules) {
            if (!mod->OnRawMessage(line))
                break;
        }
    }

    void Connection::setNick(std::string _nick) {
        nick = _nick;
    }

    void Connection::setUser(std::string _user) {
        user = _user;
    }

    void Connection::setRealname(std::string _realname) {
        realname = _realname;
    }

    void Connection::useNickServ(bool b) {
        _useNickServ = b;
    }

    void Connection::setNickServUser(std::string ns_user) {
        this->ns_user = ns_user;
    }

    void Connection::setNickServPass(std::string pass) {
        this->ns_pass = pass;
    }

    void Connection::cmd(std::string cmd, std::vector<std::string> *args) {
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

    void Connection::msg(std::string target, std::string msg) {
        cmd("PRIVMSG", new std::vector<std::string>{target, msg});
    }

    void Connection::shutdown() {
        shutdown("Bye!");
    }

    void Connection::shutdown(std::string s) {
        cmd("QUIT", new std::vector<std::string>{s});
        for (Module *m : modules) {
            delete (m);
        }
    }

    bool parseMessage(IRC::ParsedLine line, IRC::CommandLine &out) {
        out.prefix = line.prefix;
        out.cmd = line.cmd;
        out.params = line.params;
        out.channel = out.params[0];
        StringUtils::split(out.params.back().substr(1), ' ', out.args);
        out.command = out.args[0];
        // TODO add proper parsing logic, and return true if the line is valid
        return true;
    }

    void Connection::handleCommand(IRC::CommandLine line) {
        std::map<std::string, std::string>::iterator c;
        if ((c = cmds.find(line.command)) != cmds.end()) {
            //msg()
        }
    }

    void Connection::log(std::string s) {
        std::ofstream f;
        std::stringstream sstr;
        sstr << "logs/" << this->nick << ".log";
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%d-%m-%Y %I:%M:%S", timeinfo);
        std::string str(buffer);
        f.open(sstr.str().c_str(), std::ios_base::app);
        std::cout << "[" << str << "] " << s << std::endl;
        f << "[" << str << "] " << s << std::endl;
        f.close();
    }

    void Connection::registerModule(IRC::Module *mod) {
        this->modules.push_back(mod);
    }

    void Connection::SendNextCap() {
        if (!capPaused) {
            if (capQueue.empty()) {
                send("CAP END");
            } else {
                std::string sCap = *capQueue.begin();
                capQueue.erase(capQueue.begin());
                cmd("CAP", new std::vector<std::string>{"REQ", sCap});
            }
        }
    }

    void Connection::PauseCap() {
        capPaused++;
    }

    void Connection::ResumeCap() {
        capPaused--;
        SendNextCap();
    }
};
