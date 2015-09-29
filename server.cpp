#include "network_server.hpp"



const std::string working_directory = "/tmp/fileserver";


std::map<std::string, std::string> user_database;



int main(int32_t argc, char **argv)
{
    uint16_t server_port = 8000;
    if (argc > 1) {
        server_port = atoi(argv[1]);
    }

    user_database["root"] = "passwd123";
    user_database["admin"] = "somepasswd";

    auto server = std::make_shared<NetworkServer>("0.0.0.0", server_port, working_directory);
    server->run();


    return 0;
}
