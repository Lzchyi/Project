#include "shell.hpp"
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "client_messaging.hpp"
#include "util.hpp"



Shell::Shell(uint32_t pipe, uint32_t child_pid): child_pid_(child_pid)
{
    fifo_ = pipe;
}

//----------------------------------------------------------------------

void Shell::exit_nicely()
{
    kill(child_pid_, SIGINT);
    exit(0);
}

//----------------------------------------------------------------------

void Shell::handle_cmd_help()
{
    std::stringstream out;
    out << std::endl;
    out << "help     - print this help" << std::endl;
    out << "auth     - authorize on remote server" << std::endl;
    out << "upload   - upload file to remote server" << std::endl;
    out << "download - fetch file from remote server" << std::endl;
    out << "ls       - list files on remote server" << std::endl;
    out << "quit     - quit from program" << std::endl;
    out << std::endl;

    std::cout << out.str() << std::endl;
}

//----------------------------------------------------------------------

void Shell::handle_cmd_auth()
{
    std::cout << std::endl;

    std::cout << "please enter username: ";
    std::string username;
    std::getline(std::cin, username);


    std::cout << "please enter password: ";
    std::string password;
    std::getline(std::cin, password);

    send_auth_request(username, password);
}

//----------------------------------------------------------------------

void Shell::handle_cmd_upload()
{
    std::cout << std::endl;

    std::cout << "please enter file name to upload: ";
    std::string filename;
    std::getline(std::cin, filename);

    send_upload_request(filename);
}

//----------------------------------------------------------------------

void Shell::handle_cmd_download()
{
    std::cout << std::endl;

    std::cout << "please ender file name to download: ";
    std::string filename;
    std::getline(std::cin, filename);


    send_download_request(filename);
}

//----------------------------------------------------------------------

void Shell::handle_cmd_ls()
{
    struct m_cmd_ls cmd;

    auto data = data_from_struct(cmd, sizeof(cmd));

    write_to_fifo(data);
}

//----------------------------------------------------------------------

void Shell::handle_cmd_quit()
{
    exit_nicely();
}

//----------------------------------------------------------------------

void Shell::process_cmd(std::string cmd)
{
    if (cmd == "help") {
        handle_cmd_help();
    }
    else if (cmd == "auth") {
        handle_cmd_auth();
    }
    else if (cmd == "upload") {
        handle_cmd_upload();
    }
    else if (cmd == "download") {
        handle_cmd_download();
    }
    else if (cmd == "ls") {
        handle_cmd_ls();
    }
    else if (cmd == "quit") {
        handle_cmd_quit();
    }
}

//----------------------------------------------------------------------

std::string Shell::read_from_socket(uint32_t sock)
{
    int32_t rv = read(sock, buffer_.data(), buffer_.size());
    if (rv > 0) {
        return std::string(buffer_.begin(), buffer_.begin() + rv);
    }

    return "";
}

//----------------------------------------------------------------------

void Shell::received_stdin_data()
{
    auto data = read_from_socket(STDIN_FILENO); // 0 is always stdin socket

    std::string cmd = data.substr(0, data.size() - 1); // remove last \n char

    process_cmd(cmd);
}

//----------------------------------------------------------------------

void Shell::received_fifo_data()
{
    std::cout << __func__  << std::endl;

    auto data = read_from_socket(fifo_);

    std::cout << std::endl;
    std::cout << data << std::endl;
    std::cout.flush();
}

//----------------------------------------------------------------------

void Shell::run()
{
    std::string cmd;


    fd_set rfds;


    while (true) {
        std::cout << ">>> ";
        std::cout.flush();

        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fifo_, &rfds);

        int32_t rv = select(fifo_ + 1, &rfds, nullptr, nullptr, nullptr);
        if (rv > 0) {
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                received_stdin_data();
            }
            else if (FD_ISSET(fifo_, &rfds)) {
                received_fifo_data();
            }
        }
    }
}

//----------------------------------------------------------------------

void Shell::send_auth_request(std::string username, std::string password)
{
    struct m_cmd_auth cmd;
    memset(&cmd.username_, 0, cmd.username_.size());
    memset(&cmd.password_, 0, cmd.password_.size());

    if ((username.length() < cmd.username_.size()) && (password.length() < cmd.password_.size())) {
        memcpy(cmd.username_.data(), username.data(), username.length());
        memcpy(cmd.password_.data(), password.data(), password.length());

        auto data = data_from_struct(cmd, sizeof(cmd));
        write_to_fifo(data);
    }
}

//----------------------------------------------------------------------

void Shell::send_upload_request(std::string filename)
{
    struct m_cmd_upload cmd;

    if (filename.length() < cmd.filename_.size()) {
        memcpy(cmd.filename_.data(), filename.data(), filename.length());

        auto data = data_from_struct(cmd, sizeof(cmd));
        write_to_fifo(data);
    }
}

//----------------------------------------------------------------------

void Shell::send_download_request(std::string filename)
{
    struct m_cmd_download cmd;

    if (filename.length() < cmd.filename_.size()) {
        memcpy(cmd.filename_.data(), filename.data(), filename.length());

        auto data = data_from_struct(cmd, sizeof(cmd));
        write_to_fifo(data);
    }
}

//----------------------------------------------------------------------

void Shell::write_to_fifo(const std::vector<uint8_t>& data)
{
    write(fifo_, data.data(), data.size());
}
