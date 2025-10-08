#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <unistd.h>
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include <sys/stat.h>
#include "Utils.hpp"
#include <algorithm>
#include <fstream>

class Client
{
    private:
        int fd;
        std::string headers;
        size_t  bodySize;
        bool    endHeaders;
        bool    reqComplete;
        bool    hasBody;
        bool    requestError;
        Request* currentRequest;
        Server* server;
        Response currentResponse;
    public:
        Client(int fd, Server* srv);
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
        const Response& getResponse() const;
        bool    getRequestError() const;
        
        //handle methods
        void    handleGET();
        const Location*   findMathLocation(const std::string& url);
        std::string    joinPath(const Location& location);
        bool    allowedMethod(const Location& location, const std::string& method);
        void    handleRedirection(const Location& location);
        void    errorResponse(int code, std::string error);
};

#endif
