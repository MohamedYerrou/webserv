#ifndef CONFIGSTRUCTS_HPP
#define CONFIGSTRUCTS_HPP


#include <string>
#include <vector>
#include <map>

typedef struct Location
{
    std::string							path;
    std::string							root;
    std::string							index;
    std::string							upload_store;
    std::vector<std::string>			methods;
    bool								autoindex;
    std::map<std::string, std::string>	cgi;
	std::pair<int, std::string>			http_redirection;

	Location(): autoindex(false) {}

    // void print() const {
    //     std::cout << "    Location path: " << path << "\n";
	// 	std::cout << "      Root: " << root << "\n";
    //     std::cout << "      Methods: ";
    //     for (size_t i = 0; i < methods.size(); i++)
    //         std::cout << methods[i] << " ";
	// 	std::cout << "\n";
	// 	for (auto it = cgi.begin(); it != cgi.end(); it++)
    //         std::cout << "      cgi " << it->first << " " << it->second << "\n";
	// 	std::cout << "      Return: " << http_redirection.first << " " << http_redirection.second << "\n";
    // }
}				Location;

typedef struct ServerConfig
{
    std::vector<std::pair<std::string, int> >	listens;
    std::map<int, std::string>					errors;
    std::string									root;
    std::string									index;
    bool										autoindex;
    size_t										client_max_body_size;
	std::vector<Location>					locations;

	// ServerConfig(): autoindex(false), client_max_body_size(1024) {}

    // void print() const {
    //     std::cout << "Server:\n";
    //     for (size_t i = 0; i < listens.size(); i++)
    //         std::cout << "  Listen: " << listens[i].first << ":" << listens[i].second << "\n";
    //     std::cout << "  Root: " << root << "\n";
    //     std::cout << "  Index: " << index << "\n";
    //     for (auto it = errors.begin(); it != errors.end(); it++)
    //         std::cout << "  Error: " << it->first << " " << it->second << "\n";
    //     for (size_t i = 0; i < locations.size(); i++)
    //         locations[i].print();
    //     std::cout << "\n";
    // }
}				ServerConfig;

#endif

class MyException: public std::exception
{
    private:
        std::string msg;
    public:
        MyException(std::string msg): msg(msg){};
        const char* what() const throw() {return msg.c_str();};
        virtual ~MyException() _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {};
};

