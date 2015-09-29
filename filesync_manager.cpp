#include "filesync_manager.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <deque>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "network_messaging.hpp"
#include "util.hpp"



FileSyncManager::FileSyncManager(std::string working_directory):
    working_directory_(working_directory)
{
    verify_working_directory();
}

//----------------------------------------------------------------------

void FileSyncManager::check_client_auth(const std::shared_ptr<ClientConnection>& conn,
    std::string user, std::string passwd)
{
    std::cout << __func__ << " user -> " << user << "\t passwd -> " << passwd << std::endl;


    if (user_database.count(user)) {
        if (user_database.at(user) == passwd) {
            conn->authorize(user, passwd);
            create_directory(user);

            std::stringstream out;
            out << "access garanted to user " << user << " on client connection " << conn->ip_str() << ":" << conn->port() << std::endl;
            std::cout << out.str() << std::endl;
            std::cout.flush();
        }
    }
}

//----------------------------------------------------------------------

std::string FileSyncManager::get_dir_listing(std::string dirname)
{
    std::stringstream out;

    auto d = opendir(dirname.c_str());
    struct dirent* dir;


    std::deque<std::string> dirlist;
    std::deque<std::string> filelist;


    if (d) {
        while ((dir = readdir(d)) != nullptr) {
            if ((std::string(dir->d_name) == ".") || (std::string(dir->d_name) == "..")) {
                continue;
            }

            if (dir->d_type == DT_DIR) {
                dirlist.push_back(std::string(dir->d_name));
            }

            if (dir->d_type == DT_REG) {
                filelist.push_back(std::string(dir->d_name));
            }
        }
    }

    out << std::endl;
    out << "#########################" << std::endl;
    out << "#   directory listing   #" << std::endl;
    out << "#########################" << std::endl;
    out << std::endl;

    uint32_t cnt = 0;

    for (auto name: dirlist) {
        out << cnt++ << ". [d] " << name << std::endl;
    }

    for (auto name: filelist) {
        out << cnt++ << ". [f] " << name << std::endl;
    }

    out << std::endl;
    out << "total dirs: " << dirlist.size() << std::endl;
    out << "total files: " << filelist.size() << std::endl;

    return out.str();
}

//----------------------------------------------------------------------

void FileSyncManager::handle_client_connected(const std::shared_ptr<ClientConnection>& conn)
{
    connections_[conn] = std::vector<uint8_t>();
}

//----------------------------------------------------------------------

void FileSyncManager::handle_client_disconnected(const std::shared_ptr<ClientConnection>& conn)
{
    connections_.erase(conn);
}

//----------------------------------------------------------------------

void FileSyncManager::handle_data_received(const std::shared_ptr<ClientConnection>& conn,
    const std::vector<uint8_t>& data)
{
    auto& buffer = connections_[conn];

    std::copy(data.begin(), data.end(), std::back_inserter(buffer));


    auto type = static_cast<NetworkMessageType>(buffer.at(0));

    switch (type) {
        case NetworkMessageType::AUTHORIZE:
            handle_msg_authorize(conn, buffer);
            break;
        case NetworkMessageType::UPLOAD:
            handle_msg_upload(conn, buffer);
            break;
        case NetworkMessageType::DOWNLOAD:
            handle_msg_download(conn, buffer);
            break;
        case NetworkMessageType::GET_FILELIST:
            handle_msg_get_filelist(conn, buffer);
            break;
        default:
            if (buffer.size() > 20480) {
                buffer.resize(0);
                buffer.shrink_to_fit();
            }
            break;
    }
}

//----------------------------------------------------------------------

void FileSyncManager::handle_msg_authorize(const std::shared_ptr<ClientConnection>& conn,
    std::vector<uint8_t>& data)
{
    struct nm_authorize request;

    if (fillstruct(&request, sizeof(request), data)) {
        auto user = std::string(request.username_.begin(), request.username_.end());
        auto passwd = std::string(request.password_.begin(), request.password_.end());

        user = user.substr(0, strlen(user.c_str()));
        passwd = passwd.substr(0, strlen(passwd.c_str()));


        check_client_auth(conn, user, passwd);

        if (conn->is_authorized()) {
            request.authorized_ = true;
        }

        auto answer = data_from_struct(request, sizeof(request));
        write_handler_(conn, answer);

        data.resize(0);
        data.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void FileSyncManager::handle_msg_upload(const std::shared_ptr<ClientConnection>& conn,
    std::vector<uint8_t>& data)
{
    std::cout << __func__ << std::endl;

    struct nm_upload request;

    if (fillstruct(&request, sizeof(request), data)) {
        if (!conn->is_authorized()) {
            request.access_error_ = true;
        }
        else {
            auto filename = std::string(request.filename_.begin(), request.filename_.end());
            filename = filename.substr(0, strlen(filename.c_str()));

            uint32_t data_size = request.data_size_;

            std::vector<uint8_t> filedata(data_size);
            std::copy(request.data_.begin(), request.data_.begin() + data_size, filedata.begin());


            if (save_file(conn, filename, filedata)) {
                request.file_saved_ = true;
            }
        }


        auto answer = data_from_struct(request, sizeof(request));
        write_handler_(conn, answer);

        data.resize(0);
        data.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void FileSyncManager::handle_msg_download(const std::shared_ptr<ClientConnection>& conn,
    std::vector<uint8_t>& data)
{
    struct nm_download request;

    if (fillstruct(&request, sizeof(request), data)) {
        if (!conn->is_authorized()) {
            request.access_error_ = true;
        }
        else {
            auto filename = std::string(request.filename_.begin(), request.filename_.end());
            filename = filename.substr(0, strlen(filename.c_str()));

            filename = path_join({working_directory_, conn->username(), filename});

            if (!is_file(filename)) {
                request.file_not_found_ = true;
            }
            else {
                auto fsize = filesize(filename);
                if (fsize > static_cast<int32_t>(request.data_.size())) {
                    request.file_to_large_ = true;
                }
                else {
                    std::vector<uint8_t> filedata;
                    readfile(filename, &filedata);

                    std::copy(filedata.begin(), filedata.end(), request.data_.begin());
                    request.data_size_ = fsize;
                    request.file_found_ = true;
                }
            }
        }


        auto answer = data_from_struct(request, sizeof(request));
        write_handler_(conn, answer);

        data.resize(0);
        data.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

void FileSyncManager::handle_msg_get_filelist(const std::shared_ptr<ClientConnection>& conn,
    std::vector<uint8_t>& data)
{
    struct nm_get_filelist request;

    if (fillstruct(&request, sizeof(request), data)) {
        if (!conn->is_authorized()) {
            request.access_error_ = true;
        }
        else {
            std::cout << __func__ << " from " << conn->ip_str() << ":" << conn->port() << std::endl;


            auto dirname = conn->username();
            auto listing = get_dir_listing(dirname);


            if (listing.length() < request.list_.size()) {
                memcpy(request.list_.data(), listing.data(), listing.length());
            }
        }


        auto answer = data_from_struct(request, sizeof(request));
        write_handler_(conn, answer);

        data.resize(0);
        data.shrink_to_fit();
    }
}

//----------------------------------------------------------------------

bool FileSyncManager::save_file(const std::shared_ptr<ClientConnection>& conn,
    std::string filename, const std::vector<uint8_t>& filedata)
{
    std::stringstream out {filename};
    std::string item = "";

    std::deque<std::string> items;

    while (std::getline(out, item, '/')) {
        if (item.length()) {
            items.push_back(item);
        }
    }

    if (items.size() > 0) {
        filename = items.at(items.size() - 1);
    }

    filename = path_join({working_directory_, conn->username(), filename});


    int32_t fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd > 0) {
        write(fd, filedata.data(), filedata.size());
        close(fd);

        std::cout << "file saved" << std::endl;

        return true;
    }
    else {
        std::cout << "unable open file" << std::endl;
    }

    return false;
}

//----------------------------------------------------------------------

void FileSyncManager::verify_working_directory()
{
    if (!is_path_exist(working_directory_)) {
        create_directory(working_directory_);
    }
    else if (!is_directory(working_directory_)) {
        throw std::runtime_error("Please specify correct working directory path");
    }

    if (chdir(working_directory_.c_str()) != 0) {
        throw std::runtime_error("Unable chdir to working directory");
    }
}
