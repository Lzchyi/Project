#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "network_client.hpp"
#include "shell.hpp"



std::shared_ptr<NetworkClient> client;
std::shared_ptr<Shell> shell;



void signal_handler(int32_t signum)
{
    if (signum == SIGCHLD) {
        std::cout << "Client process terminated. Exiting..." << std::endl;
        exit(-1);
    }
}

//----------------------------------------------------------------------

int main(int32_t argc, char **argv)
{
    std::string server_address = "127.0.0.1";
    uint16_t server_port = 8000;

    if (argc >= 3) {
        server_address = std::string(argv[1]);
        server_port = atoi(argv[2]);
    }

    signal(SIGCHLD, signal_handler);


    int32_t pipefd[2];
    if (pipe(pipefd) != 0) {
        throw std::runtime_error("unable to create pipe");
    }


    int32_t child_pid = fork();
    if (child_pid == 0) {
        client = std::make_shared<NetworkClient>(server_address, server_port, pipefd[0]);
        client->run();
    }
    else {
        shell = std::make_shared<Shell>(pipefd[1], child_pid);
        shell->run();
    }


    return 0;
}
