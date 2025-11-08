#ifndef HTMLMACROS_HPP
# define HTMLMACROS_HPP

# include <string>
# include <sstream>

inline std::string size_to_string(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

# define HTML_DOCTYPE std::string("<!DOCTYPE html>")
# define HTML_TAG(tag, content) (std::string)"<" + tag ">" + content + "</" + tag ">"
# define HTML_HTML(content) HTML_TAG("html", content)
# define HTML_HEAD(content) HTML_TAG("head", content)
# define HTML_BODY(content) HTML_TAG("body", content)
# define HTML_TITLE(content) HTML_TAG("title", content)
# define HTML_H1(content) HTML_TAG("h1", content)
# define HTML_P(content) HTML_TAG("p", content)

# define HTML_DOCUMENT(head_content, body_content) \
	HTML_DOCTYPE + \
	HTML_HTML( \
		HTML_HEAD(head_content) + \
		HTML_BODY(body_content) \
	)

# define HTML_PAGE(title, body_content) \
	HTML_DOCUMENT(HTML_TITLE(title), body_content)

# define HTML_ERRPAGE(code, title, message) \
	HTML_PAGE((std::string)code + " " + title, \
		HTML_H1(title) + \
		HTML_P(message))


# define HTML_ERROR_408 HTML_ERRPAGE("408", "Request Timeout", "The request took too long to complete.")
# define HTML_ERROR_500 HTML_ERRPAGE("500", "Internal Server Error", "An internal server error occurred.")
# define HTML_ERROR_502 HTML_ERRPAGE("502", "Bad Gateway", "The server received an invalid response from an upstream server.")
# define HTML_ERROR_504 HTML_ERRPAGE("504", "Gateway Timeout", "The server did not receive a timely response from an upstream server.")

# define CGI_ERROR_BODY(code) \
		( std::string("<!DOCTYPE html><html><head><title>CGI Error</title></head><body>") + \
			std::string("<h1>CGI Error</h1>") + \
			( (code) == 500 ? std::string("<p>Internal Server Error - CGI script execution failed.</p>") : \
				( (code) == 502 ? std::string("<p>Bad Gateway - CGI script produced invalid headers.</p>") : \
					( (code) == 504 ? std::string("<p>Gateway Timeout - CGI script took too long to respond.</p>") : \
						std::string("<p>An error occurred while processing the CGI request.</p>") ) ) ) + \
			std::string("</body></html>") )

# define HTTP_ERROR_RESPONSE(status_text, body) \
		( std::string("HTTP/1.1 ") + (status_text) + std::string("\r\n") + \
			std::string("Content-Type: text/html\r\n") + \
			std::string("Content-Length: ") + size_to_string((body).size()) + std::string("\r\n") + \
			std::string("Connection: close\r\n\r\n") + (body) )

#endif
