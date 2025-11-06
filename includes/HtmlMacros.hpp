#ifndef HTMLMACROS_HPP
# define HTMLMACROS_HPP

# include <string>
# include <sstream>

// Helper function for C++98 compatible size_t to string conversion
inline std::string size_to_string(size_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

// Basic HTML Building Block Macro Functions
# define HTML_DOCTYPE std::string("<!DOCTYPE html>")
# define HTML_TAG(tag, content) (std::string)"<" + tag ">" + content + "</" + tag ">"
# define HTML_SELF_CLOSING(tag) (std::string)"<" + tag + ">"
# define HTML_A(href, text) (std::string)"<a href=\"" + href + "\">" + text + "</a>"
# define HTML_TD_CLASS(cls, content) (std::string)"<td class=\"" + cls + "\">" + content + "</td>"

// Specific HTML Element Macros using the general HTML_TAG
# define HTML_HTML(content) HTML_TAG("html", content)
# define HTML_HEAD(content) HTML_TAG("head", content)
# define HTML_BODY(content) HTML_TAG("body", content)
# define HTML_TITLE(content) HTML_TAG("title", content)
# define HTML_H1(content) HTML_TAG("h1", content)
# define HTML_H2(content) HTML_TAG("h2", content)
# define HTML_P(content) HTML_TAG("p", content)
# define HTML_DIV(content) HTML_TAG("div", content)
# define HTML_UL(content) HTML_TAG("ul", content)
# define HTML_LI(content) HTML_TAG("li", content)
# define HTML_TABLE(content) HTML_TAG("table", content)
# define HTML_TR(content) HTML_TAG("tr", content)
# define HTML_TD(content) HTML_TAG("td", content)
# define HTML_TH(content) HTML_TAG("th", content)
# define HTML_EM(content) HTML_TAG("em", content)

// Self-closing tags
# define HTML_HR() HTML_SELF_CLOSING("hr")
# define HTML_BR() HTML_SELF_CLOSING("br")

// Style macro for inline CSS
# define HTML_STYLE(css) HTML_TAG("style", css)

// Main HTML Document Macro (takes head and body content)
# define HTML_DOCUMENT(head_content, body_content) \
	HTML_DOCTYPE + \
	HTML_HTML( \
		HTML_HEAD(head_content) + \
		HTML_BODY(body_content) \
	)

// Convenience macros for common page types
# define HTML_PAGE(title, body_content) \
	HTML_DOCUMENT(HTML_TITLE(title), body_content)

# define HTML_ERRPAGE(code, title, message) \
	HTML_PAGE((std::string)code + " " + title, \
		HTML_H1(title) + \
		HTML_P(message))

# define HTML_OK \
	HTML_PAGE("200 OK", \
		HTML_H1("Success") + \
		HTML_P("The request was successful."))

# define HTML_CREATED(location) \
	HTML_PAGE("201 Created", \
		HTML_H1("Resource Created") + \
		HTML_P("The resource has been created at " + HTML_A(location, location) + "."))

# define HTML_DELETED(location) \
	HTML_PAGE("200 OK", \
		HTML_H1("Resource Deleted") + \
		HTML_P("The resource at " + HTML_A(location, location) + " has been deleted."))

# define HTML_FORBIDDEN(location) \
	HTML_PAGE("403 Forbidden", \
		HTML_H1("Access Forbidden") + \
		HTML_P("You do not have permission to access " + HTML_A(location, location) + "."))

# define HTML_NOTFOUND(location) \
	HTML_PAGE("404 Not Found", \
		HTML_H1("Resource Not Found") + \
		HTML_P("The resource at " + HTML_A(location, location) + " was not found on this server."))

// Predefined error page macros using our HTML system
# define HTML_REDIRECT(code, location, message) \
	HTML_ERRPAGE(code, "Redirect", \
		"The document has moved <a href=\"" + location + "\">here</a>. " + message)
# define HTML_REDIRECT_301(location) HTML_REDIRECT("301", location, "Please update your bookmarks.")
# define HTML_REDIRECT_302(location) HTML_REDIRECT("302", location, "The document has been temporarily moved.")
# define HTML_REDIRECT_303(location) HTML_REDIRECT("303", location, "See other resource.")
# define HTML_REDIRECT_304(location) HTML_REDIRECT("304", location, "The document has not been modified.")
# define HTML_REDIRECT_305(location) HTML_REDIRECT("305", location, "Use proxy to access this resource.")
# define HTML_REDIRECT_307(location) HTML_REDIRECT("307", location, "The document has been temporarily moved.")
# define HTML_REDIRECT_308(location) HTML_REDIRECT("308", location, "Please update your bookmarks.")
# define HTML_ERROR_400 HTML_ERRPAGE("400", "Bad Request", "The request was invalid.")
# define HTML_ERROR_403 HTML_ERRPAGE("403", "Forbidden", "Access to this resource is forbidden.")
# define HTML_ERROR_403_UPLOAD HTML_ERRPAGE("403", "Forbidden", "File uploads are not allowed on this server.")
# define HTML_ERROR_404 HTML_ERRPAGE("404", "Not Found", "The requested resource was not found.")
# define HTML_ERROR_405(method) \
	HTML_ERRPAGE("405", "Method Not Allowed", \
		"The method `" + (method.empty() ? "Unknown" : method) + "' is not allowed for this resource.")
# define HTML_ERROR_408 HTML_ERRPAGE("408", "Request Timeout", "The request took too long to complete.")
# define HTML_ERROR_413(maxsize) \
	HTML_ERRPAGE("413", "Payload Too Large", "The request payload is too large." + \
					" Maximum allowed size is " + maxsize + " bytes.")
# define HTML_ERROR_500 HTML_ERRPAGE("500", "Internal Server Error", "An internal server error occurred.")
# define HTML_ERROR_501(method) \
	HTML_ERRPAGE("501", "Not Implemented", \
		"The method `" + (method.empty() ? "Unknown" : method) + "' is not implemented on this server.")
# define HTML_ERROR_502 HTML_ERRPAGE("502", "Bad Gateway", "The server received an invalid response from an upstream server.")
# define HTML_ERROR_503 HTML_ERRPAGE("503", "Service Unavailable", "The service is temporarily unavailable.")
# define HTML_ERROR_504 HTML_ERRPAGE("504", "Gateway Timeout", "The server did not receive a timely response from an upstream server.")
# define HTML_ERROR_505(protocol) \
	HTML_ERRPAGE("505", "HTTP Version Not Supported", \
		"The HTTP protocol version `" + (protocol.empty() ? "Unknown" : protocol) + "' is not supported by this server.")

// The following helper macros are provided to reduce repeated boilerplate
// when building CGI error pages and their corresponding HTTP responses.
// Usage examples:
//   std::string body = CGI_ERROR_BODY(error_code);
//   std::string resp = HTTP_ERROR_RESPONSE("502 Bad Gateway", body);

// Macro that returns an HTML error body string for common CGI error codes.
// Evaluates to an std::string expression.
# define CGI_ERROR_BODY(code) \
		( std::string("<!DOCTYPE html><html><head><title>CGI Error</title></head><body>") + \
			std::string("<h1>CGI Error</h1>") + \
			( (code) == 500 ? std::string("<p>Internal Server Error - CGI script execution failed.</p>") : \
				( (code) == 502 ? std::string("<p>Bad Gateway - CGI script produced invalid headers.</p>") : \
					( (code) == 504 ? std::string("<p>Gateway Timeout - CGI script took too long to respond.</p>") : \
						std::string("<p>An error occurred while processing the CGI request.</p>") ) ) ) + \
			std::string("</body></html>") )

// Macro that builds a full HTTP response string for a given status text
// (for example: "502 Bad Gateway") and an std::string body expression.
// Evaluates to an std::string expression and uses size_to_string for Content-Length.
# define HTTP_ERROR_RESPONSE(status_text, body) \
		( std::string("HTTP/1.1 ") + (status_text) + std::string("\r\n") + \
			std::string("Content-Type: text/html\r\n") + \
			std::string("Content-Length: ") + size_to_string((body).size()) + std::string("\r\n") + \
			std::string("Connection: close\r\n\r\n") + (body) )

# define DEFAULT_CSS "body { font-family: Arial, sans-serif; margin: 40px; }" \
					 "h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }" \
					 "table { border-collapse: collapse; width: 100%; margin-top: 20px; }" \
					 "th, td { text-align: left; padding: 8px 12px; border-bottom: 1px solid #ddd; }" \
					 "th { background-color: #f5f5f5; font-weight: bold; }" \
					 "a { text-decoration: none; color: #0066cc; }" \
					 "a:hover { text-decoration: underline; }" \
					 ".size { text-align: right; }"

#endif // HTMLMACROS_HPP
