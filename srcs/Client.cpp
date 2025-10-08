#include "../includes/Client.hpp"

Client::Client(int fd, Server* srv)
    : fd(fd), endHeaders(false), reqComplete(false), hasBody(false), requestError(false), currentRequest(NULL), server(srv)
{
    bodySize = 0;
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

bool    Client::getRequestError() const
{
    return requestError;
}

void    Client::setBodySize(size_t size)
{
    bodySize = size;
}

Request* Client::getRequest() const
{
    return currentRequest;
}

const Response& Client::getResponse() const
{
    return currentResponse;
}

const Location*   Client::findMathLocation(const std::string& url)
{
    const std::vector<Location>& locations = server->getLocations();
    if (locations.empty())
        return NULL;

    const Location* currentLocation = NULL;
    std::size_t prefix = 0;
	for (size_t i = 0; i < locations.size(); i++)
	{
        std::string locationPath = locations[i].getPATH();
        if (url == locationPath)
            return &locations[i];
        else
        {
            if (url.substr(0, locationPath.length()) == locationPath)
            {
                if (prefix < locationPath.length())
                {
                    prefix = locationPath.length();
                    currentLocation = &locations[i];
                }
            }
        }
	}
    return currentLocation;
}

std::string    Client::joinPath(const Location& location)
{
    std::string updatePath = currentRequest->getPath();
    std::string root = location.getRoot();
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    if (location.getPATH() != "/")
    {
        if (updatePath.find(location.getPATH()) == 0)
        {
            updatePath = updatePath.substr(location.getPATH().length());
        }
    }
    std::string Total = root + updatePath;
    return Total;
}

bool    Client::allowedMethod(const Location& location, const std::string& method)
{
    std::vector<std::string> methods = location.getMethod();
    if (methods.empty())
        return false;
    std::vector<std::string>::iterator it;
    it = std::find(methods.begin(), methods.end(), method);
    if (it == methods.end())
        return false;
    return true;
}

void    Client::handleRedirection(const Location& location)
{
    std::pair<int, std::string> redir = location.getRedirection();
    std::string TextStatus = getStatusText(redir.first);
    std::string fullUrl = redir.second;
    if (fullUrl.find("http://") != 0 && fullUrl.find("https://") != 0)
        fullUrl.insert(0, "http://");

    std::stringstream body;
    body << "<!DOCTYPE HTML>"
        << "<html><head><title>" << redir.first << " " << TextStatus << "</title></head>"
        << "<body><h1>" << TextStatus << "</h1>"
        << "<p>Redirecting to: <a target=\"_blank\" href=\"" << fullUrl << "\">" << fullUrl << "</a></p>"
        << "</body></html>";
    
    std::string bodyStr = body.str();
    currentResponse = Response();
    currentResponse.setProtocol(currentRequest->getProtocol());
    currentResponse.setStatus(redir.first, TextStatus);
    // currentResponse.setHeaders("Location", fullUrl);
    currentResponse.setHeaders("Content-Type", "text/html");
    currentResponse.setHeaders("Content-Length", intTostring(bodyStr.length()));
    currentResponse.setHeaders("Date", currentDate());
    currentResponse.setHeaders("Connection", "close");
    currentResponse.setBody(bodyStr);
}


void    Client::handleGET()
{
    const Location* location = findMathLocation(currentRequest->getPath());
    if (location)
    {
        if (location->hasRedir())
        {
            handleRedirection(*location);
            return;
        }
        if (location->getRoot().empty())
        {
            errorResponse(500, "Missing root directive");
            return;
        }
        std::string totalPath = joinPath(*location);
        if (isDir(totalPath))
        {
            //handle directory
        }
    }
}

void    Client::handleCompleteRequest()
{
    if (currentRequest->getMethod() == "GET")
    {
        //TODO: handle get method
        handleGET();
    }
    else if (currentRequest->getMethod() == "DELETE")
    {
        //TODO: handle delete mothod
    }
    else
    {
        //TODO: handle post method
    }
}

void    Client::errorResponse(int code, std::string error)
{
    std::stringstream body;
    body << "<!DOCTYPE HTML>"
        << "<html><head><title>"<< code << " " << getStatusText(code) << "</title></head>"
        << "<body><h1>"<< code << " " << getStatusText(code) << "</h1>"
        << "<p>" << error << "</p>"
        << "</body></html>";

    std::string bodyStr = body.str();
    currentResponse = Response();
    currentResponse.setProtocol("http/1.0");
    currentResponse.setStatus(code, getStatusText(code));
    currentResponse.setHeaders("Content-Type", "text/html");
    currentResponse.setHeaders("Content-Length", intTostring(bodyStr.length()));
    currentResponse.setHeaders("Date", currentDate());
    currentResponse.setHeaders("Connection", "close");
    currentResponse.setBody(bodyStr);
}

void    Client::handleHeaders(const std::string& raw)
{
    std::cout << "Header request" << std::endl;
    std::cout << raw << std::endl;
    try
    {
        currentRequest = new Request();
        currentRequest->parseRequest(raw);
        // parsedRequest(*currentRequest);
        bodySize = currentRequest->getContentLength();
        if (bodySize > 0 && currentRequest->getMethod() == "POST")
            hasBody = true;
        else
            reqComplete = true;
    } catch (const std::exception& e)
    {
        reqComplete = true;
        requestError = true;
        errorResponse(400, e.what());
        // std::cout << "Request error: " << e.what() << std::endl;
    }
}

void    Client::handleBody(const char* buf, ssize_t length)
{
    size_t toAppend = std::min((size_t)length, bodySize);
    currentRequest->appendBody(buf, toAppend);
    bodySize -= toAppend;
    if (bodySize <= 0)
        reqComplete = true;
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

