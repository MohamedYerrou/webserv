#include "../includes/Client.hpp"

Client::Client(int fd, Server* srv)
    : fd(fd), endHeaders(false), reqComplete(false), hasBody(false), requestError(false), currentRequest(NULL), server(srv), location(NULL)
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

const Location*   Client::findMathLocation(std::string url)
{
    if (url[0] != '/')
        url.insert(0, "/");
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

std::string    Client::joinPath()
{
    std::string updatePath = currentRequest->getPath();
    std::string root = location->getRoot();
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    if (location->getPATH() != "/")
    {
        if (updatePath.find(location->getPATH()) == 0)
        {
            updatePath = updatePath.substr(location->getPATH().length());
        }
    }
    std::string Total = root + updatePath;
    return Total;
}

bool    Client::allowedMethod(const std::string& method)
{
    std::vector<std::string> methods = location->getMethod();
    if (methods.empty())
        return false;
    std::vector<std::string>::iterator it;
    it = std::find(methods.begin(), methods.end(), method);
    if (it == methods.end())
        return false;
    return true;
}

void    Client::handleRedirection()
{
    std::pair<int, std::string> redir = location->getRedirection();
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

void    Client::handleFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string bodyStr = buffer.str();
        currentResponse = Response();
        currentResponse.setProtocol(currentRequest->getProtocol());
        currentResponse.setStatus(200, "OK");
        currentResponse.setBody(bodyStr);
        currentResponse.setHeaders("Content-Type", getMimeType(path));
        currentResponse.setHeaders("Content-Length", intTostring(bodyStr.length()));
        currentResponse.setHeaders("Date", currentDate());
        currentResponse.setHeaders("Connection", "close");
    }
    else
    {
        errorResponse(500, strerror(errno));
    }
    file.close();
}

// void    Client::listingDirectory(const std::string& path)
// {
// }

void    Client::handleDirectory(const std::string& path)
{
    std::string indexPath = path;
    if (indexPath[indexPath.length() - 1] != '/')
        indexPath += '/';
    std::vector<std::string> index = location->getIndex();
    std::vector<std::string>::iterator it;
    bool    foundFile = false;
    for (it = index.begin(); it != index.end(); it++)
    {
        indexPath += *it;
        if (isFile(indexPath))
        {
            handleFile(indexPath);
            foundFile = true;
            break;
        }
    }
    if (!foundFile && location->getAutoIndex())
    {
        //TODO:
        // listingDirectory(path);
    }
    else if (!foundFile)
    {
        errorResponse(403, "Forbiden serving this directory");
    }
}


void    Client::handleGET()
{
    location = findMathLocation(currentRequest->getPath());
    if (location)
    {
        if (location->hasRedir())
        {
            handleRedirection();
            return;
        }
        if (location->getRoot().empty())
        {
            errorResponse(500, "Missing root directive");
            return;
        }
        std::string totalPath = joinPath();
        if (isDir(totalPath))
        {
            handleDirectory(totalPath);
        }
        if (isFile(totalPath))
        {
            handleFile(totalPath);
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

void    Client::errorResponse(int code, const std::string& error)
{
    if (!location)
        location = findMathLocation(currentRequest->getPath());
    std::map<int, std::string> errors = location->getErrors();
    std::map<int, std::string>::iterator it = errors.find(code);
    if (it != errors.end() && !it->second.empty())
    {
        std::string errorPage = it->second;
        std::string path = location->getRoot();
        if (path[path.length() - 1] == '/')
            path.erase(path.length() - 1);
        if (errorPage[0] != '/')
            errorPage.insert(0, "/");
        path += errorPage;
        if (isFile(path))
        {
            std::ifstream file((path).c_str(), std::ios::binary);
            if (file.is_open())
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string body = buffer.str();
                currentResponse = Response();
                currentResponse.setProtocol("HTTP/1.0");
                currentResponse.setStatus(code, getStatusText(code));
                currentResponse.setHeaders("Content-Type", getMimeType(path));
                currentResponse.setHeaders("Content-Length", intTostring(body.length()));
                currentResponse.setHeaders("Date", currentDate());
                currentResponse.setHeaders("Connection", "close");
                currentResponse.setBody(body);
                return;
            }
        }
    }
    std::stringstream body;
    body << "<!DOCTYPE HTML>"
        << "<html><head><title>"<< code << " " << getStatusText(code) << "</title></head>"
        << "<body><h1>"<< code << " " << getStatusText(code) << "</h1>"
        << "<p>" << error << "</p>"
        << "</body></html>";

    std::string bodyStr = body.str();
    currentResponse = Response();
    currentResponse.setProtocol("HTTP/1.0");
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
        if (currentRequest->getErrorVersion())
            errorResponse(505, e.what());
        else
        {
            std::cout << "HERRRRRRRR" << std::endl;
            errorResponse(400, e.what());
        }
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

