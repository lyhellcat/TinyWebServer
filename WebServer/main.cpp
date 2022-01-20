#include "webserver.h"

int main() {
    WebServer server(
        1388, 3, 60000, false,
        3306, "root", "root", "Webserver",
        12, 6, true, 1,
        1024);

    server.Start();
}
