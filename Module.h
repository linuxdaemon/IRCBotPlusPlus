#ifndef IRCBOT_IRCCAP_H
#define IRCBOT_IRCCAP_H

#include <string>
#include <nlohmann/json.hpp>
#include "IRC.h"

using json = nlohmann::json;

namespace IRC {
    struct ParsedLine;

    class Connection;

    class Module {
    public:
        Module(Connection *conn);

        virtual bool OnServerCapAvailable(std::string) = 0;

        virtual void OnServerCapResult(std::string, bool) = 0;

        virtual bool OnRawMessage(ParsedLine line) = 0;

        Connection *conn;
    };

    namespace Modules {
        class SASL : public Module {
        public:
            SASL(IRC::Connection *conn, std::string user, std::string pass, bool require, bool enabled);

            SASL(IRC::Connection *conn, json config);

            bool OnServerCapAvailable(std::string);

            void OnServerCapResult(std::string, bool);

            bool OnRawMessage(IRC::ParsedLine line);

        private:
            void CheckRequireAuth();

            void Authenticate(std::string s);

            std::string user;
            std::string pass;
            bool require;
            bool enabled;
            bool authenticated = false;
        };
    };
};


#endif //IRCBOT_IRCCAP_H
