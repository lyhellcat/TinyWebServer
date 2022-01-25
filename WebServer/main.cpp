#include <signal.h>

#include "webserver.h"

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
        1388, 3, 600000, false,
        3306, "root", "root", "Webserver",
        12, 6, false, 1,
        1024);
    struct sigaction action; action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    server->Start();
}
