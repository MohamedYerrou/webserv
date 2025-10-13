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
#include <cstdio>

class Client
{
	private:
		int				fd;
		std::ifstream	fileStream;
		std::string 	headers;
		size_t			bodySize;
		bool			endHeaders;
		bool			reqComplete;
		bool			hasBody;
		bool			requestError;
		Request*		currentRequest;
		Server*			currentServer;
		const Location* location;
		Response		currentResponse;
		bool			sentAll;
		bool			fileOpened;
	public:
		Client(int fd, Server* srv);
		~Client();
		int					getFD() const;
		const std::string&	getHeaders() const;
		bool				getEndHeaders() const;
		bool				getReqComplete() const;
		void				appendData(const char* buf, ssize_t length);
		void				setBodySize(size_t size);
		void				handleHeaders(const std::string& raw);
		void				handleBody(const char* buf, ssize_t length);
		Request*			getRequest() const;
		void				handleCompleteRequest();
		Response& 			getResponse();
		bool				getRequestError() const;
		bool				getSentAll() const;
		void				setSentAll(bool flag);
		
		//handle methods
		const Location*	findMathLocation(std::string url);
		const Location* 		findBestMatch(const std::string uri);
		std::string		joinPath();
		void			handleGET();
        void            handleDELETE();
        void            handleDeleteFile(std::string totalPath);
        void            handleDeleteDir(std::string totalPath);
		bool			allowedMethod(const std::string& method);
		void			handleRedirection();
		void			errorResponse(int code, const std::string& error);
		void			handleDirectory(const std::string& path);
		void			handleFile();
		void			listingDirectory(std::string path);
		std::string		constructFilePath(std::string uri);
		void    		PrepareResponse(const std::string& path);
};

#endif

