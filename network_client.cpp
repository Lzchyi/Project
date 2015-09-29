#include "network_client.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "client_messaging.hpp"
#include "network_messaging.hpp"
#include "util.hpp"



NetworkClient::NetworkClient(std::string server_address, uint16_t server_port, uint32_t pipe):
    server_address_(server_address), server_port_(server_port)
{
    parent_pid_ = getppid();
    pipe_ = pipe;

    std::stringstream out;
    out << std::endl;
    out << "[#] client started" << std::endl;
    out << "[#] trying connect to remote server " << server_address_ << ":" << server_port_ << std::endl;
    out << std::endl;

    std::cout << out.str() << std::endl;
    std::cout.flush();
}

//----------------------------------------------------------------------

void NetworkClient::disconnect()
{
    write_to_shell("disconnected from remote server");

    connected_to_remote_ = false;
    close(sock_);
    sock_ = 0;
}

//----------------------------------------------------------------------

void NetworkClient::handle_answer_authorize(std::vector<uint8_t> data)
{
    struct nm_authorize answer;

    if (fillstruct(&answer, sizeof(answer), data)) {
        if (answer.authorized_) {
            write_to_shell("access on file server granted");
        }
        else {
            write_to_shell("incorrect username or password");
        }

        message_buffer_.resize(0);
        message_buffer_.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void NetworkClient::handle_answer_upload(std::vector<uint8_t> data)
{
    struct nm_upload answer;

    if (fillstruct(&answer, sizeof(answer), data)) {
        if (answer.file_saved_) {
            write_to_shell("file successfully saved");
        }
        else if (answer.access_error_) {
            write_to_shell("access error, please authorize first");
        }

        message_buffer_.resize(0);
        message_buffer_.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void NetworkClient::handle_answer_download(std::vector<uint8_t> data)
{
    struct nm_download answer;

    if (fillstruct(&answer, sizeof(answer), data)) {
        if (answer.file_found_) {
            auto filename = std::string(answer.filename_.begin(), answer.filename_.end());
            filename = filename.substr(0, strlen(filename.c_str()));



            int32_t fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd > 0) {
                write(fd, answer.data_.data(), answer.data_size_);
                close(fd);

                write_to_shell("file saved");
            }
            else {
                write_to_shell("file saving error");
            }
        }
        else if (answer.file_to_large_) {
            write_to_shell("requested to download file is too large");
        }
        else if (answer.file_not_found_) {
            write_to_shell("requested to download file not found");
        }
        else if (answer.access_error_) {
            write_to_shell("access error, please authorize first");
        }

        message_buffer_.resize(0);
        message_buffer_.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void NetworkClient::handle_answer_get_filelist(std::vector<uint8_t> data)
{
    struct nm_get_filelist answer;

    if (fillstruct(&answer, sizeof(answer), data)) {
        if (answer.access_error_) {
            write_to_shell("access error, please authrorize first.");
        }
        else {
            auto listing = std::string(answer.list_.begin(), answer.list_.end());
            listing = listing.substr(0, strlen(listing.c_str()));

            write_to_shell(listing);
        }

        message_buffer_.resize(0);
        message_buffer_.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void NetworkClient::handle_data_from_pipe()
{
    auto data = read_from_socket(pipe_);

    handle_message_from_shell(data);
}

//----------------------------------------------------------------------

void NetworkClient::handle_data_from_server()
{
    auto data = read_from_socket(sock_);
    if (!data.size()) {
        disconnect();
        return;
    }

    std::copy(data.begin(), data.end(), std::back_inserter(message_buffer_));


    auto type = static_cast<NetworkMessageType>(message_buffer_.at(0));

    switch (type) {
        case NetworkMessageType::AUTHORIZE:
            handle_answer_authorize(message_buffer_);
            break;
        case NetworkMessageType::UPLOAD:
            handle_answer_upload(message_buffer_);
            break;
        case NetworkMessageType::DOWNLOAD:
            handle_answer_download(message_buffer_);
            break;
        case NetworkMessageType::GET_FILELIST:
            handle_answer_get_filelist(message_buffer_);
            break;
        default:
            if (message_buffer_.size() > 20480) {
                message_buffer_.resize(0);
                message_buffer_.shrink_to_fit();
            }
            break;
    }
}

//----------------------------------------------------------------------

void NetworkClient::handle_message_from_shell(const std::vector<uint8_t>& data)
{
    auto type = static_cast<ClientMessageType>(data.at(0));

    switch (type) {
        case ClientMessageType::CMD_AUTH:
            send_cmd_auth(data);
            break;
        case ClientMessageType::CMD_UPLOAD:
            send_cmd_upload(data);
            break;
        case ClientMessageType::CMD_DOWNLOAD:
            send_cmd_download(data);
            break;
        case ClientMessageType::CMD_LS:
            send_cmd_ls();
            break;
        default:
            std::cout << __func__ << " unknown message received" << std::endl;
            break;
    }
}

//----------------------------------------------------------------------

void NetworkClient::initialize_connection()
{
    struct sockaddr_in remote_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(server_port_);
    remote_addr.sin_addr.s_addr = inet_addr(server_address_.c_str());

    int32_t sock;

    while (true) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { 
            continue;
        }

        if (connect(sock, (const struct sockaddr *)&remote_addr, socklen) < 0) {
            close(sock);
            sleep(1); // sleep for one second to avoid flooding
            continue;
        }

        write_to_shell("connection to remote server established");

        connected_to_remote_ = true;
        break;
    }

    sock_ = sock;
}

//----------------------------------------------------------------------

std::vector<uint8_t> NetworkClient::read_from_socket(uint32_t sock)
{
    std::vector<uint8_t> data;

    int32_t rv = read(sock, buffer_.data(), buffer_.size());
    if (rv > 0) {
        data.resize(rv);
        std::copy(buffer_.begin(), buffer_.begin() + rv, data.begin());
    }

    return data;
}

//----------------------------------------------------------------------

void NetworkClient::run()
{
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 1;

    while (true) {
        if (!connected_to_remote_) {
            initialize_connection();
        }


        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        FD_SET(pipe_, &rfds);

        auto maxfds = std::max(sock_, pipe_);

        int32_t rv = select(maxfds + 1, &rfds, nullptr, nullptr, &tv);
        if (rv > 0) {
            if (FD_ISSET(sock_, &rfds)) {
                handle_data_from_server();
            }
            else if (FD_ISSET(pipe_, &rfds)) {
                handle_data_from_pipe();
            }
        }
        else if ((rv < 0) && (errno != EINTR)) {
            disconnect();
        }
    }
}

//----------------------------------------------------------------------

void NetworkClient::send_cmd_auth(const std::vector<uint8_t>& data)
{
    struct m_cmd_auth cmd;

    if (fillstruct(&cmd, sizeof(cmd), data)) {
        struct nm_authorize request;
        memset(&request, 0, sizeof(request));

        auto user = std::string(cmd.username_.begin(), cmd.username_.end());
        auto passwd = std::string(cmd.password_.begin(), cmd.password_.end());

        auto user_len = strlen(user.c_str());
        auto passwd_len = strlen(passwd.c_str());

        memcpy(request.username_.data(), cmd.username_.data(), user_len);
        memcpy(request.password_.data(), cmd.password_.data(), passwd_len);

        int32_t rv = send(sock_, &request, sizeof(request), MSG_NOSIGNAL);
        if (rv != sizeof(request)) {
            connected_to_remote_ = false;
        }
    }
}

//----------------------------------------------------------------------

void NetworkClient::send_cmd_upload(const std::vector<uint8_t>& data)
{
    struct m_cmd_upload cmd;

    if (fillstruct(&cmd, sizeof(cmd), data)) {
        auto filename = std::string(cmd.filename_.begin(), cmd.filename_.end());
        filename = filename.substr(0, strlen(filename.c_str()));

        if (!is_file(filename)) {
            write_to_shell("unable open file " + filename);
            return;
        }

        struct nm_upload request;

        auto fsize = filesize(filename);
        if ((fsize > 0) && (fsize < static_cast<int32_t>(request.data_.size()))) {
            std::vector<uint8_t> filedata;
            if (!readfile(filename, &filedata)) {
                write_to_shell("unable to read the file: " + filename + ", please use absolute path");
                return;
            }

            memcpy(request.filename_.data(), filename.data(), filename.length());
            memcpy(request.data_.data(), filedata.data(), filedata.size());
            request.data_size_ = filedata.size();
        }
        else {
            write_to_shell("file is too large to sent it: " + filename);
            return;
        }

        int32_t rv = send(sock_, &request, sizeof(request), MSG_NOSIGNAL);
        if (rv != sizeof(request)) {
            connected_to_remote_ = false;
        }
    }
}

//----------------------------------------------------------------------

void NetworkClient::send_cmd_download(const std::vector<uint8_t>& data)
{
    struct m_cmd_download cmd;

    if (fillstruct(&cmd, sizeof(cmd), data)) {
        auto filename = std::string(cmd.filename_.begin(), cmd.filename_.end());
        filename = filename.substr(0, strlen(filename.c_str()));

        struct nm_download request;
        std::copy(cmd.filename_.begin(), cmd.filename_.end(), request.filename_.begin());


        int32_t rv = send(sock_, &request, sizeof(request), MSG_NOSIGNAL);
        if (rv != sizeof(request)) {
            connected_to_remote_ = false;
        }
    }
}

//----------------------------------------------------------------------

void NetworkClient::send_cmd_ls()
{
    struct nm_get_filelist request;

    int32_t rv = send(sock_, &request, sizeof(request), MSG_NOSIGNAL);
    if (rv != sizeof(request)) {
        connected_to_remote_ = false;
    }
}

//----------------------------------------------------------------------

void NetworkClient::write_to_shell(std::string msg)
{
    std::cout << std::endl;
    std::cout << "[info] " << msg << std::endl;
    std::cout.flush();
}
