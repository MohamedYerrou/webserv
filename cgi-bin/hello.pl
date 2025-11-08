#!/usr/bin/env perl

# Use standard Perl best practices
use strict;
use warnings;

# --- 1. The CGI Header ---
# This is the most important line.
# It MUST be the first thing you print.
# The two newlines (\n\n) are required.
print "Content-Type: text/html; charset=utf-8\n\n";

# --- 2. The HTML Body ---
my $server_time = scalar(localtime());
my $request_method = $ENV{'REQUEST_METHOD'} || "Not set";

# We use a "here-document" (<<EOF) to print
# a multi-line block of HTML easily.
print <<"EOF";
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Simple Perl CGI</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 40px; 
            background: #f0f2f5;
        }
        .container { 
            background: #fff; 
            border: 1px solid #ddd; 
            border-radius: 8px; 
            padding: 1.5rem;
            max-width: 600px;
        }
        h1 { color: #005a9c; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ Hello from Perl!</h1>
        <p>This is a simple, no-dependency CGI script.</p>
        <hr>
        <p><b>Server Time:</b> $server_time</p>
        <p><b>Request Method:</b> $request_method</p>
    </div>
</body>
</html>
EOF