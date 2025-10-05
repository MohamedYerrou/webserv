#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

class Location
{
private:
    std::string							_path;
    std::string							_root;
    std::vector<std::string>			_index;
    std::string							_upload_store;
    std::vector<std::string>			_methods;
    std::map<std::string, std::string>	_cgi;
    std::map<int, std::string>			_errors;
	std::pair<int, std::string>			_http_redirection;
    bool								_autoindex;
    
public:
    Location();
    ~Location();
    void    set_path(std::string& path);
    void    set_root(std::string& root);
    void    push_index(std::string& index);
    void    set_upload_store(std::string& upload_store);
    void    push_method(std::string& method);
    void    insert_cgi(std::pair<std::string, std::string>  cgi);
    void    insert_error(std::pair<int, std::string>  error);
    void    set_redir(std::pair<int, std::string> redir);
    void    set_autoindex(bool auto_idx);
};

#endif
