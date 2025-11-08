#include "../includes/CGIHandler.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstring>

CGIHandler::CGIHandler(Client* c)
	: client(c), pid(-1), started(false), finished(false),
	  is_complete(false), is_error(false), error_code(0)
{
	in_fd[0] = in_fd[1] = -1;
	out_fd[0] = out_fd[1] = -1;
}

CGIHandler::~CGIHandler()
{
	if (in_fd[0] != -1)  close(in_fd[0]);
	if (in_fd[1] != -1)  close(in_fd[1]);
	if (out_fd[0] != -1) close(out_fd[0]);
	if (out_fd[1] != -1) close(out_fd[1]);

	if (pid > 0)
	{
		int status;
		if (waitpid(pid, &status, WNOHANG) <= 0)
		{
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);
		}
	}
}

bool CGIHandler::isFinished() const
{
	return finished;
}

std::string CGIHandler::getBuffer() const
{
	return buffer;
}

int CGIHandler::getOutFD() const
{
	return out_fd[0];
}

time_t CGIHandler::getStartTime() const
{
	return _startTime;
}

bool CGIHandler::isStarted() const
{
	return started;
}

pid_t CGIHandler::getPid() const
{
	return pid;
}

int CGIHandler::getStdinFd() const
{
	return in_fd[1];
}

int CGIHandler::getStdoutFd() const
{
	return out_fd[0];
}

void CGIHandler::appendResponse(const char* buf, size_t len)
{
	buffer.append(buf, len);
}

void CGIHandler::setComplete(bool value)
{
	is_complete = value;
}

void CGIHandler::setError(bool value)
{
	is_error = value;
}

void CGIHandler::setErrorCode(int code)
{
	error_code = code;
}

bool CGIHandler::isComplete() const
{
	return is_complete;
}

bool CGIHandler::hasError() const
{
	return is_error;
}

int CGIHandler::getErrorCode() const
{
	return error_code;
}

void CGIHandler::setupPipes(int err_pipe[2])
{
	if (pipe(in_fd) == -1 || pipe(out_fd) == -1 || pipe(err_pipe) == -1)
	{
		if (in_fd[0] != -1)    close(in_fd[0]);
		if (in_fd[1] != -1)    close(in_fd[1]);
		if (out_fd[0] != -1)   close(out_fd[0]);
		if (out_fd[1] != -1)   close(out_fd[1]);
		if (err_pipe[0] != -1) close(err_pipe[0]);
		if (err_pipe[1] != -1) close(err_pipe[1]);
		throw std::runtime_error("pipe failed");
	}

	if (fcntl(err_pipe[1], F_SETFD, FD_CLOEXEC) == -1)
	{
		close(in_fd[0]); close(in_fd[1]);
		close(out_fd[0]); close(out_fd[1]);
		close(err_pipe[0]); close(err_pipe[1]);
		throw std::runtime_error("fcntl failed");
	}
}

std::vector<char*> CGIHandler::buildEnvp(const std::map<std::string, std::string>& env)
{
	std::vector<char*> envp;
	for (std::map<std::string, std::string>::const_iterator it = env.begin();
		 it != env.end(); ++it)
	{
		std::string envStr = it->first + "=" + it->second;
		char* envCstr = new char[envStr.size() + 1];
		std::strcpy(envCstr, envStr.c_str());
		envp.push_back(envCstr);
	}
	envp.push_back(NULL);
	return envp;
}

void CGIHandler::executeScript(const std::string& scriptPath, std::vector<char*>& envp)
{
	if (access(scriptPath.c_str(), F_OK) != 0)
	{
		write(STDOUT_FILENO, "E", 1);
		for (size_t i = 0; i < envp.size(); ++i)
			delete[] envp[i];
		exit(1);
	}
	
	std::map<std::string, std::string> cgi =
		client->findMathLocation(client->getCurrentRequest()->getPath())->getCgi();

	std::string interpreter;
	size_t dotPos = scriptPath.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string ext = scriptPath.substr(dotPos);
		std::map<std::string, std::string>::const_iterator it = cgi.find(ext);
		if (it != cgi.end())
			interpreter = it->second;
	}
	
	if (!interpreter.empty())
	{
		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(scriptPath.c_str()),
			NULL
		};
		execve(interpreter.c_str(), argv, &envp[0]);
	}
	else
	{
		char* argv[] = { const_cast<char*>(scriptPath.c_str()), NULL };
		execve(scriptPath.c_str(), argv, &envp[0]);
	}
	
	write(STDOUT_FILENO, "E", 1);
	for (size_t i = 0; i < envp.size(); ++i)
		delete[] envp[i];
	exit(1);
}

void CGIHandler::executeChildProcess(const std::string& scriptPath, 
									   const std::map<std::string, std::string>& env, 
									   int err_pipe[2])
{
	close(in_fd[1]);
	close(out_fd[0]);
	close(err_pipe[0]);

	dup2(in_fd[0], STDIN_FILENO);
	dup2(out_fd[1], STDOUT_FILENO);
	dup2(out_fd[1], STDERR_FILENO);
	close(in_fd[0]);
	close(out_fd[1]);

	std::vector<char*> envp = buildEnvp(env);
	executeScript(scriptPath, envp);
}

void CGIHandler::handleParentProcess(int err_pipe[2], const std::string& scriptPath)
{
	close(in_fd[0]);
	close(out_fd[1]);
	close(err_pipe[1]);
	fcntl(err_pipe[0], F_SETFL, O_NONBLOCK);
	
	char err = 0;
	ssize_t n = read(err_pipe[0], &err, 1);
	close(err_pipe[0]);
	
	if (n == -1 || n == 0)
	{
		// read failed or EOF with no data - treat as success (exec succeeded and closed pipe)
		fcntl(out_fd[0], F_SETFL, O_NONBLOCK);
		fcntl(in_fd[1], F_SETFL, O_NONBLOCK);
		started = true;
		_startTime = time(NULL);
		return;
	}
	
	if (n > 0)
	{
		// Child wrote error byte - exec failed
		int status;
		waitpid(pid, &status, WNOHANG);
		pid = -1;
		
		if (in_fd[1] != -1)  close(in_fd[1]);
		if (out_fd[0] != -1) close(out_fd[0]);
		
		throw std::runtime_error(std::string("execve failed for ") + scriptPath);
	}

	// fcntl(out_fd[0], F_SETFL, O_NONBLOCK);
	// fcntl(in_fd[1], F_SETFL, O_NONBLOCK);
	// started = true;
	// _startTime = time(NULL);
}

void CGIHandler::startCGI(const std::string& scriptPath,
						   const std::map<std::string, std::string>& env)
{
	int err_pipe[2];
	
	setupPipes(err_pipe);
	
	pid = fork();
	if (pid == -1)
	{
		close(in_fd[0]); close(in_fd[1]);
		close(out_fd[0]); close(out_fd[1]);
		close(err_pipe[0]); close(err_pipe[1]);
		throw std::runtime_error("fork failed");
	}
	
	if (pid == 0)
		executeChildProcess(scriptPath, env, err_pipe);
	else
		handleParentProcess(err_pipe, scriptPath);
}

std::string Client::findActualScriptPath(const std::map<std::string, std::string>& cgiMap)
{
	std::string actualPath = newPath;
	
	if (!cgiMap.empty() && !isFile(newPath) && !isDir(newPath))
	{
		std::string testPath = newPath;
		while (!testPath.empty())
		{
			if (isFile(testPath))
			{
				size_t dotPos = testPath.find_last_of('.');
				if (dotPos != std::string::npos)
				{
					std::string ext = testPath.substr(dotPos);
					if (cgiMap.find(ext) != cgiMap.end())
					{
						actualPath = testPath;
						break;
					}
				}
			}
			
			size_t lastSlash = testPath.find_last_of('/');
			if (lastSlash == std::string::npos || lastSlash == 0)
				break;
			testPath = testPath.substr(0, lastSlash);
		}
	}
	
	return actualPath;
}

void Client::setCGIPathVariables(std::map<std::string, std::string>& env, const std::string& fullPath)
{
	size_t queryPos = fullPath.find('?');
	std::string pathWithoutQuery = (queryPos != std::string::npos) ? 
		fullPath.substr(0, queryPos) : fullPath;
	
	size_t scriptEndPos = pathWithoutQuery.find(newPath.substr(newPath.find_last_of('/') + 1));
	if (scriptEndPos == std::string::npos)
	{
		size_t lastSlash = pathWithoutQuery.find_last_of('/');
		if (lastSlash != std::string::npos)
		{
			std::string scriptName = newPath.substr(newPath.find_last_of('/'));
			scriptEndPos = pathWithoutQuery.find(scriptName);
			if (scriptEndPos != std::string::npos)
				scriptEndPos += scriptName.length();
		}
	}
	else
	{
		scriptEndPos += newPath.substr(newPath.find_last_of('/') + 1).length();
	}
	
	std::string scriptName;
	std::string pathInfo;
	std::string pathTranslated;
	
	if (scriptEndPos != std::string::npos && scriptEndPos < pathWithoutQuery.length())
	{
		scriptName = pathWithoutQuery.substr(0, scriptEndPos);
		std::string rawPathInfo = pathWithoutQuery.substr(scriptEndPos);
		pathInfo = urlDecode(rawPathInfo);
		
		if (!pathInfo.empty())
		{
			std::string root = location->getRoot();
			if (root[root.length() - 1] == '/')
				root.erase(root.length() - 1);
			
			std::string locationPath = location->getPATH();
			if (locationPath != "/" && pathInfo.find(locationPath) == 0)
				pathInfo = pathInfo.substr(locationPath.length());
			
			pathTranslated = root + pathInfo;
		}
	}
	else
	{
		scriptName = pathWithoutQuery;
		pathInfo = "";
		pathTranslated = "";
	}
	
	env["SCRIPT_NAME"] = scriptName;
	env["PATH_INFO"] = pathInfo;
	
	if (!pathInfo.empty())
		env["PATH_TRANSLATED"] = pathTranslated;
	
	if (queryPos != std::string::npos)
		env["QUERY_STRING"] = fullPath.substr(queryPos + 1);
	else
		env["QUERY_STRING"] = "";
}

void Client::setCGIServerVariables(std::map<std::string, std::string>& env, 
									const std::map<std::string, std::string>& headers)
{
	env["REMOTE_ADDR"] = "127.0.0.1";
	env["REMOTE_HOST"] = "127.0.0.1";
	
	std::map<std::string, std::string>::const_iterator host_it = headers.find("Host");
	if (host_it == headers.end())
		host_it = headers.find("host");
	
	if (host_it != headers.end())
	{
		std::string host = host_it->second;
		size_t colonPos = host.find(':');
		if (colonPos != std::string::npos)
		{
			env["SERVER_NAME"] = host.substr(0, colonPos);
			env["SERVER_PORT"] = host.substr(colonPos + 1);
		}
		else
		{
			env["SERVER_NAME"] = host;
			env["SERVER_PORT"] = "80";
		}
	}
	else
	{
		env["SERVER_NAME"] = "localhost";
		env["SERVER_PORT"] = "80";
	}
}

void Client::setCGIContentVariables(std::map<std::string, std::string>& env, 
									 const std::map<std::string, std::string>& headers)
{
	std::map<std::string, std::string>::const_iterator ct_it = headers.find("Content-Type");
	if (ct_it == headers.end())
		ct_it = headers.find("content-type");
	
	if (ct_it != headers.end())
		env["CONTENT_TYPE"] = ct_it->second;
	else
		env["CONTENT_TYPE"] = "";
	
	if (currentRequest->getMethod() == "POST" || currentRequest->getMethod() == "PUT")
	{
		std::ostringstream oss;
		oss << currentRequest->getBody().size();
		env["CONTENT_LENGTH"] = oss.str();
	}
	else
		env["CONTENT_LENGTH"] = "";
}

void Client::setCGIHttpHeaders(std::map<std::string, std::string>& env, 
								const std::map<std::string, std::string>& headers)
{
	env["REDIRECT_STATUS"] = "200";
	for (std::map<std::string, std::string>::const_iterator h_it = headers.begin();
		 h_it != headers.end(); ++h_it)
	{
		std::string headerName = h_it->first;
		
		if (headerName == "Content-Type" || headerName == "content-type" ||
			headerName == "Content-Length" || headerName == "content-length")
			continue;
		
		std::string metaVar = "HTTP_";
		for (size_t i = 0; i < headerName.length(); ++i)
		{
			char c = headerName[i];
			if (c == '-')
				metaVar += '_';
			else if (c >= 'a' && c <= 'z')
				metaVar += (c - 32);
			else
				metaVar += c;
		}
		
		env[metaVar] = h_it->second;
	}
}

void Client::buildCGIEnvironment(std::map<std::string, std::string>& env)
{
	const std::map<std::string, std::string>& headers = currentRequest->getHeaders();
	
	env["REQUEST_METHOD"] = currentRequest->getMethod();
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	env["SCRIPT_FILENAME"] = newPath;
	
	std::string fullPath = currentRequest->getUri();
	setCGIPathVariables(env, fullPath);
	setCGIServerVariables(env, headers);
	setCGIContentVariables(env, headers);
	setCGIHttpHeaders(env, headers);
}

void Client::checkCGIValid()
{
	const std::vector<std::string>& allowedMethods = location->getMethod();
	const std::string& requestMethod = currentRequest->getMethod();
	bool methodAllowed = false;

	for (size_t i = 0; i < allowedMethods.size(); ++i)
	{
		if (allowedMethods[i] == requestMethod)
		{
			methodAllowed = true;
			break;
		}
	}

	if (!methodAllowed)
		return errorResponse(405, "Method not allowed");

	const std::map<std::string, std::string>& cgiMap = location->getCgi();
	bool cgiConfigured = !cgiMap.empty();

	std::string actualPath = findActualScriptPath(cgiMap);

	if (isDir(actualPath))
	{
		const std::vector<std::string>& indexFiles = location->getIndex();
		bool indexFound = false;
		std::string originalPath = actualPath;

		if (!indexFiles.empty())
		{
			if (originalPath.length() > 0 && originalPath[originalPath.length() - 1] != '/')
				originalPath += "/";

			for (size_t i = 0; i < indexFiles.size(); ++i)
			{
				std::string indexPath = originalPath + indexFiles[i];
				if (isFile(indexPath))
				{
					actualPath = indexPath;
					indexFound = true;
					break;
				}
			}
		}

		if (!indexFound)
		{
			if (location->getAutoIndex())
				return listingDirectory(actualPath);
			else
				return errorResponse(403, "Forbidden");
		}
	}
	else if (!isFile(actualPath))
		return errorResponse(404, "Not found");

	newPath = actualPath;

	if (cgiConfigured)
	{
		size_t dotPos = newPath.find_last_of('.');
		if (dotPos != std::string::npos)
		{
			std::string ext = newPath.substr(dotPos);
			std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
			
			if (it != cgiMap.end())
			{
				setIsCGI(true);
				cgiHandler = new CGIHandler(this);
				
				std::map<std::string, std::string> env;
				buildCGIEnvironment(env);
				
				cgiHandler->startCGI(newPath, env);
				return;
			}
		}
	}
}