#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <cstring>
#include <string>
#include <vector>



void create_directory(const std::string&);

//----------------------------------------------------------------------

int32_t filesize(const std::string&);

//----------------------------------------------------------------------

std::string get_ip_str(uint32_t);

//----------------------------------------------------------------------

bool is_directory(const std::string&);

//----------------------------------------------------------------------

bool is_file(const std::string&);

//----------------------------------------------------------------------

bool is_path_exist(const std::string&);

//----------------------------------------------------------------------

std::string path_join(std::initializer_list<std::string>);

//----------------------------------------------------------------------

bool readfile(const std::string&, std::vector<uint8_t>*);

//----------------------------------------------------------------------

bool set_socket_nonblocking_mode(uint32_t);

//----------------------------------------------------------------------

template<typename T>
std::vector<uint8_t> data_from_struct(T& t, uint32_t size)
{
    std::vector<uint8_t> data(size);

    memcpy(data.data(), &t, size);

    return data;
}

//----------------------------------------------------------------------

template<typename T>
bool fillstruct(T* t, uint32_t size, const std::vector<uint8_t>& data)
{
    if (size != data.size()) {
        return false;
    }

    memcpy(t, data.data(), size);

    return true;
}


#endif // __UTIL_HPP__
