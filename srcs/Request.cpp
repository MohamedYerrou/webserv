#include "../includes/Request.hpp"

Request::Request()
    : uploadFile(-1)
{
}

Request::~Request()
{
}

const std::string& Request::getMethod() const
{
    return method;
}

const std::string& Request::getPath() const
{
    return path;
}

const std::string& Request::getProtocol() const
{
    return protocol;
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
    return headers;
}

const std::map<std::string, std::string>& Request::getQueries() const
{
    return queries;
}

size_t  Request::getContentLength() const
{
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length");
    if (it != headers.end())
    {
        int length = stringToInt(it->second);
        if (length >= 0)
            return static_cast<size_t>(length);
    }
    return 0;
}

bool Request::parseMethod(const std::string& method)
{
    // std::cout << "Method: " << method << std::endl;
    if (method.empty())
        return false;
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    return false;
}

std::string Request::decodeUri(const std::string& uri, bool isQuery)
{
    std::string result;
    for (size_t i = 0; i < uri.size(); i++)
    {
        if (isQuery && uri[i] == '+')
        {
            result += ' ';
            continue;
        }
        if (uri[i] == '%' && i + 2  < uri.size())
        {
            if (isxdigit(uri[i + 1]) && isxdigit(uri[i + 2]))
            {
                std::string str = uri.substr(i + 1, 2);
                int number = stringToInt(str, 16);
                char c = static_cast<char>(number);
                result += c;
                i += 2;
                continue ;
            }
        }
        result += uri[i];
    }
    return result;
}

std::string Request::normalizePath(const std::string& path)
{
    std::stringstream stearm(path);
    std::string segment;
    std::vector<std::string> segments;
    std::string result;

    while (std::getline(stearm, segment, '/'))
    {
        if (segment.empty() || segment == ".")
            continue;
        if (segment == "..")
        {
            if (!segments.empty())
                segments.pop_back();
            continue;
        }
        segments.push_back(segment);
    }
    if (segments.empty())
        return "/";
    for (size_t i = 0; i < segments.size(); i++)
    {
        result += "/";
        result += segments.at(i);
    }
    if (path.at(path.length() - 1) == '/')
        result += "/";
    return result;
}

void    Request::parseQuery(const std::string& query)
{
    // std::cout << "Query: " << query << std::endl;
    std::stringstream ss(query);
    std::string str;
    while (std::getline(ss, str, '&'))
    {
        if (!str.empty())
        {
            std::size_t pos = str.find("=");
            if (pos != std::string::npos)
            {
                std::string name = str.substr(0, pos);
                std::string value = str.substr(pos + 1);
                queries[name] = value;
            }
            else
            {
                std::string name = str;
                queries[name] = "";
            }
        }
    }
}

void    Request::splitUri(const std::string& str)
{
    //TODO: parse queryString
    std::size_t pos = str.find("?");
    if (pos != std::string::npos)
    {
        path = decodeUri(str.substr(0, pos), false);
        std::string queryString = decodeUri(str.substr(pos + 1), true);
        parseQuery(queryString);
    }
    else
        path = decodeUri(str, false);
}

std::string Request::removeFragment(const std::string& uri)
{
    std::size_t fragmentPos = uri.find("#");
    if (fragmentPos != std::string::npos)
    {
        std::string str = uri.substr(0, fragmentPos);
        return str;
    }
    return uri;
}

bool Request::parseUri(const std::string& uri)
{
    // std::cout << "Uri: " << uri << std::endl;
    if (uri.empty() || uri[0] != '/')
        return false;
    std::string str = removeFragment(uri);
    splitUri(str);

    path = normalizePath(path);
    return true;
}


std::string trim(std::string str)
{
    std::size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    std::size_t end = str.find_last_not_of(" \t");
    return (str.substr(start, end - start + 1));
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

void    Request::parseLine(const std::string& raw)
{
    // std::cout << raw << std::endl;
    std::size_t pos = raw.find("\r\n");
    if (pos != std::string::npos)
    {
        std::string line = raw.substr(0, pos);
        std::stringstream str(line);
        str >> method >> uri >> protocol;
        if (!parseMethod(method))
            throw std::runtime_error("Bad request: Unsupported method");
        if (!parseUri(uri))
            throw std::runtime_error("Bad request: Invalid uri");
        if (protocol.empty())
            throw std::runtime_error("Bad request: protocol empty");
        // if (protocol != "http/1.0")
        //     throw std::runtime_error("Bad request: Unsupported HTTP version");
    }
    else
        throw std::runtime_error("Bad request");
}

void    Request::parseHeaders(const std::string& raw)
{
    std::size_t pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos)
        throw std::runtime_error("Bad request: Missing end of header");
    pos = raw.find("\r\n");
    pos += 2;
    std::size_t currentPos = raw.find("\r\n", pos);
    while (currentPos != std::string::npos)
    {
        std::string headerLine = raw.substr(pos, currentPos - pos);
        if (headerLine.empty())
            break ;
        std::size_t colonPos = headerLine.find(":");
        if (colonPos != std::string::npos)
        {
            std::string key = trim(headerLine.substr(0, colonPos));
            std::string value = trim(headerLine.substr(colonPos + 1));
            if (!key.empty())
                headers[key] = value;
        }
        else
            throw std::runtime_error("Bad request: Invalid header format");
        pos = currentPos + 2;
        currentPos = raw.find("\r\n", pos);
    }
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

void    Request::generateTmpFile()
{
    std::string tmpFile = "testFile";
    uploadFile = open(tmpFile.c_str(), O_CREAT | O_RDWR, 0600);
    if (uploadFile == -1)
        throw std::runtime_error("Cannot Create tmp file: " + std::string(strerror(errno)));
    std::cout << "Temporary file has been created" << std::endl;
}

void    Request::appendBody(const char* buf, size_t length)
{
    std::cout << "body from request" << std::endl;
    ssize_t count = write(uploadFile, buf, length);
    if (count != (ssize_t)length)
        throw std::runtime_error("Write error: " + std::string(strerror(errno)));
}

void    Request::parseRequest(const std::string& raw)
{
    parseLine(raw);
    parseHeaders(raw);
}

