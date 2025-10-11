#!/usr/bin/python3
import os
print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print("<p>Request method: %s</p>" % os.getenv("REQUEST_METHOD"))
print("<p>Script filename: %s</p>" % os.getenv("SCRIPT_FILENAME"))
print("<p>Server software: %s</p>" % os.getenv("SERVER_SOFTWARE"))
print("</body></html>")
