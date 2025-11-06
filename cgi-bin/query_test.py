#!/usr/bin/env python3
import os
import sys
from urllib.parse import parse_qs

print("Content-Type: text/html\n")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Query String Test</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5rem;
            margin-bottom: 10px;
        }
        
        .header p {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        
        .content {
            padding: 30px;
        }
        
        .section {
            margin-bottom: 30px;
        }
        
        .section h2 {
            color: #667eea;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 2px solid #f0f0f0;
        }
        
        .query-display {
            background: #f8f9ff;
            border-left: 4px solid #667eea;
            padding: 15px 20px;
            border-radius: 5px;
            margin-bottom: 15px;
            font-family: 'Courier New', monospace;
            font-size: 1.1rem;
            word-break: break-all;
        }
        
        .empty-state {
            text-align: center;
            padding: 40px;
            color: #999;
        }
        
        .empty-state-icon {
            font-size: 4rem;
            margin-bottom: 20px;
        }
        
        .param-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }
        
        .param-table th,
        .param-table td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #e0e0e0;
        }
        
        .param-table th {
            background: #667eea;
            color: white;
            font-weight: 600;
        }
        
        .param-table tr:hover {
            background: #f8f9ff;
        }
        
        .param-key {
            font-weight: 600;
            color: #667eea;
        }
        
        .param-value {
            color: #333;
            font-family: 'Courier New', monospace;
        }
        
        .test-links {
            background: #fff4e6;
            border: 2px solid #ffb74d;
            border-radius: 10px;
            padding: 20px;
            margin-top: 20px;
        }
        
        .test-links h3 {
            color: #f57c00;
            margin-bottom: 15px;
        }
        
        .test-link {
            display: block;
            padding: 10px 15px;
            margin: 8px 0;
            background: white;
            border: 1px solid #ffb74d;
            border-radius: 5px;
            text-decoration: none;
            color: #f57c00;
            transition: all 0.3s;
        }
        
        .test-link:hover {
            background: #f57c00;
            color: white;
            transform: translateX(5px);
        }
        
        .env-info {
            background: #e8f5e9;
            border: 1px solid #4caf50;
            border-radius: 8px;
            padding: 15px;
            margin-top: 15px;
        }
        
        .env-row {
            display: flex;
            margin: 8px 0;
        }
        
        .env-label {
            font-weight: 600;
            color: #2e7d32;
            min-width: 200px;
        }
        
        .env-value {
            color: #333;
            font-family: 'Courier New', monospace;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔍 Query String Test</h1>
            <p>Test your CGI query string parsing</p>
        </div>
        
        <div class="content">
            <div class="section">
                <h2>📋 Current Query String</h2>
""")

query_string = os.environ.get('QUERY_STRING', '')

if query_string:
    print(f'                <div class="query-display">{query_string}</div>')
    
    # Parse the query string
    print('                <table class="param-table">')
    print('                    <thead>')
    print('                        <tr>')
    print('                            <th>Parameter</th>')
    print('                            <th>Value</th>')
    print('                        </tr>')
    print('                    </thead>')
    print('                    <tbody>')
    
    params = parse_qs(query_string)
    for key, values in params.items():
        for value in values:
            print(f'                        <tr>')
            print(f'                            <td class="param-key">{key}</td>')
            print(f'                            <td class="param-value">{value}</td>')
            print(f'                        </tr>')
    
    print('                    </tbody>')
    print('                </table>')
else:
    print("""                <div class="empty-state">
                    <div class="empty-state-icon">🤷</div>
                    <h3>No Query Parameters</h3>
                    <p>Try clicking one of the test links below!</p>
                </div>""")

print("""
            </div>
            
            <div class="section">
                <div class="test-links">
                    <h3>🧪 Test Examples</h3>
                    <a href="?name=John&age=25" class="test-link">
                        ➤ Simple parameters: name=John&age=25
                    </a>
                    <a href="?search=hello%20world&page=1" class="test-link">
                        ➤ URL encoded: search=hello world&page=1
                    </a>
                    <a href="?user=alice&user=bob&user=charlie" class="test-link">
                        ➤ Multiple values: user=alice&user=bob&user=charlie
                    </a>
                    <a href="?email=test@example.com&redirect=/home" class="test-link">
                        ➤ Special chars: email=test@example.com&redirect=/home
                    </a>
                    <a href="?q=python+programming&filter=tutorial&sort=date" class="test-link">
                        ➤ Complex query: q=python programming&filter=tutorial&sort=date
                    </a>
                    <a href="?" class="test-link">
                        ➤ Empty query string
                    </a>
                </div>
            </div>
            
            <div class="section">
                <h2>🌍 CGI Environment</h2>
                <div class="env-info">
""")

env_vars = [
    ('REQUEST_METHOD', os.environ.get('REQUEST_METHOD', 'N/A')),
    ('QUERY_STRING', os.environ.get('QUERY_STRING', 'N/A')),
    ('SCRIPT_NAME', os.environ.get('SCRIPT_NAME', 'N/A')),
    ('PATH_INFO', os.environ.get('PATH_INFO', 'N/A')),
    ('SERVER_PROTOCOL', os.environ.get('SERVER_PROTOCOL', 'N/A')),
    ('HTTP_HOST', os.environ.get('HTTP_HOST', 'N/A')),
]

for label, value in env_vars:
    print(f'                    <div class="env-row">')
    print(f'                        <span class="env-label">{label}:</span>')
    print(f'                        <span class="env-value">{value}</span>')
    print(f'                    </div>')

print("""                </div>
            </div>
        </div>
    </div>
</body>
</html>""")
