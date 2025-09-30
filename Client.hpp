#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <unistd.h>
#include "Request.hpp"

class Client
{
    private:
        int fd;
        std::string headers;
        size_t  bodySize;
        bool    endHeaders;
        bool    reqComplete;
        bool    hasBody;
        Request* currentRequest;
    public:
        Client(int fd);
        ~Client();
        int getFD() const;
        const std::string& getHeaders() const;
        bool    getEndHeaders() const;
        bool    getReqComplete() const;
        void    appendData(const char* buf, ssize_t length);
        void    setBodySize(size_t size);
        void    handleHeaders(const std::string& raw);
        void    handleBody(const char* buf, ssize_t length);
        Request* getRequest() const;
        void    handleCompleteRequest();
};

#endif