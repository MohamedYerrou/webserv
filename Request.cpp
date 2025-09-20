#include "Request.hpp"

Request::Request()
{
}

Request::~Request()
{
}

const std::string& Request::getMethod() const
{
    return method;
}

const std::string& Request::getUri() const
{
    return uri;
}

const std::string& Request::getProtocol() const
{
    return protocol;
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
    return headers;
}

const std::string& Request::getBody() const
{
    return body;
}

size_t  Request::getContentLength() const
{
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length");
    if (it != headers.end())
    {
        int length = stringToInt(it->second);
        if (length >= 0)
            return static_cast<size_t>(length);
    }
    return 0;
}

std::string trim(std::string str)
{
    std::size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    std::size_t end = str.find_last_not_of(" \t");
    return (str.substr(start, end - start + 1));
}

int stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int result;
    ss >> result;
    return result;
}

void    Request::parseLine(const std::string& raw)
{
    std::size_t pos = raw.find("\r\n");
    if (pos != std::string::npos)
    {
        std::string line = raw.substr(0, pos);
        std::stringstream str(line);
        str >> method >> uri >> protocol;
    }
    else
        throw std::runtime_error("Bad request");
}

void    Request::parseHeaders(const std::string& raw)
{
    std::size_t pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos)
        throw std::runtime_error("Bad request");
    pos = raw.find("\r\n");
    pos += 2;
    std::size_t currentPos = raw.find("\r\n", pos);
    while (currentPos != std::string::npos)
    {
        std::string headerLine = raw.substr(pos, currentPos - pos);
        if (headerLine.empty())
            break ;
        std::size_t colonPos = headerLine.find(":");
        if (colonPos != std::string::npos)
        {
            std::string key = trim(headerLine.substr(0, colonPos));
            std::string value = trim(headerLine.substr(colonPos + 1));
            if (!key.empty())
                headers[key] = value;
        }
        else
            throw std::runtime_error("Bad request");
        pos = currentPos + 2;
        currentPos = raw.find("\r\n", pos);
    }
}

void    Request::parseBody(const std::string& raw)
{
    
}

void    parsedRequest(Request req)
{
    std::cout << "========================================" << std::endl;
    std::cout << "=============Parsed Request=============" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "Uri: " << req.getUri() << std::endl;
    std::cout << "Protocol: " << req.getProtocol() << std::endl;
    std::cout << std::endl;

    std::cout << "================Headers================" << std::endl;
    std::map<std::string, std::string> headers = req.getHeaders();
    std::map<std::string, std::string>::iterator it = headers.begin();
    for (; it != headers.end(); it++)
    {
        std::cout << it->first << "=  " << it->second << std::endl;
    }

    std::cout << "\n=================Body=================" << std::endl;
    std::cout << req.getBody() << std::endl;
}



void    Request::parseRequest(const std::string& raw)
{
    // std::cout << "========================================" << std::endl;
    // std::cout << "This from Request class: " << std::endl;
    // std::cout << "========================================" << std::endl;

    // std::cout << raw << std::endl;
 
    parseLine(raw);
    parseHeaders(raw);
    
    //TODO: body
    parseBody(raw);
}

