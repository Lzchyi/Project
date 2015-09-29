#ifndef __CLIENT_CONNECTION_HPP__
#define __CLIENT_CONNECTION_HPP__

#include <cstdint>
#include <string>



class ClientConnection
{
    public:
        ClientConnection(uint32_t, uint32_t, uint16_t);

        void authorize(std::string, std::string);
        bool is_authorized() const;
        uint32_t ip() const;
        std::string ip_str() const;
        uint16_t port() const;
        uint32_t sock() const;
        std::string username() const;
    private:
        uint32_t sock_;
        uint32_t ip_;
        uint16_t port_;
        std::string username_;
        std::string password_;
        bool is_authorized_ = false;
};


#endif // __CLIENT_CONNECTION_HPP__
