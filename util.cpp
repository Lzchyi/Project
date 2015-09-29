#include "util.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>



void create_directory(const std::string& path)
{
    bool is_absolute_path = false;
    if (path.find("/") == 0) {
        is_absolute_path = true;
    }


    std::stringstream ss {path};
    std::string item = "";

    std::vector<std::string> items;

    while (std::getline(ss, item, '/')) {
        if (item.length()) {
            items.push_back(item);
        }
    }

    std::string current = "";
    if (is_absolute_path) {
        current = "/";
    }


    for (const auto& item: items) {
        if (!current.length() || current == "/") {
            current = current + item;
        }
        else {
            current = current + "/" + item;
        }

        mkdir(path.c_str(), 0777);
    }
}

//----------------------------------------------------------------------

int32_t filesize(const std::string& filename)
{
    struct stat st;
    if (stat(filename.c_str(), &st) == 0) {
        return st.st_size;
    }

    return -1;
}

//----------------------------------------------------------------------

std::string get_ip_str(uint32_t ip)
{
    struct in_addr addr;
    memset(&addr, 0, sizeof(addr));

    addr.s_addr = htonl(ip);

    return inet_ntoa(addr);
}

//----------------------------------------------------------------------

bool is_directory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------

bool is_file(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------

bool is_path_exist(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------

std::string path_join(std::initializer_list<std::string> lst)
{
    if (!lst.size()) {
        return "";
    }

    bool is_root = false;
    std::stringstream ss;

    uint32_t count = 0;
    for (const auto &str: lst) {
        if (count == 0) {
            if (str.find("/") == 0) {
                is_root = true;
            }
        }

        ss << "/" << str;
        ++count;
    }

    std::stringstream ss_cleaned;
    if (is_root) {
        ss_cleaned << "/";
    }

    std::string item = "";

    while (std::getline(ss, item, '/')) {
        if (!item.length()) {
            continue;
        }

        ss_cleaned << item << "/";
    }


    std::string joinedpath = ss_cleaned.str();

    if (joinedpath.length() > 1) {
        if (joinedpath.rfind("/") == (joinedpath.length() - 1)) {
            joinedpath = joinedpath.substr(0, (joinedpath.length() - 1));
        }
    }

    return joinedpath;
}

//----------------------------------------------------------------------

bool readfile(const std::string& filename, std::vector<uint8_t>* data)
{
    std::ifstream f(filename, std::ios::binary);

    if (!f.is_open()) {
        return false;
    }

    try {
        f.seekg(0, f.end);

        uint32_t filesize = f.tellg();

        data->reserve(filesize);

        f.seekg(0, f.beg);
        data->assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

        f.close();

        return true;
    }
    catch (...) {
        f.close();
    }

    return false;
}

//----------------------------------------------------------------------

bool set_socket_nonblocking_mode(uint32_t sock)
{
    int32_t flags = 0;
    flags = fcntl(sock, F_GETFD, 0);
    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) < 0) {
        return false;
    }

    return true;
}
