#include "network_server.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <array>
#include "util.hpp"



NetworkServer::NetworkServer(std::string address, uint16_t port, std::string working_directory):
    address_(address), port_(port)
{
    auto handler = [&](const std::shared_ptr<ClientConnection>& conn,
        const std::vector<uint8_t>& data) {this->write_to_client(conn, data);};

    fs_manager_ = std::make_shared<FileSyncManager>(working_directory);
    fs_manager_->set_write_handler(handler);
}

//----------------------------------------------------------------------

std::shared_ptr<ClientConnection> NetworkServer::get_client_connection(uint32_t sock)
{
    if (connections_.count(sock)) {
        return connections_.at(sock);
    }

    return nullptr;
}

//----------------------------------------------------------------------

void NetworkServer::handle_client_connected()
{
    struct sockaddr_in client_addr;
    uint32_t socklen = sizeof(struct sockaddr_in);

    int32_t sock = accept(server_sock_, (struct sockaddr *)&client_addr, &socklen);
    if (sock > 0) {
        set_socket_nonblocking_mode(sock);

        uint32_t ip = ntohl(client_addr.sin_addr.s_addr);
        uint16_t port = ntohs(client_addr.sin_port);

        if (io_loop_add_socket(sock)) {
            auto conn = std::make_shared<ClientConnection>(sock, ip, port);
            connections_[sock] = conn;

            std::cout << "client connected " << conn->ip_str() << ":" << conn->port() << std::endl;
            fs_manager_->handle_client_connected(conn);
        }
        else {
            close(sock);
        }
    }
}

//----------------------------------------------------------------------

void NetworkServer::handle_client_disconnected(uint32_t sock)
{

    auto conn = get_client_connection(sock);
    if (conn) {
        std::cout << "client disconnected " << conn->ip_str() << ":" << conn->port() << std::endl;
        fs_manager_->handle_client_disconnected(conn);
    }


    connections_.erase(sock);

    io_loop_del_socket(sock);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

//----------------------------------------------------------------------

void NetworkServer::handle_data_received(uint32_t sock)
{
    static std::array<uint8_t, 20480> buffer;

    int32_t rv = read(sock, buffer.data(), buffer.size());
    if (rv > 0) {
        auto conn = get_client_connection(sock);
        if (conn) {
            std::vector<uint8_t> data(rv);
            std::copy(buffer.begin(), buffer.begin() + rv, data.begin());

            fs_manager_->handle_data_received(conn, data);
        }
    }
    else {
        handle_client_disconnected(sock);
    }
}

//----------------------------------------------------------------------

void NetworkServer::initialize_epoll()
{
    epfd_ = epoll_create(epoll_event_size_);
    if (epfd_ == -1) {
        throw std::runtime_error("Unable to create epoll socket");
    }
    ev_.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
}

//----------------------------------------------------------------------

void NetworkServer::initialize_socket()
{
    struct sockaddr_in server_addr;
    uint32_t socklen = sizeof(struct sockaddr_in);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = inet_addr(address_.c_str());

    int32_t server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        throw std::runtime_error("unable to create TCP socket");
    }

    set_socket_nonblocking_mode(server_sock);

    int32_t enable = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    if (bind(server_sock, (const struct sockaddr *)&server_addr, socklen) < 0) {
        throw std::runtime_error("unable to bind TCP port for listening");
    }

    listen(server_sock, SOMAXCONN);

    server_sock_ = server_sock;
}

//----------------------------------------------------------------------

bool NetworkServer::io_loop_add_socket(uint32_t sock)
{
    ev_.data.fd = sock;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, sock, &ev_) == 0) {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------

bool NetworkServer::io_loop_del_socket(uint32_t sock)
{
    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, sock, &ev_) == 0) {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------

void NetworkServer::run()
{
    initialize_socket();
    initialize_epoll();


    int32_t event_cnt = 0;
    struct epoll_event events[epoll_event_size_];

    if (!io_loop_add_socket(server_sock_)) {
        throw std::runtime_error("Unable to add socket to epoll");
    }


    while (true) {
        try {
            event_cnt = epoll_wait(epfd_, events, epoll_event_size_, -1); // -1 mean wait forever

            for (int32_t n = 0; n < event_cnt; ++n) {
                uint32_t sock = events[n].data.fd;
                uint32_t ev = events[n].events;

                if ((ev & EPOLLIN) || (ev & EPOLLPRI)) {
                    if (sock == server_sock_) {
                        handle_client_connected();
                    }
                    else {
                        handle_data_received(sock);
                    }
                }
                else if ((ev & EPOLLERR) || (ev & EPOLLHUP) || (ev & EPOLLRDHUP)) {
                    handle_client_disconnected(sock);
                }
            }
        }
        catch (...) {
        }
    }
}

//----------------------------------------------------------------------

void NetworkServer::write_to_client(const std::shared_ptr<ClientConnection>& conn,
    const std::vector<uint8_t>& data)
{
    int32_t rv = send(conn->sock(), data.data(), data.size(), MSG_NOSIGNAL);
    if (rv != static_cast<int32_t>(data.size())) {
        handle_client_disconnected(conn->sock());
    }
}
