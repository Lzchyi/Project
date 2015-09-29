#ifndef __FILESYNC_MANAGER_HPP__
#define __FILESYNC_MANAGER_HPP__

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "client_connection.hpp"



extern std::map<std::string, std::string> user_database;



class FileSyncManager
{
    public:
        FileSyncManager(std::string);

        void handle_client_connected(const std::shared_ptr<ClientConnection>&);
        void handle_client_disconnected(const std::shared_ptr<ClientConnection>&);
        void handle_data_received(const std::shared_ptr<ClientConnection>&, const std::vector<uint8_t>&);
        void handle_msg_authorize(const std::shared_ptr<ClientConnection>&, std::vector<uint8_t>&);
        void handle_msg_upload(const std::shared_ptr<ClientConnection>&, std::vector<uint8_t>&);
        void handle_msg_download(const std::shared_ptr<ClientConnection>&, std::vector<uint8_t>&);
        void handle_msg_get_filelist(const std::shared_ptr<ClientConnection>&, std::vector<uint8_t>&);

        template<typename Handler>
        void set_write_handler(Handler&&);
    private:
        void check_client_auth(const std::shared_ptr<ClientConnection>& conn, std::string user, std::string passwd);

        std::string get_dir_listing(std::string);
        bool save_file(const std::shared_ptr<ClientConnection>&, std::string, const std::vector<uint8_t>&);
        void verify_working_directory();

        std::function<void(const std::shared_ptr<ClientConnection>&, const std::vector<uint8_t>&)> write_handler_;
        std::map<std::shared_ptr<ClientConnection>, std::vector<uint8_t>> connections_;
        std::string working_directory_;
};

//----------------------------------------------------------------------

template<typename Handler>
void FileSyncManager::set_write_handler(Handler&& handler)
{
    write_handler_ = std::forward<Handler>(handler);
}


#endif // __FILESYNC_MANAGER_HPP__
