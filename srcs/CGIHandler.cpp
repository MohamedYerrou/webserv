#include "../includes/CGIHandler.hpp"
#include "../includes/Client.hpp"
#include "../includes/Server.hpp"
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <vector>
#include <cstring>
#include <sstream>

CGIHandler::CGIHandler(Client* c)
: client(c), pid(-1), started(false), finished(false), bytes_written(0), _error(false)
{
    (void)client;
    in_fd[0] = in_fd[1] = -1;
    out_fd[0] = out_fd[1] = -1;
}

CGIHandler::~CGIHandler()
{
    if (in_fd[0] != -1)
    { 
        close(in_fd[0]);
        in_fd[0] = -1; 
    }
    if (in_fd[1] != -1)
    { 
        close(in_fd[1]);
        in_fd[1] = -1; 
    }
    if (out_fd[0] != -1)
    { 
        close(out_fd[0]);
        out_fd[0] = -1; 
    }
    if (out_fd[1] != -1)
    { 
        close(out_fd[1]);
        out_fd[1] = -1; 
    }

    if (pid > 0) {
        int status;
        if (waitpid(pid, &status, WNOHANG) <= 0)
        { 
            kill(pid, SIGKILL); 
            waitpid(pid, &status, 0);
        }
    }
}

void CGIHandler::setError(bool error)
{
    finished = true;
    _error = error;
}

void CGIHandler::setComplete(bool complete)
{
    finished = complete;
    if (complete)
        _error = false;
}

bool CGIHandler::is_Error() const
{
    return _error;
}

size_t CGIHandler::getBytesWritten() const
{
    return bytes_written;
}

void CGIHandler::addBytesWritten(size_t bytes)
{
    bytes_written += bytes;
}

void CGIHandler::appendResponse(const char* buf, ssize_t length)
{
    if (length > 0)
        buffer.append(buf, length);
}

int CGIHandler::getPid() const
{
    return pid;
}

bool CGIHandler::isStarted() const
{
    return started;
}

bool CGIHandler::isComplete() const
{
    return finished;
}

int CGIHandler::getStdinFd() const
{
    return in_fd[1];
}

int CGIHandler::getStdoutFd() const
{
    return out_fd[0];
}

bool CGIHandler::isFinished() const
{
    return finished;
}

std::string CGIHandler::getBuffer() const
{
    return buffer;
}

std::vector<std::string> Client::buildCGIEnv(const std::string& scriptPath)
{
    std::vector<std::string> env;
    
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=" + currentRequest->getProtocol());
    env.push_back("REQUEST_METHOD=" + currentRequest->getMethod());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    
    std::string uri = currentRequest->getPath();
    size_t qPos = uri.find('?');
    if (qPos != std::string::npos)
        env.push_back("QUERY_STRING=" + uri.substr(qPos + 1));
    else
        env.push_back("QUERY_STRING=");
    
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    std::map<std::string, std::string> headers = currentRequest->getHeaders();
    (void)scriptPath;
    
    // if (headers.find("Host") != headers.end())
    // {
    //     std::string host = headers["Host"];
    //     size_t colonPos = host.find(':');
    //     if (colonPos != std::string::npos)
    //     {
    //         env.push_back("SERVER_NAME=" + host.substr(0, colonPos));
    //         env.push_back("SERVER_PORT=" + host.substr(colonPos + 1));
    //     }
    //     else
    //     {
    //         env.push_back("SERVER_NAME=" + host);
    //         env.push_back("SERVER_PORT=80");
    //     }
    // }
    // else
    // {
    //     env.push_back("SERVER_NAME=localhost");
    //     env.push_back("SERVER_PORT=80");
    // }
    
    if (headers.find("Content-Type") != headers.end())
        env.push_back("CONTENT_TYPE=" + headers["Content-Type"]);
    
    if (headers.find("Content-Length") != headers.end())
        env.push_back("CONTENT_LENGTH=" + headers["Content-Length"]);
    
    env.push_back("REDIRECT_STATUS=200");
    for (std::map<std::string, std::string>::iterator it = headers.begin() ; it != headers.end() ; ++it)
    {
        env.push_back("HTTP_" + it->first + '=' + it->second);
    }

    return env;
}

void Client::handleCGI()
{
    if (!cgiHandler)
    {
        std::vector<std::string> vecEnv = buildCGIEnv(newPath);
        std::map<std::string,std::string> envMap;
        for (std::size_t i = 0; i < vecEnv.size(); ++i)
        {
            std::size_t pos = vecEnv[i].find('=');
            if (pos != std::string::npos)
                envMap[vecEnv[i].substr(0, pos)] = vecEnv[i].substr(pos + 1);
        }

        cgiHandler = new CGIHandler(this);
        
        try
        {
            cgiHandler->startCGI(newPath, envMap);
            return;
        }
        catch (const std::exception& e)
        {
            errorResponse(500, e.what());
            cgiHandler->setError(true);
            delete cgiHandler;
            cgiHandler = NULL;
            return;
        }
    }
}

void CGIHandler::startCGI(const std::string& scriptPath, const std::map<std::string,std::string>& env)
{
    int err_pipe[2];
    if (pipe(in_fd) == -1 || pipe(out_fd) == -1 || pipe(err_pipe) == -1) {
        if (in_fd[0] != -1) { close(in_fd[0]); in_fd[0] = -1; }
        if (in_fd[1] != -1) { close(in_fd[1]); in_fd[1] = -1; }
        if (out_fd[0] != -1) { close(out_fd[0]); out_fd[0] = -1; }
        if (out_fd[1] != -1) { close(out_fd[1]); out_fd[1] = -1; }
        throw std::runtime_error("pipe failed");
    }
    
    pid = fork();
    
    if (pid == -1) {
        close(in_fd[0]); close(in_fd[1]); in_fd[0] = in_fd[1] = -1;
        close(out_fd[0]); close(out_fd[1]); out_fd[0] = out_fd[1] = -1;
        throw std::runtime_error("fork failed");
    }
    
    if (pid == 0)
    {
        close(in_fd[1]);
        close(out_fd[0]);

        dup2(in_fd[0], STDIN_FILENO);
        dup2(out_fd[1], STDOUT_FILENO);

        close(in_fd[0]);
        close(out_fd[1]);
        
        std::vector<char*> envp;
        for (std::map<std::string,std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
        {
            std::string s = it->first + "=" + it->second;
            char* cstr = new char[s.size() + 1];
            strcpy(cstr, s.c_str());
            envp.push_back(cstr);
        }
        envp.push_back(NULL);
        
        if (access(scriptPath.c_str(), F_OK) != 0) {
            write(out_fd[1], "E", 1);
            setError(true);
            for (size_t i = 0; i < envp.size(); ++i) delete[] envp[i];
            exit(1);
        }
        std::map<std::string, std::string> cgi = (client->findMathLocation(client->getcurrentRequest()->getPath())->getCgi());
        
        // Find the appropriate interpreter based on script extension
        std::string interpreter;
        std::size_t dotPos = scriptPath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            std::string ext = scriptPath.substr(dotPos);
            std::map<std::string, std::string>::const_iterator it = cgi.find(ext);
            if (it != cgi.end())
            interpreter = it->second;
        }
        
        if (!interpreter.empty())
        {
            // Execute with the found interpreter
            char* argv[] = { const_cast<char*>(interpreter.c_str()), const_cast<char*>(scriptPath.c_str()), NULL };
            execve(interpreter.c_str(), argv, envp.empty() ? NULL : &envp[0]);
        }
        // else
        // {
        //     char* argv[] = { const_cast<char*>(scriptPath.c_str()), NULL };
        //     execve(scriptPath.c_str(), argv, envp.empty() ? NULL : &envp[0]);
        // }
        write(out_fd[1], "E", 1);
        for (size_t i = 0; i < envp.size(); ++i) delete[] envp[i];
        exit(1);
    }
    else
    {
        close(in_fd[0]);
        close(out_fd[1]);

        fcntl(out_fd[0], F_SETFL, O_NONBLOCK);
        fcntl(in_fd[1], F_SETFL, O_NONBLOCK);
        
        started = true;
    }
}


bool Client::validateCGIScript(const std::string& scriptPath)
{
    std::ifstream file(scriptPath.c_str());
    if (!file.is_open())
        return false;

    std::string line;
    bool hasShebang = false;
    bool isEmpty = true;

    // Check for shebang line
    if (std::getline(file, line))
    {
        isEmpty = false;
        if (line.length() >= 2 && line[0] == '#' && line[1] == '!')
            hasShebang = true;
    }

    file.close();

    // Script must not be empty and should have a valid shebang
    return !isEmpty && hasShebang;
}


void Client::checkCGIValid()
{
    // 1. Check Request Method Authorization (Nginx uses 405 Method Not Allowed)
    const std::vector<std::string>& allowedMethods = location->getMethod();
    const std::string& requestMethod = currentRequest->getMethod();
    bool methodAllowed = false;

    for (size_t i = 0; i < allowedMethods.size(); ++i)
    {
        if (allowedMethods[i] == requestMethod)
        {
            methodAllowed = true;
            break;
        }
    }

    if (!methodAllowed)
    {
        std::cerr << "[CGI Check] Method " << requestMethod << " not allowed for path: " << currentRequest->getPath() << std::endl;
        return errorResponse(405, "METHOD NOT ALLOWED");
    }

    // 2. Determine if CGI processing is configured at all for this location
    const std::map<std::string, std::string>& cgiMap = location->getCgi();
    bool cgiConfigured = !cgiMap.empty();

    // 3. Handle Path Resolution: Directory vs. File

    if (isDir(newPath))
    {
        // Path points to a directory. Look for index files (Nginx-like behavior).
        const std::vector<std::string>& indexFiles = location->getIndex();
        bool indexFound = false;
        std::string originalPath = newPath;

        if (!indexFiles.empty())
        {
            // Ensure directory path ends with a slash for proper index file joining
            if (originalPath.length() > 0 && originalPath[originalPath.length() - 1] != '/')
                originalPath += "/";

            for (size_t i = 0; i < indexFiles.size(); ++i)
            {
                std::string indexPath = originalPath + indexFiles[i];

                if (isFile(indexPath))
                {
                    newPath = indexPath; // Update newPath to the actual index file path
                    indexFound = true;
                    break;
                }
            }
        }

        if (!indexFound)
        {
            if (location->getAutoIndex())
            {
                std::cout << "[CGI Check] Listing directory (Autoindex ON): " << newPath << std::endl;
                return listingDirectory(newPath);
            }
            else
            {
                std::cerr << "[CGI Check] Index file not found and autoindex is OFF: " << newPath << std::endl;
                // Nginx returns 403 Forbidden when an index is missing and autoindex is off.
                return errorResponse(403, "FORBIDDEN");
            }
        }
    }
    else if (!isFile(newPath))
    {
        // Path is neither a directory nor a file.
        std::cerr << "[CGI Check] Resource not found: " << newPath << std::endl;
        return errorResponse(404, "NOT FOUND");
    }

    // 4. File Handling (Reached here if newPath is confirmed to be a regular file)

    if (cgiConfigured)
    {
        std::size_t dotPos = newPath.find_last_of('.');

        if (dotPos != std::string::npos)
        {
            std::string ext = newPath.substr(dotPos);
            std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
            if (it != cgiMap.end())
            {
                setIsCGI(true);
                std::cout << "[CGI Check] Serving CGI script with interpreter: " << it->second << " for path: " << newPath << std::endl;
                return;
            }
        }
    }

    // 5. Default to Static File (Final Fallback)
    setIsCGI(false);
    if (currentRequest->getMethod() == "GET")
        handleGET();
    // else if (currentRequest->getMethod() == "POST")
    //     handlePOST();
    std::cout << "[CGI Check] Serving static file: " << newPath << std::endl;
}