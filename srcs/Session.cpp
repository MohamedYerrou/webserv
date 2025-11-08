#include <string>
#include <time.h>
#include <iostream>
#include "../includes/Session.hpp"
#include <stdlib.h>



Session::Session(): last_access(time(NULL))
{
}

Session::~Session()
{
}


std::string generateSessionId()
{
    char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxys";
    std::string id;
    size_t size = sizeof(alphanum) - 1;

    for (size_t i = 0; i < 16; i++)
    {
        id += alphanum[rand() % size];
    }
    return id;
}
