#include "Module.h"
#include "Util.h"

IRC::Module::Module(IRC::Connection *conn) {
    this->conn = conn;
}

// TODO add support for other sasl mechanisms
IRC::Modules::SASL::SASL(IRC::Connection *conn, std::string user, std::string pass,
                         bool require, bool enabled) : Module(conn) {
    this->user = user;
    this->pass = pass;
    this->require = require;
    this->enabled = enabled;
}

bool IRC::Modules::SASL::OnServerCapAvailable(std::string s) {
    return s == "sasl";
}

void IRC::Modules::SASL::OnServerCapResult(std::string s, bool b) {
    conn->log("[sasl] " + s);
    if (s == "sasl") {
        if (b) {
            conn->PauseCap();
            conn->send("AUTHENTICATE PLAIN");
        } else {
            ::IRC::Modules::SASL::CheckRequireAuth();
        }
    }
}

bool IRC::Modules::SASL::OnRawMessage(IRC::ParsedLine line) {
    if (line.cmd == "AUTHENTICATE") {
        Authenticate(line.params[0]);
    } else if (line.cmd == "903") {
        conn->ResumeCap();
        authenticated = true;
    } else if (line.cmd == "904" || line.cmd == "905") {
        CheckRequireAuth();
        conn->ResumeCap();
    } else if (line.cmd == "906") {
        CheckRequireAuth();
    } else if (line.cmd == "907") {
        authenticated = true;
        conn->ResumeCap();
    } else {
        return true;
    }
    return false;
}

void IRC::Modules::SASL::CheckRequireAuth() {
    if (require) {
        conn->log("[ERROR] SASL auth required, but authentication failed");
        conn->shutdown();
    }
}

void IRC::Modules::SASL::Authenticate(std::string s) {
    if (s == "+") {
        conn->log("[sasl] Attempting sasl auth with user '" + user + "' and password '" + pass + "'");
        std::string authLine = user + '\0' + user + '\0' + pass;
        ::StringUtils::Base64Encode(authLine);
        conn->send("AUTHENTICATE " + authLine);
    }
}

IRC::Modules::SASL::SASL(IRC::Connection *conn, json config) : Module(conn) {
    this->user = config["sasl"]["user"];
    std::cout << this->user << std::endl;
    this->pass = config["sasl"]["pass"];
    std::cout << this->pass << std::endl;
    std::cout << config["sasl"] << std::endl;

    this->require = config["sasl"]["required"];
    this->enabled = config["sasl"]["enabled"];
}
