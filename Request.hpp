#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cctype>

class Request
{
    private:
        std::string method;
        std::string uri;
        std::string path;
        std::string protocol;
        std::map<std::string, std::string> headers;
        std::string body;
    public:
        Request();
        ~Request();
        const std::string& getMethod() const;
        const std::string& getUri() const;
        const std::string& getProtocol() const;
        const std::map<std::string, std::string>& getHeaders() const;
        const std::string& getBody() const;
        void    parseRequest(const std::string& raw);
        void    parseLine(const std::string& raw);
        void    parseHeaders(const std::string& raw);
        void    parseBody(const std::string& raw);
        size_t  getContentLength() const;
        bool    parseMethod(const std::string& method);
        bool    parseUri(const std::string& uri);
        std::string normalizePath(const std::string& path);
        std::string decodeUri(const std::string& uri);
};

std::string trim(std::string str);
int stringToInt(const std::string& str, int base = 10);
void    parsedRequest(Request req);

#endif 