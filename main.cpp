#include "includes/server.hpp"
#include <iostream>

int main() {
    try {
        ServerConfig conf;
        conf.port = 8080;
        conf.server_name = "localhost";
        conf.root = "./www";
        conf.index = "index.html";

        Server server(conf);
        server.init();
        server.run();

    } catch(const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
