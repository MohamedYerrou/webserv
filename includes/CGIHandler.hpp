#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

class Client;

class CGIHandler
{
private:
    Client* client;
    int in_fd[2];
    int out_fd[2];
    pid_t pid;
    bool started;
    bool finished;
    std::string buffer;
    time_t _startTime;
    bool is_complete;
    bool is_error;
    int error_code;

public:
    CGIHandler(Client* c);
    ~CGIHandler();

    void                startCGI(const std::string& scriptPath, const std::map<std::string,std::string>& env);
    bool                isFinished() const;
    std::string         getBuffer() const;
    int                 getOutFD() const;
    time_t              getStartTime() const;
    bool                isStarted() const;
    
    // New methods for pipe-based CGI handling
    pid_t               getPid() const;
    int                 getStdinFd() const;
    int                 getStdoutFd() const;
    void                appendResponse(const char* buf, size_t len);
    void                setComplete(bool val);
    void                setError(bool val);
    void                setErrorCode(int code);
    bool                isComplete() const;
    bool                hasError() const;
    int                 getErrorCode() const;
    void                setupPipes(int err_pipe[2]);
    void                executeChildProcess(const std::string& scriptPath, const std::map<std::string, std::string>& env, int err_pipe[2]);
    void                handleParentProcess(int err_pipe[2], const std::string& scriptPath);
    std::vector<char*>  buildEnvp(const std::map<std::string, std::string>& env);
    void                executeScript(const std::string& scriptPath, std::vector<char*>& envp);
};

#endif
