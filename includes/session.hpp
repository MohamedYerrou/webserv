#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>

class session
{
private:
public:
    std::string							id;
    time_t								last_access;
    std::map<std::string, std::string>	data;

    session();
    ~session();
};


std::string generateSessionId();

#endif