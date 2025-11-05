#include "../includes/Location.hpp"
#include "../includes/Location.hpp"

Location::Location(): _autoindex(false), isRedirection(false)
{
    _autoindex = false;
}

Location::~Location()
{
}

void    Location::set_path(std::string& path)
{
    _path = path;
}

void    Location::set_root(std::string& root)
{
    _root = root;
}

void    Location::set_upload_store(std::string& upload_store)
{
    _upload_store = upload_store;
}

void    Location::push_method(std::string& method)
{
    _methods.push_back(method);
}


void    Location::push_index(std::vector<std::string>& tokens, size_t& i)
{
    while (tokens[++i] != ";")
        _index.push_back(tokens[i]);
        // currLocation.push_index(tokens[i]);
}

void    Location::insert_cgi(std::vector<std::string>& tokens, size_t& i)
{
    std::string	ext = tokens[++i];
    std::string path = tokens[++i];
    _cgi.insert(make_pair(ext, path));
}
void    Location::insert_error(std::vector<std::string>& tokens, size_t& i)
{
    int			code;
    std::string	path;

    code = atoi(tokens[++i].c_str());
    path = tokens[++i];
    _errors.insert(make_pair(code, path));
}

void    Location::set_redir(std::vector<std::string>& tokens, size_t& i)
{
    int			status;
    std::string	path;
    
    status = atoi(tokens[++i].c_str());
    path = tokens[++i];
    // currLocation.set_redir(make_pair(status, path));
    _http_redirection = make_pair(status, path);
    isRedirection = true;
    // this->set_isRedirection(true);


}

void    Location::set_autoIndex(std::vector<std::string>& tokens, size_t& i)
{
    if (tokens[++i] == "on")
        _autoindex = true;
}

//GETTERS

const std::string& Location::getPATH() const
{
    return _path;
}

const std::string& Location::getRoot() const
{
    return _root;
}

const std::vector<std::string>& Location::getIndex() const
{
    return _index;
}

const std::string& Location::getUploadStore() const
{
    return _upload_store;
}

const std::vector<std::string>& Location::getMethod() const
{
    return _methods;
}

bool Location::getAutoIndex() const
{
    return _autoindex;
}

bool Location::hasRedir() const
{
    return isRedirection;
}

const std::pair<int, std::string> Location::getRedirection() const
{
    return _http_redirection;
}

const std::map<int, std::string>& Location::getErrors() const
{
    return _errors;
}

void    Location::printLocation() const
{
    std::cout << "Total location" << std::endl;
    std::cout << "Path: " << _path << std::endl;
    // std::cout << "Root: " << _root << std::endl;
    // std::cout << "Index: " << _index << std::endl;
    // std::cout << "AutoIndex: " << _autoindex << std::endl;
    // std::cout << "Methods: ";
    // for (size_t i = 0; i < _methods.size(); i++)
    //     std::cout << _methods[i] << ' ';
    // std::cout << std::endl;

    //added by Mohamed
    for (std::map<std::string, std::string>::const_iterator it = _cgi.begin(); it != _cgi.end(); ++it)
	std::cout << "CGI *********************************** registered: " << it->first << " -> " << it->second << std::endl;
}
