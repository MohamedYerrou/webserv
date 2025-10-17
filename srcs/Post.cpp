#include "../includes/Client.hpp"
#define  BUFFER_SIZE 8192

std::string extractBoundary(std::string& content_type)
{
    std::string key = "boundary";
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


void    Client::multiPartData()
{
    std::string content_type = currentRequest->getHeaders().at("Content-Type");
    std::string bounadry = extractBoundary(content_type);
    

    std::ifstream		in(filename.c_str(), std::ios::binary);
	std::ofstream		out;
    std::vector<char>	buffer;
	std::vector<char>	chunk(BUFFER_SIZE);
	bool				inpart = false;
	bool				headersparsed = false;
	size_t				readpos = 0;
	std::string			partFileName;
	std::string			partHeaders;

    while (in.good())
    {
		in.read(&chunk[0], BUFFER_SIZE);
		std::streamsize bytes = in.gcount();

		if (bytes <= 0)
		{
			in.close();
			break;
		}

		buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytes);

		size_t	boundaryPos = findBoundary(buffer, bounadry, readpos);

		while (boundaryPos != std::string::npos)
		{
			if (inpart && headersparsed && out.is_open())
				out.write(&buffer[readpos], boundaryPos - readpos - 2);

			partHeaders.clear();
			while (readpos < buffer.size())
			{
				size_t endLine = readpos;
				while (endLine + 1 < buffer.size() && !(buffer[endLine] == '\r' && buffer[endLine + 1] == '\n'))
					++endLine;
				
				if (endLine + 1 >= buffer.size())
					break;

				std::string line(&buffer[readpos], &buffer[boundaryPos]);
				readpos = endLine + 2;
				if (line.empty())
					break;
				partHeaders += line + "\n";
			}
			
			partFileName.clear();
			if (headers.find("filename=\"") != std::string::npos)
			{
				size_t begin_name = headers.find("filename=\"") + 10;
				size_t end_name = headers.find("\"", begin_name);
				partFileName = headers.substr(begin_name, end_name - begin_name);
			}

			if(!partFileName.empty())
			{
				out.open(partFileName.c_str(), std::ios::binary);
				if (out.is_open())
					std::cerr << "Cannot open output file: " << filename << std::endl;
			}

			inpart = true;
			headersparsed = true;
			boundaryPos = findBoundary(buffer, bounadry, readpos);
			
		}

		if (buffer.size() > bounadry.size() * 2)
		{
			size_t safezone = buffer.size() - bounadry.size() * 2;
			if (inpart && headersparsed && out.is_open())
			{
				out.write(&buffer[readpos], safezone - readpos);
				buffer.erase(buffer.begin(), buffer.begin() + safezone);
				readpos = 0;
			}
		}

    }

	if (inpart && headersparsed && out.is_open())
		out.write();

	if (out.is_open())
		out.close();
}

void    Client::handlePost()
{
    std::string		Content_type = currentRequest->getHeaders().at("Content-Type");
    std::fstream	body("testfile", std::ios::in);
    std::string		line;

    if (Content_type == "multipart/form-data")
    {
        multiPartData();
    }
    else
        return ;
}