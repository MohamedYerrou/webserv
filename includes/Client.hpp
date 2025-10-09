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
#include <dirent.h>

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
        const Location* location;
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
        const Location*   findMathLocation(std::string url);
        std::string    joinPath();
        bool    allowedMethod(const std::string& method);
        void    handleRedirection();
        void    errorResponse(int code, const std::string& error);
        void    handleDirectory(const std::string& path);
        void    handleFile(const std::string& path);
        void    listingDirectory(std::string path);
};

#endif
