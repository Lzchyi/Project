#ifndef __NETWORK_MESSAGING_HPP__
#define __NETWORK_MESSAGING_HPP__



enum class NetworkMessageType : uint8_t
{
    AUTHORIZE = 0,
    UPLOAD,
    DOWNLOAD,
    GET_FILELIST
};

//----------------------------------------------------------------------

struct nm_authorize
{
    NetworkMessageType msg_type = NetworkMessageType::AUTHORIZE;
    std::array<uint8_t, 64> username_;
    std::array<uint8_t, 64> password_;
    bool authorized_ = false;
    nm_authorize() {username_.fill(0); password_.fill(0);};
};

//----------------------------------------------------------------------

struct nm_upload
{
    NetworkMessageType msg_type = NetworkMessageType::UPLOAD;
    std::array<uint8_t, 256> filename_;
    std::array<uint8_t, 10240> data_; // 10 kb
    uint32_t data_size_;
    bool file_saved_ = false;
    bool access_error_ = false;
    nm_upload() {filename_.fill(0); data_.fill(0);};
};

//----------------------------------------------------------------------

struct nm_download
{
    NetworkMessageType msg_type = NetworkMessageType::DOWNLOAD;
    std::array<uint8_t, 64> filename_;
    std::array<uint8_t, 10240> data_; // 10 kb
    uint32_t data_size_;
    bool file_found_ = false;
    bool file_not_found_ = false;
    bool file_to_large_ = false;
    bool access_error_ = false;
    nm_download() {filename_.fill(0); data_.fill(0);};
};

//----------------------------------------------------------------------

struct nm_get_filelist
{
    NetworkMessageType msg_type = NetworkMessageType::GET_FILELIST;
    std::array<uint8_t, 10240> list_;
    bool access_error_ = false;
    nm_get_filelist() {list_.fill(0);};
};


#endif // __NETWORK_MESSAGING_HPP__
