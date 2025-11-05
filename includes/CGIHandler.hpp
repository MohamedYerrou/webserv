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

    void startCGI(const std::string& scriptPath, const std::map<std::string,std::string>& env);
    bool isFinished() const;
    std::string getBuffer() const;
    int getOutFD() const;
    time_t getStartTime() const { return _startTime; }
    bool isStarted() const { return started; }
    
    // New methods for pipe-based CGI handling
    pid_t getPid() const { return pid; }
    int getStdinFd() const { return in_fd[1]; }
    int getStdoutFd() const { return out_fd[0]; }
    void appendResponse(const char* buf, size_t len) { buffer.append(buf, len); }
    void setComplete(bool val) { is_complete = val; }
    void setError(bool val) { is_error = val; }
    void setErrorCode(int code) { error_code = code; is_error = true; }
    bool isComplete() const { return is_complete; }
    bool hasError() const { return is_error; }
    int getErrorCode() const { return error_code; }
};

#endif
