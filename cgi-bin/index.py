#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print()

html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Environment Variables</title>
    <style>
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            padding: 30px;
        }
        h1 {
            color: #667eea;
            text-align: center;
            margin-bottom: 10px;
        }
        h2 {
            color: #764ba2;
            border-bottom: 2px solid #667eea;
            padding-bottom: 10px;
            margin-top: 30px;
        }
        .info {
            background: #f0f4ff;
            padding: 15px;
            border-left: 4px solid #667eea;
            margin-bottom: 20px;
            border-radius: 4px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        th {
            background: #667eea;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: bold;
        }
        td {
            padding: 10px 12px;
            border-bottom: 1px solid #e0e0e0;
        }
        tr:hover {
            background: #f5f7ff;
        }
        .key {
            font-weight: bold;
            color: #764ba2;
            font-family: 'Courier New', monospace;
        }
        .value {
            color: #333;
            font-family: 'Courier New', monospace;
            word-break: break-all;
        }
        .count {
            text-align: center;
            color: #667eea;
            font-size: 18px;
            margin-top: 20px;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 CGI Environment Variables</h1>
        <div class="info">
            <strong>Server:</strong> Webserv 1.0 (Custom C++98 HTTP Server)<br>
            <strong>CGI Protocol:</strong> CGI/1.1
        </div>
        
        <h2>📋 All Environment Variables</h2>
        <table>
            <tr>
                <th>Variable Name</th>
                <th>Value</th>
            </tr>
"""

print(html)

# Print all environment variables in sorted order
for key, value in sorted(os.environ.items()):
    print(f"            <tr>")
    print(f"                <td class='key'>{key}</td>")
    print(f"                <td class='value'>{value}</td>")
    print(f"            </tr>")

env_count = len(os.environ)
print(f"""        </table>
        <div class="count">Total: {env_count} environment variables</div>
    </div>
</body>
</html>""")
