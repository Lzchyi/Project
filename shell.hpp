#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <cstdint>
#include <array>
#include <string>
#include <vector>



class Shell
{
    public:
        //Shell(std::string, uint32_t);
        Shell(uint32_t, uint32_t);

        void run();
    private:
        void exit_nicely();
        void handle_cmd_help();
        void handle_cmd_auth();
        void handle_cmd_upload();
        void handle_cmd_download();
        void handle_cmd_ls();
        void handle_cmd_quit();
        void process_cmd(std::string);
        std::string read_from_socket(uint32_t);
        void received_stdin_data();
        void received_fifo_data();
        void send_auth_request(std::string, std::string);
        void send_upload_request(std::string);
        void send_download_request(std::string);
        void write_to_fifo(const std::vector<uint8_t>&);

        uint32_t fifo_;
        uint32_t child_pid_;
        std::array<uint8_t, 10240> buffer_;
};


#endif // __SHELL_HPP__
