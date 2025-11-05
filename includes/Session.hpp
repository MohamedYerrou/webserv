#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>

class Session
{
private:
public:
    std::string							id;
    time_t								last_access;
    std::map<std::string, std::string>	data;

    Session();
    ~Session();
};


std::string generateSessionId();

#endif