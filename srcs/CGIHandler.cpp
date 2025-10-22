#include "../includes/CGIHandler.hpp"
#include "../includes/Client.hpp"
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <vector>
#include <cstring>

CGIHandler::CGIHandler(Client* c)
: client(c), pid(-1), started(false), finished(false)
{
    (void)client;
    in_fd[0] = in_fd[1] = -1;
    out_fd[0] = out_fd[1] = -1;
}

CGIHandler::~CGIHandler()
{
    if (in_fd[0] != -1) { close(in_fd[0]); in_fd[0] = -1; }
    if (in_fd[1] != -1) { close(in_fd[1]); in_fd[1] = -1; }
    if (out_fd[0] != -1) { close(out_fd[0]); out_fd[0] = -1; }
    if (out_fd[1] != -1) { close(out_fd[1]); out_fd[1] = -1; }

    if (pid > 0) {
    int status;
    if (waitpid(pid, &status, WNOHANG) <= 0) { 
        kill(pid, SIGKILL); 
        waitpid(pid, &status, 0);
        }
    }
}


bool CGIHandler::isFinished() const
{
    return finished;
}

std::string CGIHandler::getBuffer() const
{
    return buffer;
}

int CGIHandler::getOutFD() const
{
    return out_fd[0];
}


std::vector<std::string> Client::buildCGIEnv(const std::string& scriptPath)
{
    std::vector<std::string> env;
    
    env.push_back("GATEWAY_INTERFACE=CGI/1.0");
    env.push_back("SERVER_PROTOCOL=" + currentRequest->getProtocol());
    env.push_back("REQUEST_METHOD=" + currentRequest->getMethod());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    // env.push_back("PATH_INFO=" + pathInfo);  // updated

    std::map<std::string, std::string> headers = currentRequest->getHeaders();
    if (headers.find("Host") != headers.end())
        env.push_back("HTTP_HOST=" + headers["Host"]);
    
    if (headers.find("Content-Type") != headers.end())
        env.push_back("CONTENT_TYPE=" + headers["Content-Type"]);

    if (headers.find("Content-Length") != headers.end())
        env.push_back("CONTENT_LENGTH=" + headers["Content-Length"]);

    env.push_back("SERVER_SOFTWARE=server/1.0");
    env.push_back("REDIRECT_STATUS=200");

    return env;
}



void Client::handleCGI()
{
    // Build environment
    std::vector<std::string> vecEnv = buildCGIEnv(newPath);
    std::map<std::string,std::string> envMap;
    for (std::size_t i = 0; i < vecEnv.size(); ++i)
    {
        std::size_t pos = vecEnv[i].find('=');
        if (pos != std::string::npos)
            envMap[vecEnv[i].substr(0, pos)] = vecEnv[i].substr(pos + 1);
    }

    // Start CGI if not already started
    if (!cgiHandler)
    {
        cgiHandler = new CGIHandler(this);
        try
        {
            cgiHandler->startCGI(newPath, envMap);
        }
        catch (const std::exception& e)
        {
            std::cerr << "[CGI] Exception while starting CGI: " << e.what() << std::endl;
            errorResponse(500, e.what());
            delete cgiHandler;
            cgiHandler = NULL;
            return;
        }
    }

    // Read output
    if (cgiHandler)
        cgiHandler->readOutput();

    // Send response if finished
    if (cgiHandler && cgiHandler->isFinished())
    {
        std::string body = cgiHandler->getBuffer();
        std::ostringstream oss;
        oss << body.size();

        std::string res = "HTTP/1.1 200 OK\r\n";
        res += "Content-Type: text/html\r\n";
        res += "Content-Length: " + oss.str() + "\r\n\r\n";
        res += body;

        send(getFD(), res.c_str(), res.size(), 0);
        setSentAll(true);

        delete cgiHandler;
        cgiHandler = NULL;
    }
}




void CGIHandler::startCGI(const std::string& scriptPath, const std::map<std::string,std::string>& env)
{
    
    int err_pipe[2];
    if (pipe(in_fd) == -1 || pipe(out_fd) == -1 || pipe(err_pipe) == -1) {
        int saved = errno;
        if (in_fd[0] != -1) { close(in_fd[0]); in_fd[0] = -1; }
        if (in_fd[1] != -1) { close(in_fd[1]); in_fd[1] = -1; }
        if (out_fd[0] != -1) { close(out_fd[0]); out_fd[0] = -1; }
        if (out_fd[1] != -1) { close(out_fd[1]); out_fd[1] = -1; }
        if (err_pipe[0] != -1) { close(err_pipe[0]); err_pipe[0] = -1; }
        if (err_pipe[1] != -1) { close(err_pipe[1]); err_pipe[1] = -1; }
        std::cerr << "[CGI] pipe() failed: " << strerror(saved) << "\n";
        throw std::runtime_error("pipe failed");
    }
    
    pid = fork();
    if (pid == -1) {
        int saved = errno;
        close(in_fd[0]); close(in_fd[1]); in_fd[0] = in_fd[1] = -1;
        close(out_fd[0]); close(out_fd[1]); out_fd[0] = out_fd[1] = -1;
        close(err_pipe[0]); close(err_pipe[1]);
        std::cerr << "[CGI] fork() failed: " << strerror(saved) << "\n";
        throw std::runtime_error("fork failed");
    }
    std::cerr << "[CGI] startCGI: script=" << scriptPath
    << " pid=" << pid << " outfd=" << out_fd[0] << "\n";
    
    if (pid == 0)
    {
        close(in_fd[1]);
        close(out_fd[0]);
        close(err_pipe[0]);

        dup2(in_fd[0], STDIN_FILENO);
        dup2(out_fd[1], STDOUT_FILENO);
        dup2(out_fd[1], STDERR_FILENO);
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
        
        if (access(scriptPath.c_str(), X_OK) != 0) {
            write(err_pipe[1], "E", 1);
            for (size_t i = 0; i < envp.size(); ++i) delete[] envp[i];
            exit(1);
        }

        char* argv[] = { const_cast<char*>(scriptPath.c_str()), NULL };
        execve(scriptPath.c_str(), argv, envp.empty() ? NULL : &envp[0]);

        write(err_pipe[1], "E", 1);
        for (size_t i = 0; i < envp.size(); ++i) delete[] envp[i];
        exit(1);
    }
    else
    {
        close(in_fd[0]);
        close(out_fd[1]);
        close(err_pipe[1]);

        char err = 0;
        ssize_t n = read(err_pipe[0], &err, 1);
        close(err_pipe[0]);
        if (n == 1) {
            int status;
            waitpid(pid, &status, 0);
            pid = -1;
            if (in_fd[1] != -1) { close(in_fd[1]); in_fd[1] = -1; }
            if (out_fd[0] != -1) { close(out_fd[0]); out_fd[0] = -1; }
            std::cerr << "[CGI] execve() failed in child for " << scriptPath << "\n";
            throw std::runtime_error(std::string("execve failed for ") + scriptPath);
        }

        fcntl(out_fd[0], F_SETFL, O_NONBLOCK);
        started = true;
    }
}



void CGIHandler::readOutput()
{
    if (!started || finished || out_fd[0] == -1) return;

    char buf[1024];
    while (true) {
        ssize_t n = read(out_fd[0], buf, sizeof(buf));
        if (n > 0) {
            buffer.append(buf, n);
            continue;
        }
        else if (n == 0) {
            finished = true;
            if (pid > 0) { waitpid(pid, NULL, WNOHANG); pid = -1;}
            close(out_fd[0]); out_fd[0] = -1;
            break;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                finished = true;
                if (pid > 0) { waitpid(pid, NULL, WNOHANG); pid = -1; }
                close(out_fd[0]); out_fd[0] = -1;
                break;
            }
        }
    }
}

void    Client::checkCGIValid()
{
    
    location = findMathLocation(currentRequest->getPath());

    newPath = joinPath();
    std::cout << "newPath==============================>" << newPath << std::endl;
    const std::vector<std::string>& indexMethod = location->getMethod();
    bool isMethod = false;
    for (size_t i = 0; i < indexMethod.size(); ++i)
        {
            if (indexMethod[i] == "GET" || indexMethod[i] == "POST")
            {
                isMethod = true;
                break;
            }
        }    
    if (!location->getCgi().empty() && (currentRequest->getMethod() == "GET" || currentRequest->getMethod() == "POST")
        && (isMethod))
    {
        std::size_t dotPos = newPath.find_last_of('.');
        if (isDir(newPath))
        {
            //TODO loop indexes
            const std::vector<std::string>& indexFiles = location->getIndex();
            if (!indexFiles.empty())
            {
                bool indexFound = false;

                // Try to serve an index file (like index.html or index.py)
                for (size_t i = 0; i < indexFiles.size(); ++i)
                {
                    newPath = newPath + "/" + indexFiles[i];
                    if (isFile(newPath))
                    {
                        setIsCGI(true);
                        indexFound = 1;
                        std::cout << "hello [INDEX] Found index file: " << newPath << std::endl;
                        break;
                    }    
                }
                if (indexFound == 1)
                {
                    std::cout << "indexFound==============" << indexFound << std::endl;
                    setIsCGI(true);
                }
                else if (!indexFound)
                {
                    if (location->getAutoIndex())
                    {
                        std::cout << "MUST HANDLE LISTING DIRECTORY" << std::endl;
                        std::cout << "newPath ==============>" << newPath <<std::endl;
                        listingDirectory(newPath);
                    }
                    else
                    {
                        std::cout << "[CGI] No index file found for directory: " << newPath << std::endl;
                        errorResponse(403, "Forbidden");
                    }
                }
            }
            else if (location->getAutoIndex())
            {
                std::cout << "MUST HANDLE LISTING DIRECTORY" << std::endl;
                std::cout << "newPath ==============>" << newPath <<std::endl;
                listingDirectory(newPath);
            }
            else
            {
                // setIsCGI(false);
                std::cout << "[CGI] No index file found for directory: " << newPath << std::endl;
                errorResponse(403, "Forbidden");
            }

        }
        else if (isFile(newPath))
        {
            if (dotPos == std::string::npos)
            {
                // setIsCGI(false);
                std::cerr << "[CGI] No file extension found in path: " << newPath << std::endl;
                errorResponse(400, "Invalid CGI path: missing extension");
            }
            std::string ext = newPath.substr(dotPos);
            std::map<std::string, std::string>::const_iterator it = location->getCgi().find(ext);

            bool isCgiFile = (it != location->getCgi().end());

            if (isCgiFile)
            {
                setIsCGI(true);
                std::cout << "[CGI] Interpreter found for " << ext
                        << ": " << it->second << std::endl;
            }
            else
            {
                setIsCGI(false);
                std::cout << "[STATIC] Serving static file: " << newPath << std::endl;
            }
        }
    }
    else
        errorResponse(400, "Method not allowed");
}
