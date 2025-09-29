#include "Client.hpp"

Client::Client(): fd(-1), keepAlive(true), requestComplete(false)
{
}

Client::Client(int fd): fd(fd), keepAlive(true), requestComplete(false)
{
}

Client::~Client()
{
}

// std::string& Client::get_readBuffer()
// {
//     return readBuffer;
// }

// std::string& Client::get_readBuffer()
// {
//     return writeBuffer;
// }

// bool        Client::get_keepAlive()
// {
//     return keepAlive;
// }

// bool        Client::get_requestComplete()
// {
//     return requestComplete;
// }


// void        set_readBuffer()
// {

// }

// void        set_readBuffer()
// {

// }

// bool        set_keepAlive()
// {

// }

// bool        set_requestComplete()
// {

// }
