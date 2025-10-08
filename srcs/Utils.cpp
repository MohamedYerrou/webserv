#include "../includes/Utils.hpp"
#include "../includes/Request.hpp"

std::string currentDate()
{
    time_t now = time(NULL);
    struct tm* timeInfo = gmtime(&now);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
    return std::string(buffer);
}

std::string intTostring(int value)
{
    std::string result;
    std::stringstream ss;
    ss << value;
    ss >> result;
    return result;
}

int stringToInt(const std::string& str, int base)
{
    std::stringstream ss(str);
    int result;
    if (base == 16)
        ss >> std::hex >> result;
    else
        ss >> result;
    return result;
}

std::string trim(std::string str)
{
    std::size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    std::size_t end = str.find_last_not_of(" \t");
    return (str.substr(start, end - start + 1));
}

bool    isFile(const std::string& path)
{
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1)
        return false;
    return S_ISREG(fileStat.st_mode);
}

bool    isDir(const std::string& path)
{
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1)
        return false;
    return S_ISDIR(fileStat.st_mode);
}

std::string getStatusText(int code)
{
    std::string text;
    switch (code)
    {
        case 200:
            text = "OK";
            break;
        case 201:
            text = "Created";
            break;
        case 204:
            text = "No Content";
            break;
        case 301:
            text = "Moved Permanently";
            break;
        case 400:
            text = "Bad Request";
            break;
        case 403:
            text = "Forbidden";
            break;
        case 404:
            text = "Not Found";
            break;
        case 405:
            text = "Method Not Allowed";
            break;
        case 408:
            text = "Request Timeout";
            break;
        case 411:
            text = "Length Required";
            break;
        case 500:
            text = "Internal Server Error";
            break;
        case 505:
            text = "HTTP Version Not Supported";
    }
    return text;
}

void    parsedRequest(Request req)
{
    std::cout << "========================================" << std::endl;
    std::cout << "=============Parsed Request=============" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "Path: " << req.getPath() << std::endl;
    std::cout << "Protocol: " << req.getProtocol() << std::endl;
    std::cout << std::endl;

    std::cout << "================Headers================" << std::endl;
    std::map<std::string, std::string> headers = req.getHeaders();
    std::map<std::string, std::string>::iterator it = headers.begin();
    for (; it != headers.end(); it++)
    {
        std::cout << it->first << "=  " << it->second << std::endl;
    }
}