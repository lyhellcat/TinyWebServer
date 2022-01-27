#include "webserver.h"

#include <signal.h>

using namespace std;

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
    server = new WebServer(
        8077, 3, 0, false,
        3306, "root", "root", "Webserver",
        12, 64, false, 1,
        1024);
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    server->Start();
}
