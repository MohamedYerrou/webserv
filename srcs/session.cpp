#include <string>
#include <time.h>
#include <iostream>
#include "../includes/session.hpp"
#include <stdlib.h>



session::session(): last_access(time(NULL))
{
}

session::~session()
{
}


std::string generateSessionId()
{
    char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxys";
    std::string id;

    for (size_t i = 0; i < 16; i++)
    {
        id += alphanum[rand() % sizeof(alphanum) - 1];
    }

    return id;
}

// int main()
// {
//     char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxys";
//     srand(time(NULL));
//     std::cout << sizeof(alphanum) << std::endl;
//     std::cout << generateSessionId() << std::endl;
//     return 0;
// }