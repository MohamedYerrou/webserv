#include "MiniServer.hpp"
#include "Request.hpp"

void    inputRequest()
{
    std::cout << "Enter your full http request:" << std::endl;
    std::string input;
    std::string fullRequest;
    while (true)
    {
        std::getline(std::cin, input);
        if (input.empty())
        {
            fullRequest += "\r\n";
            break;
        }
        fullRequest += input;
        fullRequest += "\r\n";
    }
    std::cout << "Want a body?: ";
    char c;
    std::cin >> c;
    if (c == 'y' || c == 'Y')
    {
        std::cout << "Enter the request body" << std::endl;
        std::string body;
        while (std::getline(std::cin, input))
        {
            body += input;
            body += "\r\n";
        }
        fullRequest += body;
    }
    Request req;
    req.parseRequest(fullRequest);
    parsedRequest(req);
}

int main()
{
    try
    {
        inputRequest();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;;
    }
    // MiniServer server;
    // Request req;

    // std::string post_request = 
    //     "POST /submit HTTP/1.1\r\n"
    //     "Host : example.com\r\n"
    //     "Content-Type: application/x-www-form-urlencoded\r\n"
    //     "Content-Length: 27\r\n"
    //     "\r\n"
    //     "name=John&email=john@example.com";
    
    // try
    // {
    //     Request post_req;
    //     post_req.parseRequest(post_request);
    //     parsedRequest(post_req);
    // } catch (std::exception& e)
    // {
    //     std::cout << e.what() << std::endl;
    // }


    // try
    // {
    //     server.setup();
    //     server.accept_connection();
    //     server.process_message();
    // }catch(const std::exception& e)
    // {
    //     std::cout << e.what() << std::endl;
    // }
    return 0;
}