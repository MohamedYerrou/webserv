#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdlib.h>

class Location
{
private:
    std::string							_path;
    std::string							_root;
    std::vector<std::string>            _index;
    std::string							_upload_store;
    std::vector<std::string>			_methods;
    std::map<std::string, std::string>	_cgi;
    std::map<int, std::string>			_errors;
	std::pair<int, std::string>			_http_redirection;
    bool								_autoindex;
    bool                                isRedirection;
    
public:
	Location();
	~Location();
    void	set_path(std::string& path);
	void	set_root(std::string& root);
	void	set_upload_store(std::string& upload_store);
	void	push_method(std::string& method);
	void    push_index(std::vector<std::string>& tokens, size_t& i);
	void	insert_cgi(std::vector<std::string>& tokens, size_t& i);
	void	insert_error(std::vector<std::string>& tokens, size_t& i);
	void	set_redir(std::vector<std::string>& tokens, size_t& i);
	void	set_autoIndex(std::vector<std::string>& tokens, size_t& i);

    //GETTERS
	const std::string&							getPATH() const;
	const std::string&							getRoot() const;
	const std::string&							getUploadStore() const;
	const std::pair<int, std::string>			getRedirection() const;
	const std::vector<std::string>&				getIndex() const;
	const std::vector<std::string>&				getMethod() const;
	const std::map<int, std::string>&			getErrors() const;
	bool										getAutoIndex() const;
	bool										hasRedir() const;
	void										printLocation() const;
	const std::map<std::string, std::string>&	getCgi() const;

};

#endif
