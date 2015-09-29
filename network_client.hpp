#ifndef __NETWORK_CLIENT_HPP__
#define __NETWORK_CLIENT_HPP__

#include <cstdint>
#include <array>
#include <queue>
#include <string>



class NetworkClient
{
    public:
        NetworkClient(std::string, uint16_t, uint32_t);

        void run();
    private:
        void disconnect();

        void handle_answer_authorize(std::vector<uint8_t>);
        void handle_answer_upload(std::vector<uint8_t>);
        void handle_answer_download(std::vector<uint8_t>);
        void handle_answer_get_filelist(std::vector<uint8_t>);

        void handle_data_from_pipe();
        void handle_data_from_server();
        void handle_message_from_shell(const std::vector<uint8_t>&);
        void initialize_connection();
        std::vector<uint8_t> read_from_socket(uint32_t);
        void send_cmd_auth(const std::vector<uint8_t>&);
        void send_cmd_upload(const std::vector<uint8_t>&);
        void send_cmd_download(const std::vector<uint8_t>&);
        void send_cmd_ls();
        void write_to_shell(std::string);

        uint32_t sock_;
        std::string server_address_;
        uint16_t server_port_;
        uint32_t pipe_;
        uint32_t parent_pid_;
        bool connected_to_remote_ = false;
        std::queue<std::string> message_queue_;
        std::array<uint8_t, 20480> buffer_; // io buffer
        std::vector<uint8_t> message_buffer_;
};


#endif // __NETWORK_CLIENT_HPP__
