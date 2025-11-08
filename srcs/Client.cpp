#include "../includes/Client.hpp"
#include "../includes/Session.hpp"

Client::Client(int fd, Server* srv)
    : fd(fd),
      bodySize(0),
      endHeaders(false),
      reqComplete(false),
      hasBody(false),
      requestError(false),
      currentRequest(NULL),
      currentServer(srv),
      location(NULL),
      sentAll(false),
      fileOpened(false),
      postFileOpened(false),
      oneBody(false),
      inBody(false),
      finishBody(false),
      lastActivityTime(time(NULL)),
      sess(NULL),
      cgiHandler(NULL),
      isCGI(false)
{
}

Client::~Client()
{
    if (fileOpened)
        fileStream.close();
    if (currentRequest != NULL)
        delete currentRequest;
    if (cgiHandler != NULL)
    {
        delete cgiHandler;
        cgiHandler = NULL;
    }
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

Response& Client::getResponse()
{
    return currentResponse;
}

bool				Client::getSentAll() const
{
    return sentAll;
}

void				Client::setSentAll(bool flag)
{
    sentAll = flag;
}

bool			Client::isTimedOut()
{
    time_t currentTime = time(NULL);
    time_t elapsed = currentTime - lastActivityTime;
    return elapsed > REQUEST_TIMEOUT;
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
    if (currentRequest->getHeaders().count("cookie") == 0)
    {
        std::string sid = generateSessionId();
        currentServer->getSessions()[sid] = Session();
        currentServer->getSessions()[sid].id = sid;
        sess = &currentServer->getSessions()[sid];
        sess->data = currentRequest->getQueries();
        return ;
    }
    else
    {
        cookies = parseCookies(currentRequest->getHeaders().at("cookie"));
        
    }

    if (cookies.count("session_id"))
    {
        std::string sid = cookies["session_id"];
        if (currentServer->getSessions().count(sid))
        {
            sess = &currentServer->getSessions().at(sid);
            sess->last_access = time(NULL);
            sess->data = currentRequest->getQueries();
        }
    }
}

bool    			Client::getContentType()
{
    std::map<std::string, std::string>::const_iterator it = currentRequest->getHeaders().find("content-type");
    if (it !=  currentRequest->getHeaders().end())
    {
        std::string type = it->second;
        if (type.find("multipart/form-data") == 0)
        {
            std::size_t pos = type.find("boundary=");
            if (pos != std::string::npos)
            {
                boundary = type.substr(pos + 9);
                std::size_t semicolon = boundary.find(';');
                if (semicolon != std::string::npos)
                    boundary = boundary.substr(0, semicolon);
                if (boundary.length() >= 2 && boundary[0] == '"' && boundary[boundary.length() - 1] == '"')
                    boundary = boundary.substr(1, boundary.length() - 2);
                boundary = "--" + boundary;
                endBoundry = boundary + "--";
                return true;
            }
            else
            {
                errorResponse(400, "Invalid multipart body.");
                reqComplete = true;
                return false;
            }
        }
    }
    return false;
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

void    Client::handleFile()
{
    if (!fileOpened)
    {
        sentAll = true;
        return;
    }
    char    buffer[7000];
    fileStream.read(buffer, sizeof(buffer));
    ssize_t bytesRead = fileStream.gcount();
    if (bytesRead == 0 || bytesRead == -1)
        sentAll = true;
    else
    {
        std::string message;
        Session     sess;
        if (currentRequest->getPath() == "/profile.html")
        {
            if (currentRequest->getHeaders().count("cookie"))
            {
                std::string sid = cookies["session_id"];
                if (currentServer->getSessions().count(sid))
                {
                    sess = currentServer->getSessions().at(sid);
                    message = "Welcome back " + sess.data["username"] + ", your session_id is: " + sess.id;
                }
                else
                    message = "Your session key has been expired";
            }
            else
            {
                message = "Welcome guest";
            }
            std::stringstream body;
            body << "<!DOCTYPE HTML>"
                << "<html><head></head>"
                << "<body><h1>Cookies and Sessions example</h1>"
                << "<p>" << message << "</p>"
                << "</body></html>";
            currentResponse.setHeaders("Content-Length", intTostring(body.str().size()));
            currentResponse.setBody(body.str());
            sentAll = true;
            return;
        }

        currentResponse.setBody(std::string(buffer, bytesRead));
        if (bytesRead < (ssize_t)sizeof(buffer))
            sentAll = true;
    }
}

const Location*   Client::findMathLocation(std::string url)
{
    if (url[0] != '/') //this if got an error in parsing
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

void Client::handleCompleteRequest()
{
	location = findMathLocation(currentRequest->getPath());
    if (location->getRoot().empty())
    {
        errorResponse(500, "Missing root directive");
        return;
    }
	newPath = joinPath();
	
	const std::map<std::string, std::string>& cgiMap = location->getCgi();
	if (!cgiMap.empty())
	{
		try
		{
			checkCGIValid();
			if (getIsCGI())
				return;
		}
		catch (const std::exception& e)
		{
			std::cerr << "CGI Error: " << e.what() << std::endl;
			return errorResponse(500, "Internal Server Error");
		}
	}
	if (currentRequest->getMethod() == "GET")
		handleGET();
	else if (currentRequest->getMethod() == "DELETE")
		handleDELETE();
}

void    Client::defaultResponse(int code, const std::string& error)
{
    std::stringstream body;
    body << "<!DOCTYPE html>"
        << "<html lang=\"en\">"
        << "<head>"
        << "<meta charset=\"UTF-8\">"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        << "<title>" << code << " - " << getStatusText(code) << "</title>"
        << "<style>"
        << "* { margin: 0; padding: 0; box-sizing: border-box; }"
        << "@keyframes gradientBG {"
        << "    0% { background-position: 0% 50%; }"
        << "    50% { background-position: 100% 50%; }"
        << "    100% { background-position: 0% 50%; }"
        << "}"
        << "body {"
        << "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;"
        << "    background: linear-gradient(-45deg, "
        << (code >= 500 ? "#ff6b6b, #f06595, #cc5de8, #845ef7" : 
            (code >= 400 ? "#ffd43b, #ff922b, #ff6b6b, #f06595" : 
                         "#74c0fc, #4dabf7, #228be6, #15aabf")) << ");"
        << "    background-size: 400% 400%;"
        << "    animation: gradientBG 15s ease infinite;"
        << "    height: 100vh;"
        << "    display: flex;"
        << "    align-items: center;"
        << "    justify-content: center;"
        << "    color: #2d3436;"
        << "}"
        << ".error-container {"
        << "    background: rgba(255, 255, 255, 0.92);"
        << "    padding: 3.5rem 4rem;"
        << "    border-radius: 24px;"
        << "    box-shadow: 0 20px 40px rgba(0, 0, 0, 0.15);"
        << "    text-align: center;"
        << "    max-width: 600px;"
        << "    width: 90%;"
        << "    border: 1px solid rgba(255, 255, 255, 0.2);"
        << "    backdrop-filter: blur(10px);"
        << "}"
        << ".error-code {"
        << "    font-size: 9rem;"
        << "    font-weight: 800;"
        << "    background: linear-gradient(45deg, "
        << (code >= 500 ? "#ff6b6b, #e74c3c" : (code >= 400 ? "#ffd43b, #e67e22" : "#74c0fc, #3498db")) << ");"
        << "    -webkit-background-clip: text;"
        << "    -webkit-text-fill-color: transparent;"
        << "    line-height: 1;"
        << "    margin-bottom: 1.5rem;"
        << "    text-shadow: 3px 3px 6px rgba(0, 0, 0, 0.1);"
        << "    letter-spacing: -2px;"
        << "}"
        << ".error-title {"
        << "    font-size: 2.5rem;"
        << "    font-weight: 700;"
        << "    margin-bottom: 1.5rem;"
        << "    color: #1a1c1d;"
        << "    letter-spacing: -0.5px;"
        << "    text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.1);"
        << "}"
        << ".error-message {"
        << "    font-size: 1.2rem;"
        << "    color: #4a4f52;"
        << "    line-height: 1.7;"
        << "    text-shadow: 0 1px 1px rgba(255, 255, 255, 0.8);"
        << "}"
        << "</style>"
        << "</head>"
        << "<body>"
        << "<div class=\"error-container\">"
        << "<div class=\"error-code\">" << code << "</div>"
        << "<h1 class=\"error-title\">" << getStatusText(code) << "</h1>"
        << "<p class=\"error-message\">" << error << "</p>"
        << "</div>"
        << "</body>"
        << "</html>";

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
        if (!path.empty() && path[path.length() - 1] == '/')
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
                file.close();
                return;
            }
        }
    }
    defaultResponse(code, error);
}

void    Client::handleHeaders(const std::string& raw)
{
    // std::cout << raw << std::endl;
    try
    {
        currentRequest = new Request();
        currentRequest->parseRequest(raw);
        // parsedRequest(*currentRequest);
        bodySize = currentRequest->getContentLength();
        if (bodySize > currentServer->getMaxBodySize())
        {
            errorResponse(413, "Payload Too Large");
            reqComplete = true;
        }
        else if (bodySize == 0 && currentRequest->getMethod() == "POST")
        {
            errorResponse(411, "Length Required");
            reqComplete = true;
        }
        else if (bodySize > 0 && currentRequest->getMethod() == "POST")
            hasBody = true;
        else
            reqComplete = true;
    } catch (const std::exception& e)
    {
        reqComplete = true;
        requestError = true;
        errorResponse(currentRequest->getStatusCode(), e.what());
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
    std::string uploadFile = loc->getUploadStore();
    if (uploadFile.empty())
    {
        errorResponse(500, "MISSING UPLOAD STORE");
        reqComplete = true;
        return "";
    }
    path = uploadFile + uri.erase(0, loc->getPATH().length());
    return path;
}

void    Client::handlePostError()
{
    location = findMathLocation(currentRequest->getPath());
    target_path = constructFilePath(currentRequest->getPath());
    if (!allowedMethod("POST"))
    {
        reqComplete = true;
        errorResponse(405, "Method not allowed");
    }
    else if (location->getUploadStore().empty())
    {
        reqComplete = true;
        errorResponse(422, "Unprocessable Content");
    }
    else if (!isDir(location->getUploadStore()))
    {
        reqComplete = true;
        std::cout << "NOT A DIRECTORY " << location->getUploadStore() <<  std::endl;
        errorResponse(422, "Unprocessable Content");
    }
    else if (access(location->getUploadStore().c_str(), X_OK | W_OK) == -1)
    {
        reqComplete = true;
        errorResponse(403, "Forbidden");
    }
    else if (!isDir(target_path) || target_path.empty())
    {
        reqComplete = true;
        requestError = true;
        errorResponse(404, "NOT FOUND");
    }
}

void    Client::appendData(const char* buf, ssize_t length)
{
    if (!endHeaders)
    {
        headers.append(buf, length);
        std::size_t headerPos = headers.find("\r\n\r\n");
        if (headerPos != std::string::npos)
        {
            endHeaders = true;
            headerPos += 4;
            handleHeaders(headers.substr(0, headerPos));
            if (currentRequest->getMethod() == "POST")
            {
                handlePostError();
                if (reqComplete)
                    return;
            }
            handleSession();
            size_t bodyInHeader = headers.length() - headerPos;
            if (hasBody && bodyInHeader > 0)
            {
                if (!getContentType())
                {
                    if (reqComplete)
                        return;
                    oneBody = true;
                    // currentRequest->generateTmpFile(target_path, "");
                }
                std::string bodyStart = headers.substr(headerPos);
                handlePost(bodyStart.c_str(), bodyStart.length());
            }
        }
    }
    else
    {
        if (hasBody && !reqComplete)
            handlePost(buf, length);
    }
}

void Client::cleanupCGI()
{
	if (cgiHandler)
	{
		delete cgiHandler;
		cgiHandler = NULL;
	}
}

bool Client::isCgiTimedOut()
{
	if (cgiHandler == NULL)
		return false;
	
	if (!cgiHandler->isStarted())
		return false;
	
	return (time(NULL) - cgiHandler->getStartTime() > CGI_TIMEOUT);
}

bool Client::getIsCGI() const
{
	return isCGI;
}

void Client::setIsCGI(bool val)
{
	isCGI = val;
}

Server* Client::getServer() const
{
	return currentServer;
}

CGIHandler* Client::getCGIHandler() const
{
	return cgiHandler;
}

Request*		Client::getCurrentRequest()
{
    return currentRequest;
}