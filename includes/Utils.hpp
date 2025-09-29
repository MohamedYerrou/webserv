#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

class MyException: public std::exception
{
    private:
        std::string msg;
    public:
        MyException(std::string msg): msg(msg){};
        const char* what() const throw() {return msg.c_str();};
        virtual ~MyException() _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {};
};

void	run_server(int epfd, std::vector<int>& fd_vect);
bool    listening_fd(std::vector<int>& vect, int fd);
void	throw_exception(std::string function, std::string err);
void	setNonBlocking(int fd);

#endif