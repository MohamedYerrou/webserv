#include "../includes/Location.hpp"

Location::Location(): _autoindex(false)
{
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

void    Location::set_index(std::string& index)
{
    _index = index;
}

void    Location::set_upload_store(std::string& upload_store)
{
    _upload_store = upload_store;
}

void    Location::push_method(std::string& method)
{
    _methods.push_back(method);
}

void    Location::insert_cgi(std::pair<std::string, std::string>  cgi)
{
    _cgi.insert(cgi);
}

void    Location::insert_error(std::pair<int, std::string> error)
{
    _errors.insert(error);
}

void    Location::set_redir(std::pair<int, std::string> redir)
{
    _http_redirection = redir;
}