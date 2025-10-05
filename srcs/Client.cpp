#include "../includes/Client.hpp"
#include "../includes/Server.hpp"
#include <fstream>
#include <bits/stdc++.h>

Client::Client(int fd, Server* server)
    : fd(fd), endHeaders(false), reqComplete(false), hasBody(false), currentRequest(NULL)
{
    bodySize = 0;
    currentServer = server;
}

Client::~Client()
{
}

int Client::getFD() const
{
    return fd;
}

const std::string& Client::getHeaders() const
{
    return headers;
}

bool    Client::getEndHeaders() const
{
    return endHeaders;
}

bool    Client::getReqComplete() const
{
    return reqComplete;
}

void    Client::setBodySize(size_t size)
{
    bodySize = size;
}

Request* Client::getRequest() const
{
    return currentRequest;
}



// void    Client::handlePost()
// {
//     std::string		Content_type = (currentRequest->getHeaders())["Content-Type"];
//     std::fstream	body("testfile", std::ios::in);
//     std::string		line;

//     if (Content_type == "application/x-www-form-urlencoded")
//     {
//         while (std::getline(body, line, '&'))
// 		{
// 			line
// 		}
//         return ;
//     }
//     else if (Content_type == "multipart/form-data")
//         return ;
//     else if (Content_type == "text/plain")
// }

// void    Client::handleCompleteRequest()
// {
//     if (currentRequest->getMethod() == "GET")
//     {
//         //TODO: handle get method
//     }
//     else if (currentRequest->getMethod() == "DELETE")
//     {
//         //TODO: handle delete mothod
//     }
//     else
//     {
//         //TODO: handle post method
//     }
// }

void    Client::handleHeaders(const std::string& raw)
{
    std::cout << "Header request" << std::endl;
    std::cout << raw << std::endl;
    try
    {
        currentRequest = new Request();
        currentRequest->parseRequest(raw);
        bodySize = currentRequest->getContentLength();
        if (bodySize > 0)
            hasBody = true;
        else
            reqComplete = true;
    } catch (const std::exception& e)
    {
        std::cout << "Request error: " << e.what() << std::endl;
    }
}

void    Client::handleBody(const char* buf, ssize_t length)
{
    size_t toAppend = std::min((size_t)length, bodySize);
    currentRequest->appendBody(buf, toAppend);
    bodySize -= toAppend;
    if (bodySize <= 0)
    {
        std::cout << "Body complete" << std::endl;
        reqComplete = true;
    }
}

void    Client::appendData(const char* buf, ssize_t length)
{
    if (!endHeaders)
    {
        std::cout << "Buf: " << buf << std::endl;
        headers.append(buf, length);
        std::size_t headerPos = headers.find("\r\n\r\n");
        if (headerPos != std::string::npos)
        {
            endHeaders = true;
            headerPos += 4;
            handleHeaders(headers.substr(0, headerPos));
            size_t bodyInHeader = headers.length() - headerPos;
            if (hasBody && bodyInHeader > 0)
            {
                currentRequest->generateTmpFile();
                std::string bodyStart = headers.substr(headerPos);
                handleBody(bodyStart.c_str(), bodyStart.length());
            }
        }
    }
    else
    {
        if (hasBody && !reqComplete)
            handleBody(buf, length);
    }
}
