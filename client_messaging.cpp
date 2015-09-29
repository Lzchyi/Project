#ifndef __CLIENT_MESSAGING_HPP__
#define __CLIENT_MESSAGING_HPP__



enum class ClientMessageType : uint8_t
{
    CMD_AUTH = 0,
    CMD_UPLOAD,
    CMD_DOWNLOAD,
    CMD_LS
};

//----------------------------------------------------------------------

struct m_cmd_auth
{
    ClientMessageType msg_type = ClientMessageType::CMD_AUTH;
    std::array<uint8_t, 64> username_;
    std::array<uint8_t, 64> password_;
    m_cmd_auth() {username_.fill(0); password_.fill(0);};
};

//----------------------------------------------------------------------

struct m_cmd_upload
{
    ClientMessageType msg_type = ClientMessageType::CMD_UPLOAD;
    std::array<uint8_t, 256> filename_;
    m_cmd_upload() {filename_.fill(0);};
};

//----------------------------------------------------------------------

struct m_cmd_download
{
    ClientMessageType msg_type = ClientMessageType::CMD_DOWNLOAD;
    std::array<uint8_t, 64> filename_;
    m_cmd_download() {filename_.fill(0);};
};

//----------------------------------------------------------------------

struct m_cmd_ls
{
    ClientMessageType msg_type = ClientMessageType::CMD_LS;
};


#endif // __CLIENT_MESSAGING_HPP__
