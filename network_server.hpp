#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <sys/epoll.h>
#include <iostream>
#include <functional>
#include <map>
#include <memory>
#include "client_connection.hpp"
#include "filesync_manager.hpp"



class NetworkServer
{
    public:
        NetworkServer(std::string, uint16_t, std::string);

        void run();
    private:
        std::shared_ptr<ClientConnection> get_client_connection(uint32_t);
        void handle_client_connected();
        void handle_client_disconnected(uint32_t);
        void handle_data_received(uint32_t);
        void initialize_epoll();
        void initialize_socket();
        bool io_loop_add_socket(uint32_t);
        bool io_loop_del_socket(uint32_t);
        void write_to_client(const std::shared_ptr<ClientConnection>&, const std::vector<uint8_t>&);

        int32_t epfd_ = 0;
        struct epoll_event ev_;
        static constexpr uint32_t epoll_event_size_ = 16;
        std::map<uint32_t, std::shared_ptr<ClientConnection>> connections_;
        uint32_t server_sock_;
        std::string address_;
        uint16_t port_;
        std::shared_ptr<FileSyncManager> fs_manager_;
};


#endif // __SERVER_HPP__
