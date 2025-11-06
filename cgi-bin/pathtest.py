#!/usr/bin/env python3
import os

print("Content-Type: text/plain\r\n\r\n")
print(f"SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'NOT SET')}")
print(f"SCRIPT_FILENAME: {os.environ.get('SCRIPT_FILENAME', 'NOT SET')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'NOT SET')}")
print(f"PATH_TRANSLATED: {os.environ.get('PATH_TRANSLATED', 'NOT SET')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'NOT SET')}")