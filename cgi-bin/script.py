#!/usr/bin/env python3
import sys
data = sys.stdin.read()
print("Content-Type: text/html\r\n\r\n")
print("<h1>CGI OK</h1>")
print(f"<p>Body: {data}</p>")
