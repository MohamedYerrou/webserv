#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <iostream>
#include <unistd.h>
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include <sys/stat.h>
#include "Utils.hpp"
#include <algorithm>
#include <fstream>
#include <dirent.h>
#include "session.hpp"

class Client
{
	private:
		int									fd;
		std::string 						headers;
		ssize_t								bodySize;
		bool								endHeaders;
		bool								reqComplete;
		bool								hasBody;
		bool								requestError;
		Request*							currentRequest;
		Server*								currentServer;
		const Location* 					location;
		Response							currentResponse;
		std::string							filename;
		
		public:
		size_t								readPos;
		bool								inPart;
		bool								headersParsed;
		std::ofstream						out;
		std::string							partHeaders;
		std::string							partFilename;
		size_t								toAppend;
		session*							sess;
		std::map<std::string, std::string>	cookies;


		Client(int fd, Server* srv);
		~Client();
		int									getFD() const;
		const std::string&					getHeaders() const;
		bool								getEndHeaders() const;
		bool								getReqComplete() const;
		ssize_t								getBodySize() const;
		void								appendData(const char* buf, ssize_t length);
		void								setBodySize(size_t size);
		void								handleHeaders(const std::string& raw);
		void								handleBody(const char* buf, ssize_t length);
		Request*							getRequest() const;
		void								handleCompleteRequest();
		const Response& 					getResponse() const;
		bool								getRequestError() const;
		
		//handle methods
		const Location*						findMathLocation(std::string url);
		const Location*						findBestMatch(const std::string uri);
		std::string							joinPath();
		void								handleGET();
		bool								allowedMethod(const std::string& method);
		void								handleRedirection();
		void								errorResponse(int code, const std::string& error);
		void								handleDirectory(const std::string& path);
		void								handleFile(const std::string& path);
		void								listingDirectory(std::string path);
		std::string							constructFilePath(std::string uri);
		void								handlePost();
		void								multiPartData(const char* chunk, ssize_t length);
		void								NonMultiPartData(const char* chunk, ssize_t length);
		void								drainSocket(ssize_t length);
		void    							handleSession();
		std::map<std::string, std::string>  parseCookies(std::string);

};	
	
#endif	
	