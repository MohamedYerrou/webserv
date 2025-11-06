#ifndef CLIENT_HPP
#define CLIENT_HPP

#define	REQUEST_TIMEOUT 30
#define RESPONSE_TIMEOUT 10
#define CGI_TIMEOUT 10

#include <string>
#include <iostream>
#include <unistd.h>
#include "Request.hpp"
#include "Response.hpp"
#include "CGIHandler.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include "Utils.hpp"
#include <algorithm>
#include <fstream>
#include <dirent.h>
#include <cstdio>

class Server;

class Client
{
	private:
		int				fd;
		std::ifstream	fileStream;
		std::string 	headers;
		std::string		body;
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
		bool			oneBody;
		std::string		boundary;
		std::string		endBoundry;
		bool			inBody;
		bool			finishBody;
		time_t			lastActivityTime;
		bool			isRequesting;

		CGIHandler* cgiHandler;
		bool		isCGI;
		std::string newPath;

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
		void				handlePost(const char* buf, ssize_t length);
		Request*			getRequest() const;
		void				handleCompleteRequest();
		Response& 			getResponse();
		bool				getRequestError() const;
		bool				getSentAll() const;
		void				setSentAll(bool flag);
		bool    			getContentType();
		
		//handle methods
		const Location*		findMathLocation(std::string url);
		const Location*		findBestMatch(const std::string uri);
		std::string			joinPath();
		void				handleGET();
        void            	handleDELETE();
        void            	handleDeleteFile(std::string totalPath);
        void            	handleDeleteDir(std::string totalPath);
		bool				allowedMethod(const std::string& method);
		void				handleRedirection();
		void				errorResponse(int code, const std::string& error);
		void				handleDirectory(const std::string& path);
		void				handleFile();
		void				listingDirectory(std::string path);
		std::string			constructFilePath(std::string uri);
		void    			PrepareResponse(const std::string& path);
		bool				isTimedOut();
		void				setLastActivityTime(time_t time);
		void				setIsRequesting(bool flag);
		void    			handleBodyHeaders();
		void    			handleInBody();
		void    			handlePostError();
		
		//Cgi added by mohamed
		void				checkCGIValid();
		bool				getIsCGI() const;
		void 				setIsCGI(bool val);
		Server*				getServer() const;
		CGIHandler*			getCGIHandler() const;
		void				cleanupCGI();
		bool				isCgiTimedOut();
		Request*			getCurrentRequest();
		std::string			findActualScriptPath(const std::map<std::string, std::string>& cgiMap);
		void				buildCGIEnvironment(std::map<std::string, std::string>& env);
		void				setCGIPathVariables(std::map<std::string, std::string>& env, const std::string& fullPath);
		void				setCGIServerVariables(std::map<std::string, std::string>& env, const std::map<std::string, std::string>& headers);
		void				setCGIContentVariables(std::map<std::string, std::string>& env, const std::map<std::string, std::string>& headers);
		void				setCGIHttpHeaders(std::map<std::string, std::string>& env, const std::map<std::string, std::string>& headers);
};

#endif

