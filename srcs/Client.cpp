#include "../includes/Client.hpp"

Client::Client(int fd, Server* srv)
    : fd(fd), endHeaders(false), reqComplete(false), hasBody(false), requestError(false), currentRequest(NULL), currentServer(srv), location(NULL)
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

void Client::handleCGI(std::string totalPath)
{
    std::cout << "[CGI] Entered handleCGI() for path: " << totalPath << std::endl;

    std::size_t dotPos = totalPath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        std::cerr << "[CGI] No file extension found in path: " << totalPath << std::endl;
        errorResponse(400, "Invalid CGI path: missing extension");
        return;
    }

    std::string ext = totalPath.substr(dotPos);
    std::cout << "[CGI] Detected file extension: " << ext << std::endl;

    const std::map<std::string, std::string>& cgiMap = location->getCgi();

    std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
    if (it == cgiMap.end())
    {
        std::cerr << "[CGI] No interpreter configured for extension: " << ext << std::endl;
        errorResponse(500, "Unsupported CGI extension");
        return;
    }

    const std::string& interpreter = it->second;
    std::cout << "[CGI] Using interpreter: " << interpreter << std::endl;

    try
    {
        std::string output = executeCGI(totalPath, interpreter);
        std::cout << "[CGI] CGI executed successfully, output length: " 
                  << output.size() << " bytes" << std::endl;

        handleCGIResponse(output);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[CGI] Exception while executing CGI: " << e.what() << std::endl;
        errorResponse(500, e.what());
    }
}

void Client::handleGET()
{
	location = findMathLocation(currentRequest->getPath());
	if (!location)
	{
		errorResponse(404, "Not Found");
		return;
	}
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
    if (totalPath.find("cgi-bin/") != std::string::npos)
    {
        handleCGI(totalPath);
        return;
    }
	if (isDir(totalPath))
	{
		handleDirectory(totalPath);
		return;
	}
	if (isFile(totalPath))
	{
		handleFile(totalPath);
		return;
	}
	errorResponse(404, "Not Found");
}


// void    Client::handleGET()
// {
//     location = findMathLocation(currentRequest->getPath());
//     if (location)
//     {
//         if (location->hasRedir())
//         {
//             handleRedirection();
//             return;
//         }
//         if (location->getRoot().empty())
//         {
//             errorResponse(500, "Missing root directive");
//             return;
//         }
//         std::string totalPath = joinPath();
//         if (isDir(totalPath))
//         {
//             handleDirectory(totalPath);
//         }
//         if (isFile(totalPath))
//         {
//             handleFile(totalPath);
//         }
//     }
// }

// void    Client::handlePost()
// {
//     std::string		Content_type = (currentRequest->getHeaders())["Content-Type"];
//     std::fstream	body("testfile", std::ios::in);
//     std::string		line;

//     if (Content_type == "text/plain")
//         return ;
//     else if (Content_type == "multipart/form-data")
//         return ;
//     else if (Content_type == "application/x-www-form-urlencoded")
//         return ;
// }

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
        {
            std::cout << "LLLLLLLLLLLLLLLLLLLLLLLLLLLLL" << std::endl;
            errorResponse(505, e.what());
            return;
        }
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
                std::string target_path = constructFilePath(currentRequest->getPath());
                currentRequest->generateTmpFile(target_path);
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


//ADDED BY MOHAMED

std::vector<std::string> Client::buildCGIEnv(const std::string& scriptPath)
{
	std::vector<std::string> env;

	env.push_back("GATEWAY_INTERFACE=CGI/1.0");
	env.push_back("SERVER_PROTOCOL=" + currentRequest->getProtocol());
	env.push_back("REQUEST_METHOD=" + currentRequest->getMethod());
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("PATH_INFO=" + currentRequest->getUri());

	// host and port
	std::map<std::string, std::string> headers = currentRequest->getHeaders();
	if (headers.find("Host") != headers.end())
		env.push_back("HTTP_HOST=" + headers["Host"]);

	if (headers.find("Content-Type") != headers.end())
		env.push_back("CONTENT_TYPE=" + headers["Content-Type"]);

	if (headers.find("Content-Length") != headers.end())
		env.push_back("CONTENT_LENGTH=" + headers["Content-Length"]);

	env.push_back("SERVER_SOFTWARE=webserv/1.0");
	env.push_back("REDIRECT_STATUS=200");

	return env;
}


std::string Client::executeCGI(const std::string& scriptPath, const std::string& interpreter)
{
	int inPipe[2];
	int outPipe[2];

	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
		throw MyException("pipe() failed");

	pid_t pid = fork();
	if (pid == -1)
		throw MyException("fork() failed");

	if (pid == 0)
	{
		// Child
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		std::vector<std::string> envVec = buildCGIEnv(scriptPath);
		char **envp = new char*[envVec.size() + 1];
		for (size_t i = 0; i < envVec.size(); ++i)
			envp[i] = strdup(envVec[i].c_str());
		envp[envVec.size()] = NULL;

		std::string cmd = interpreter.empty() ? scriptPath : interpreter;
		char *argv[] = { (char *)cmd.c_str(), (char *)scriptPath.c_str(), NULL };

		execve(cmd.c_str(), argv, envp);

		// If execve fails
		for (size_t i = 0; i < envVec.size(); ++i)
			free(envp[i]);
		delete [] envp;
		perror("execve");
		exit(1);
	}

	// Parent
	close(inPipe[0]);
	close(outPipe[1]);

	// send body if exists
	std::string body = "";
	if (currentRequest->getContentLength() > 0)
	{
		// you could read from tmp upload file instead
		// depending on how you store request bodies
	}
	write(inPipe[1], body.c_str(), body.size());
	close(inPipe[1]);

	// read CGI output
	std::string output;
	char buffer[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, bytesRead);

	close(outPipe[0]);
	waitpid(pid, NULL, 0);
	return output;
}


void Client::handleCGIResponse(const std::string& cgiOutput)
{
	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		errorResponse(500, "Invalid CGI output");
		return;
	}

	std::string headerPart = cgiOutput.substr(0, headerEnd);
	std::string bodyPart = cgiOutput.substr(headerEnd + 4);

	std::istringstream headerStream(headerPart);
	std::string line;
	while (std::getline(headerStream, line))
	{
		if (line.find(':') != std::string::npos)
		{
			size_t pos = line.find(':');
			std::string key = trim(line.substr(0, pos));
			std::string value = trim(line.substr(pos + 1));
			currentResponse.setHeaders(key, value);
		}
	}

	currentResponse.setProtocol("HTTP/1.0");
	currentResponse.setStatus(200, "OK");
	currentResponse.setBody(bodyPart);
}

