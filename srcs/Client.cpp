#include "../includes/Client.hpp"

Client::Client(int fd, Server* srv)
    : fd(fd), endHeaders(false), reqComplete(false), hasBody(false), requestError(false), currentRequest(NULL), currentServer(srv), location(NULL)
{
    bodySize = 0;
    readPos = 0;
    inPart = false;
    headersParsed = false;
	toAppend = 0;
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

ssize_t Client::getBodySize() const
{
    return  bodySize;
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
    const std::vector<Location>& locations = currentServer->getLocations();
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
    currentResponse.setHeaders("Location", fullUrl);
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

void    Client::listingDirectory(std::string path)
{
    DIR* dirStream = opendir(path.c_str());
    if (!dirStream)
    {
        errorResponse(500, "Could not open this directory");
    }
    std::string buffer;
    buffer +=  "<!DOCTYPE html><html><head><title>Listing directory: "
        + path + "</title></head><body><h1> Listing directory: " + path + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dirStream)) != NULL)
    {
        std::string url = currentRequest->getPath();
        if (url[url.length() - 1] != '/')
            url += '/';
        std::string fullPath = url + entry->d_name;
        buffer += "<li><a href=\"" + fullPath + "\">";
        buffer += entry->d_name;
        buffer += "</a></li>";
    }
    buffer += "</ul></body></html>";
    closedir(dirStream);
    currentResponse = Response();
    currentResponse.setProtocol(currentRequest->getProtocol());
    currentResponse.setStatus(200, "ok");
    currentResponse.setBody(buffer);
    currentResponse.setHeaders("Content-Type", "text/html");
    currentResponse.setHeaders("Content-Length", intTostring(buffer.length()));
    currentResponse.setHeaders("Date", currentDate());
    currentResponse.setHeaders("Connection", "close");
    
    currentResponse.setHeaders("Set-Cookie", "session_id=" + sess->id + ";");
}

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
        listingDirectory(path);
    else if (!foundFile)
        errorResponse(403, "Forbiden serving this directory");
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
        handleGET();
    else if (currentRequest->getMethod() == "DELETE")
    {
          //TODO: handle delete mothod
    }
    else
    {
        // std::cout << "ooooooooooooooooooo" << std::endl;
        handlePost();
    }
}

void    Client::errorResponse(int code, const std::string& error)
{
    if (!location)
        location = findMathLocation(currentRequest->getPath());
    std::map<int, std::string> errors = location->getErrors();
    std::map<int, std::string>::iterator it = errors.find(code);
    requestError = true;
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
        std::cout << "wa fin " << bodySize << std::endl;
        if (bodySize > 0 && currentRequest->getMethod() == "POST")
        {
			if ((size_t)bodySize > currentServer->getMaxBodySize())
				errorResponse(413, "");
            hasBody = true;
        }
        else if (bodySize == -1 && currentRequest->getMethod() == "POST")
        {
            errorResponse(400, "");
            // reqComplete = true;
        }
        else if (bodySize == -2 && currentRequest->getMethod() == "POST")
        {
            errorResponse(411, "");
            // reqComplete = true;
        }
        else 
            reqComplete = true; 
    } catch (const std::exception& e)
    {
        reqComplete = true;
        requestError = true;
        if (currentRequest->getErrorVersion())
        {
            errorResponse(505, e.what());
            return;
        }
        else
            errorResponse(400, e.what());
        // std::cout << "Request error: " << e.what() << std::endl;
    }
}

const Location* Client::findBestMatch(const std::string uri)
{
    int index = -1;
    const std::vector<Location>& locations = currentServer->getLocations();

    for (size_t i = 0; i < locations.size(); i++)
    {
        std::string loc = locations[i].getPATH();

        if (uri.compare(0, loc.length(), loc) == 0
            && (uri.length() == loc.length() || uri[loc.length()] == '/' || loc == "/"))
        {
            if (index != -1 && locations[index].getPATH().length() <  locations[i].getPATH().length())
                index = i;
            else if (index == -1)
                index = i;
        }
    }
    if (index == -1)
        return NULL;
    return &locations[index];
}

std::string Client::constructFilePath(std::string uri)
{
    std::string path;
    const Location*   loc = findBestMatch(uri);

    path = loc->getUploadStore() + uri.erase(0, loc->getPATH().length());
    return path;
}

std::map<std::string, std::string>    Client::parseCookies(std::string cookieHeader)
{
    std::map<std::string, std::string> cookies;
    std::stringstream   ss(cookieHeader);
    std::string         pair;

    while (getline(ss, pair, ';'))
    {
        size_t eq = pair.find('=');
        std::string key = pair.substr(0, eq);
        std::string value = pair.substr(eq + 1);
        if (key[0] == ' ')
            key.erase(0, 1);
        cookies[key] = value;
    }
    return cookies;
}

void    Client::handleSession()
{
    if (currentRequest->getHeaders().count("Cookie") == 0)
    {
        std::string sid = generateSessionId();
        currentServer->getSessions()[sid] = session();
        currentServer->getSessions()[sid].id = sid;
        sess = &currentServer->getSessions()[sid];
        return ;
    }
    else
        cookies = parseCookies(currentRequest->getHeaders().at("Cookie"));

    if (cookies.count("session_id"))
    {
        std::string sid = cookies["session_id"];
        if (currentServer->getSessions().count(sid))
        {
            sess = &currentServer->getSessions().at(sid);
            sess->last_access = time(NULL);
        }
    }
}

void    Client::appendData(const char* buf, ssize_t length)
{
    if (!endHeaders)
    {
        // std::cout << "Buf: " << buf << std::endl;
        headers.append(buf, length);
        std::size_t headerPos = headers.find("\r\n\r\n");
        if (headerPos != std::string::npos)
        {
            endHeaders = true;
            headerPos += 4;
            handleHeaders(headers.substr(0, headerPos));
            handleSession();
            size_t bodyInHeader = headers.length() - headerPos;
            if (hasBody && bodyInHeader > 0 && requestError)
            {
                std::string bodyStart = headers.substr(headerPos);
                drainSocket(bodyStart.size());
            }
            else if (hasBody && bodyInHeader > 0)
            {
                // std::string target_path = constructFilePath(currentRequest->getPath());
                // filename = currentRequest->generateTmpFile(target_path);
                std::string bodyStart = headers.substr(headerPos);
                handleBody(bodyStart.c_str(), bodyStart.length());
            }
        }
    }
    else
    {
        if (hasBody && requestError)
            drainSocket(length);
        else if (hasBody && !reqComplete)
            handleBody(buf, length);
    }
}
