#include "PracticalSocket.h"  // For SocketException
#include "IRC.h"
#include <fstream>
#include <thread>

using namespace std;
using json = nlohmann::json;
int main(int argc, char *argv[]) {
    if (argc != 2)
        exit(1);
    json config;

    string cfg;
    string line;
    ifstream f(argv[1]);
    if (f.is_open()) {
        while (getline(f, line)) {
            cfg += line;
        }
        f.close();
    }
    else {
        cerr << argv[1] << " is not open" << endl;
        exit(1);
    }

    config = json::parse(cfg);
    json server = config["connections"][0];

    try {
        // Establish connection with the server
        IRC::Connection conn(server["connection"]["host"], server["connection"]["port"], server);
        cout << "Connecting" << endl;
        conn.connect();
        cout << "connected " << conn.socket->getLocalAddress() << endl;
        conn.readLoop();

        cout << endl;

        // Destructor closes the socket

    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}