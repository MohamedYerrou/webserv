#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client
{
public:
    int			fd;
    std::string	readBuffer;
    std::string	writeBuffer;
    bool		keepAlive;
    bool		requestComplete;

    Client();
	Client(int fd);
    ~Client();

    std::string&	get_readBuffer();
    std::string&	get_writeBuffer();
    bool			get_keepAlive();
    bool			get_requestComplete();
};

#endif