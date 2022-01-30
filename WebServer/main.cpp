#include "webserver.h"
#include "json.hpp"

#include <fstream>
#include <signal.h>

using namespace std;
using namespace nlohmann;

WebServer *server;

void signal_handler(int signo) {
    if (signo == SIGINT) {
        cout << "\nSTOP server" << endl;
        server->Stop();
        delete server;
        exit(EXIT_SUCCESS);
    }
}

int main() {
    ifstream json_file("../config.json");
    json j;
    json_file >> j;

    server = new WebServer(
        static_cast<int>(j["Port"]),
        static_cast<int>(j["Trigger mode"]),
        static_cast<int>(j["Timeout MS"]),
        static_cast<bool>(j["Is open linger"]),
        static_cast<int>(j["Sql"]["port"]),
        static_cast<string>(j["Sql"]["user"]).c_str(),
        static_cast<string>(j["Sql"]["password"]).c_str(),
        static_cast<string>(j["Sql"]["database name"]).c_str(),
        static_cast<int>(j["Sql"]["connection pool num"]),
        static_cast<int>(j["Thread num"]),
        static_cast<bool>(j["Is open log"]),
        static_cast<int>(j["Log level"]),
        static_cast<int>(j["Log queue size"]));

    struct sigaction action;
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    server->Run();
}
