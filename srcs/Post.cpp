#include "../includes/Client.hpp"
#define  BUFFER_SIZE 8192

std::string extractBoundary(std::string& content_type)
{
    std::string key = "boundary=";
    size_t pos = content_type.find(key);
    if (pos == std::string::npos)
        return "";
    else
        return "--" + content_type.substr(pos + key.size());
}

size_t	findBoundary(std::vector<char>& buffer, std::string& boundary, size_t start)
{
	for (size_t i = start; i <= buffer.size() - boundary.size(); i++)
	{
		if (std::equal(boundary.begin(), boundary.end(), buffer.begin() + i))
			return i;
	}
	return std::string::npos;	
}

void	Client::drainSocket(ssize_t length)
{
	// std::cout << bodySize << std::endl;
	bodySize -= std::min(bodySize, length);
	if (bodySize <= 0)
		reqComplete = true;
	return;
}

void    Client::handleBody(const char* chunk, ssize_t length)
{
	std::string			Content_type = currentRequest->getHeaders().at("Content-Type");
	
	if (Content_type.find("multipart/form-data") != std::string::npos)
		multiPartData(chunk, length);
	else
		NonMultiPartData(chunk, length);
	if (bodySize <= 0)
	{
		std::cout << "file posted2" << std::endl;
		out.close();
		currentResponse = Response();
		std::string bodyStr = "Upload done.";
		currentResponse.setProtocol(currentRequest->getProtocol());
		currentResponse.setStatus(200, "OK");
		currentResponse.setHeaders("Content-Type", "text/plain");
		currentResponse.setHeaders("Content-Length", intTostring(bodyStr.length()));
		currentResponse.setHeaders("Date", currentDate());
		currentResponse.setHeaders("Connection", "close");
		currentResponse.setBody(bodyStr);
		// currentRequest->closeFileUpload();
		reqComplete = true;
		return ;
	}
}


void	Client::NonMultiPartData(const char* chunk, ssize_t length)
{
	size_t	toAppend = std::min(length, bodySize);
	if (bodySize > 0 && out.is_open())
	{
		out.write(chunk, toAppend);
		bodySize -= toAppend;
		return ;

	}
	if (!out.is_open())
		out.open("tmpfile", std::ios::binary);
	if (!out.is_open())
		std::cerr << "Can't open the file" << std::endl;
	if (bodySize > 0)
	{
		out.write(chunk, toAppend);
		bodySize -= toAppend;
	}	
}



void	Client::multiPartData(const char* chunk, ssize_t length)
{
	std::vector<char>&	buf = currentRequest->buffer;
	std::string			Content_type = currentRequest->getHeaders().at("Content-Type");
	std::string			boundary = extractBoundary(Content_type);
	size_t				boundaryPos = 0;
	// size_t				toAppend = 0;

	std::cout << "length " << length << std::endl;
	std::cout << "bodysize" << bodySize << std::endl;
	toAppend += std::min(length, bodySize);
	std::cout << "eeee " << toAppend << std::endl;
    buf.insert(buf.end(), chunk, chunk + toAppend);
	boundaryPos = findBoundary(buf, boundary, readPos);
	if (boundaryPos != std::string::npos)
	{
		if (bodySize && headersParsed && out.is_open())
		{
			out.write(&buf[readPos], boundaryPos - readPos - 2);
			buf.erase(buf.begin(), buf.begin() + boundaryPos);
			headersParsed = false;
			readPos = 0;
			bodySize -= toAppend;
			if (bodySize <= 0)
			{
				std::cout << "file posted1" << std::endl;
				out.close();
				return ;
			}
		}
		


		if (out.is_open())
		{
			out.close();
			std::cout << "out is closed" << std::endl;
		}


		if (std::equal(buf.begin() + boundaryPos, buf.begin() + boundaryPos + boundary.size() + 2, (boundary + "--").begin()))
		{
			out.close();
			std::cout << "Final boundary" << std::endl;
			return ;
		}


		readPos = findBoundary(buf, boundary, readPos) + boundary.size();
		if (readPos + 1 < buf.size() && buf[readPos] == '\r' && buf[readPos + 1] == '\n')
				readPos += 2;

		if (!headersParsed)
		{
			partHeaders.clear();
			while (readPos < buf.size())
			{
				size_t				lineEnd = readPos;
				
				while (lineEnd + 1 < buf.size() && !(buf[lineEnd] == '\r' && buf[lineEnd + 1] == '\n'))
					++lineEnd;
			
				if (lineEnd + 1 >= buf.size())
					return;

				std::string			line(&buf[readPos], &buf[lineEnd]);

				readPos = lineEnd + 2;
		
				if (line.empty())
				{
					headersParsed = true;
					break;
				}
				partHeaders += line + "\n";
			}
		}
		


		partFilename.clear();
		if (headersParsed && partHeaders.find("filename=") != std::string::npos)
		{
			size_t name_begin = partHeaders.find("filename=") + 10;
			size_t name_end = partHeaders.find("\"", name_begin);
			partFilename = partHeaders.substr(name_begin, name_end - name_begin);
		}
	


		if (!partFilename.empty())
			out.open(("/home/forrest/Desktop/webgroup/" + partFilename).c_str(), std::ios::binary);

		if (!out.is_open())
			std::cerr << "can't open the file" << std::endl;
	


		boundaryPos = findBoundary(buf, boundary, readPos);

		if (bodySize && headersParsed && out.is_open() && boundaryPos != std::string::npos)
		{			
			out.write(&buf[readPos], boundaryPos - readPos - 2);
			buf.erase(buf.begin(), buf.begin() + boundaryPos);
			headersParsed = false;
			readPos = 0;
			bodySize -= toAppend;
			if (bodySize <= 0)
			{
				std::cout << "file posted1" << std::endl;
				out.close();
				return ;
			}
		}
	}
	else if (headersParsed && buf.size() > boundary.size() * 2 && out.is_open())
	{
		size_t safeZone = buf.size() - boundary.size() * 2;
        if (safeZone > readPos)
		{
			std::cout << "aaaaaaa" << std::endl;
		    out.write(&buf[readPos], safeZone - readPos);	
			buf.erase(buf.begin(), buf.begin() + safeZone);
			readPos = 0;
			bodySize -= toAppend;
			if (bodySize <= 0)
			{
				std::cout << "file posted1" << std::endl;
				out.close();
				return ;
			}
		}
	}
}













// void    Client::multiPartData()
// {
//     std::string content_type = currentRequest->getHeaders().at("Content-Type");
//     std::string boundary = extractBoundary(content_type);
    

//     std::ifstream		in(filename.c_str(), std::ios::binary);
// 	std::ofstream		out;
//     std::vector<char>	buffer;
// 	std::vector<char>	chunk(BUFFER_SIZE);
// 	bool				inpart = false;
// 	bool				headersparsed = false;
// 	size_t				readpos = 0;
// 	std::string			partFileName;
// 	std::string			partHeaders;

//     while (in.good())
//     {
// 		in.read(&chunk[0], BUFFER_SIZE);
// 		std::streamsize bytes = in.gcount();

// 		if (bytes <= 0)
// 		{
// 			in.close();
// 			break;
// 		}

// 		buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytes);

// 		size_t	boundaryPos = findBoundary(buffer, boundary, readpos);

// 		while (boundaryPos != std::string::npos)
// 		{
// 			if (inpart && headersparsed && out.is_open())
// 			{
// 				out.write(&buffer[readpos], boundaryPos - readpos - 2);
// 				headersparsed = false;
// 			}

// 			if (out.is_open())
//                 out.close();

// 			if (boundaryPos + boundary.size() + 2 <= buffer.size() && std::equal(buffer.begin() + boundaryPos,
// 				buffer.begin() + boundaryPos + boundary.size() + 2, (boundary + "--").begin()))
// 			{
// 				in.close();
// 				return;
// 			}

// 			readpos = boundaryPos + boundary.size();
// 			if (readpos + 1 < buffer.size() && buffer[readpos] == '\r' && buffer[readpos + 1] == '\n')
// 				readpos += 2;

// 			partHeaders.clear();
// 			// if (headersparsed)
// 			// {
// 				while (readpos < buffer.size())
// 				{
// 					size_t endLine = readpos;
// 					while (endLine + 1 < buffer.size() && !(buffer[endLine] == '\r' && buffer[endLine + 1] == '\n'))
// 						++endLine;
					
// 					if (endLine + 1 >= buffer.size())
// 						break;
	
// 					std::string line(&buffer[readpos], &buffer[endLine]);
// 					readpos = endLine + 2;
// 					if (line.empty())
// 						break;
// 					partHeaders += line + "\n";
// 				}
// 				std::cout << partHeaders << std::endl;
// 			// }
// 			partFileName.clear();
// 			if (partHeaders.find("filename=\"") != std::string::npos)
// 			{
// 				size_t begin_name = partHeaders.find("filename=\"") + 10;
// 				size_t end_name = partHeaders.find("\"", begin_name);
// 				partFileName = partHeaders.substr(begin_name, end_name - begin_name);
// 				std::cout << partFileName << std::endl;
// 			}

// 			if(!partFileName.empty())
// 			{
// 				out.open(("/home/forrest/Desktop/webgroup/" + partFileName).c_str(), std::ios::binary);
// 				if (out.is_open())
// 					std::cerr << "Cannot open output file: " << filename << std::endl;
// 			}

// 			inpart = true;
// 			headersparsed = true;
// 			boundaryPos = findBoundary(buffer, boundary, readpos);
			
// 		}

// 		if (buffer.size() > boundary.size() * 2)
// 		{
// 			size_t safezone = buffer.size() - boundary.size() * 2;
// 			if (inpart && headersparsed && out.is_open())
// 			{
// 				out.write(&buffer[readpos], safezone - readpos);
// 			}
// 			buffer.erase(buffer.begin(), buffer.begin() + safezone);
// 			readpos = 0;
// 		}

//     }

// 	if (inpart && headersparsed && out.is_open())
// 		out.write(&buffer[readpos], buffer.size() - readpos);

// 	if (out.is_open())
// 		out.close();
// }

void    Client::handlePost()
{
    std::string		Content_type = currentRequest->getHeaders().at("Content-Type");
    std::fstream	body("testfile", std::ios::in);
    std::string		line;

	std::cout << Content_type << std::endl;
    if (Content_type.find("multipart/form-data") != std::string::npos)
    {
		// std::cout << "ooooooooojjjjjjjjjjjjjj" << std::endl;
        // multiPartData();
    }
    else
        return ;
}