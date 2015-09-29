#include "client_connection.hpp"
#include "util.hpp"



ClientConnection::ClientConnection(uint32_t sock, uint32_t ip, uint16_t port):
    sock_(sock), ip_(ip), port_(port)
{
}

//----------------------------------------------------------------------

void ClientConnection::authorize(std::string username, std::string password)
{
    username_ = username;
    password_ = password;
    is_authorized_ = true;
}

//----------------------------------------------------------------------

bool ClientConnection::is_authorized() const
{
    return is_authorized_;
}

//----------------------------------------------------------------------

uint32_t ClientConnection::ip() const
{
    return ip_;
}

//----------------------------------------------------------------------

std::string ClientConnection::ip_str() const
{
    return get_ip_str(ip_);
}

//----------------------------------------------------------------------

uint16_t ClientConnection::port() const
{
    return port_;
}

//----------------------------------------------------------------------

uint32_t ClientConnection::sock() const
{
    return sock_;
}

//----------------------------------------------------------------------

std::string ClientConnection::username() const
{
    return username_;
}
