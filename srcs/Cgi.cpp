#include "../includes/Client.hpp"

// void Client::handleGET()
// {
// 	location = findMathLocation(currentRequest->getPath());
//     std::string totalPath = joinPath();
//     if (!location->getCgi().empty())
//     {
//         handleCGI(totalPath);
//         return;
//     }
// 	if (!location)
// 	{
//         errorResponse(404, "Not Found");
// 		return;
// 	}
// 	if (location->hasRedir())
// 	{
//         handleRedirection();
// 		return;
// 	}
// 	if (location->getRoot().empty())
// 	{
//         errorResponse(500, "Missing root directive");
// 		return;
// 	}
// 	if (isDir(totalPath))
// 	{
//         handleDirectory(totalPath);
// 		return;
// 	}
// 	if (isFile(totalPath))
// 	{
//         handleFile(totalPath);
// 		return;
// 	}
// 	errorResponse(404, "Not Found");
// }

void Client::handleCGI(std::string totalPath) 
{ 
    std::cout << "[CGI] Entered handleCGI() for path: " << totalPath << std::endl;
    if (isDir(totalPath))
    {
        const std::vector<std::string>& indexFiles = location->getIndex();
        if (!indexFiles.empty()) { totalPath += "/" + indexFiles[0];
            std::cout << "[CGI] Directory requested, using index file: " << totalPath << std::endl;
        }
        else
        {
            std::cerr << "[CGI] No index file found for directory: " << totalPath << std::endl;
            errorResponse(403, "Directory listing forbidden or index not found");
            return;
        }
    }
    std::size_t dotPos = totalPath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        std::cerr << "[CGI] No file extension found in path: " << totalPath << std::endl;
        errorResponse(400, "Invalid CGI path: missing extension");
        return;
    }
    std::string ext = totalPath.substr(dotPos);
    std::cout << "[CGI] Detected file extension: " << ext << std::endl;
    const std::map<std::string, std::string>& cgiMap = location->getCgi();
    std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
    if (it == cgiMap.end())
    {
        std::cerr << "[CGI] No interpreter configured for extension: " << ext << std::endl;
        errorResponse(500, "Unsupported CGI extension");
        return;
    }
    const std::string& interpreter = it->second;
    std::cout << "[CGI] Using interpreter: " << interpreter << std::endl;
    try
    {
        std::string output = executeCGI(totalPath, interpreter);
        std::cout << "[CGI] CGI executed successfully, output length: " << output.size() << " bytes" << std::endl;
        handleCGIResponse(output);
    }
    catch(const std::exception& e)
    {
        std::cerr << "[CGI] Exception while executing CGI: " << e.what() << std::endl;
        errorResponse(500, e.what());
    }
}


std::vector<std::string> Client::buildCGIEnv(const std::string& scriptPath)
{
	std::vector<std::string> env;

	env.push_back("GATEWAY_INTERFACE=CGI/1.0");
	env.push_back("SERVER_PROTOCOL=" + currentRequest->getProtocol());
	env.push_back("REQUEST_METHOD=" + currentRequest->getMethod());
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("PATH_INFO=" + currentRequest->getUri());

	// host and port
	std::map<std::string, std::string> headers = currentRequest->getHeaders();
	if (headers.find("Host") != headers.end())
		env.push_back("HTTP_HOST=" + headers["Host"]);

	if (headers.find("Content-Type") != headers.end())
		env.push_back("CONTENT_TYPE=" + headers["Content-Type"]);

	if (headers.find("Content-Length") != headers.end())
		env.push_back("CONTENT_LENGTH=" + headers["Content-Length"]);

	env.push_back("SERVER_SOFTWARE=server/1.0");
	env.push_back("REDIRECT_STATUS=200");

	return env;
}


std::string Client::executeCGI(const std::string& scriptPath, const std::string& interpreter)
{
	int inPipe[2];
	int outPipe[2];

	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
		throw MyException("pipe() failed");

	pid_t pid = fork();
	if (pid == -1)
		throw MyException("fork() failed");

	if (pid == 0)
	{
		// Child
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		std::vector<std::string> envVec = buildCGIEnv(scriptPath);
		char **envp = new char*[envVec.size() + 1];
		for (size_t i = 0; i < envVec.size(); ++i)
			envp[i] = strdup(envVec[i].c_str());
		envp[envVec.size()] = NULL;

		std::string cmd = interpreter.empty() ? scriptPath : interpreter;
		char *argv[] = { (char *)cmd.c_str(), (char *)scriptPath.c_str(), NULL };

		execve(cmd.c_str(), argv, envp);

		// If execve fails
		for (size_t i = 0; i < envVec.size(); ++i)
			free(envp[i]);
		delete [] envp;
		perror("execve");
		exit(1);
	}

	// Parent
	close(inPipe[0]);
	close(outPipe[1]);

	// send body if exists
	std::string body = "";
	if (currentRequest->getContentLength() > 0)
	{
		// you could read from tmp upload file instead
		// depending on how you store request bodies
	}
	write(inPipe[1], body.c_str(), body.size());
	close(inPipe[1]);

	// read CGI output
	std::string output;
	char buffer[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, bytesRead);

	close(outPipe[0]);
	waitpid(pid, NULL, 0);
	return output;
}


void Client::handleCGIResponse(const std::string& cgiOutput)
{
	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		errorResponse(500, "Invalid CGI output");
		return;
	}

	std::string headerPart = cgiOutput.substr(0, headerEnd);
	std::string bodyPart = cgiOutput.substr(headerEnd + 4);

	std::istringstream headerStream(headerPart);
	std::string line;
	while (std::getline(headerStream, line))
	{
		if (line.find(':') != std::string::npos)
		{
			size_t pos = line.find(':');
			std::string key = trim(line.substr(0, pos));
			std::string value = trim(line.substr(pos + 1));
			currentResponse.setHeaders(key, value);
		}
	}

	currentResponse.setProtocol("HTTP/1.0");
	currentResponse.setStatus(200, "OK");
	currentResponse.setBody(bodyPart);
}