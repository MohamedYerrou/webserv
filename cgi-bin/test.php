#!/usr/bin/php-cgi
<?php
// Set the content type for CGI output
header('Content-Type: text/html; charset=utf-8');

// Basic CGI environment information
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head>\n";
echo "    <title>PHP CGI Test</title>\n";
echo "    <style>\n";
echo "        body { font-family: Arial, sans-serif; margin: 40px; }\n";
echo "        .info { background: #f0f0f0; padding: 15px; margin: 10px 0; border-radius: 5px; }\n";
echo "        .success { color: green; font-weight: bold; }\n";
echo "        .error { color: red; font-weight: bold; }\n";
echo "    </style>\n";
echo "</head>\n";
echo "<body>\n";
echo "<h1>PHP CGI Test Page</h1>\n";

// Check if running as CGI
if (isset($_SERVER['GATEWAY_INTERFACE'])) {
    echo "<p class='success'>✓ Running as CGI</p>\n";
    echo "<div class='info'>\n";
    echo "<strong>CGI Environment Variables:</strong><br>\n";
    echo "GATEWAY_INTERFACE: " . $_SERVER['GATEWAY_INTERFACE'] . "<br>\n";
    echo "SERVER_SOFTWARE: " . ($_SERVER['SERVER_SOFTWARE'] ?? 'Not set') . "<br>\n";
    echo "REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "<br>\n";
    echo "</div>\n";
} else {
    echo "<p class='error'>✗ Not running as CGI (probably running as module)</p>\n";
}

// Server information
echo "<div class='info'>\n";
echo "<strong>Server Information:</strong><br>\n";
echo "PHP Version: " . phpversion() . "<br>\n";
echo "Server API: " . php_sapi_name() . "<br>\n";
echo "Document Root: " . $_SERVER['DOCUMENT_ROOT'] . "<br>\n";
echo "Script Filename: " . $_SERVER['SCRIPT_FILENAME'] . "<br>\n";
echo "</div>\n";

// Request information
echo "<div class='info'>\n";
echo "<strong>Request Information:</strong><br>\n";
echo "Remote Address: " . $_SERVER['REMOTE_ADDR'] . "<br>\n";
echo "User Agent: " . $_SERVER['HTTP_USER_AGENT'] . "<br>\n";
echo "Query String: " . ($_SERVER['QUERY_STRING'] ?? 'None') . "<br>\n";
echo "</div>\n";

// Test PHP functionality
echo "<div class='info'>\n";
echo "<strong>PHP Functionality Test:</strong><br>\n";
echo "Current Date/Time: " . date('Y-m-d H:i:s') . "<br>\n";
echo "Random Number: " . rand(1, 100) . "<br>\n";
echo "Working Directory: " . getcwd() . "<br>\n";
echo "</div>\n";

// Form test
echo "<div class='info'>\n";
echo "<strong>Form Test (GET method):</strong><br>\n";
echo "<form method='get'>\n";
echo "    Name: <input type='text' name='name' value='" . ($_GET['name'] ?? '') . "'>\n";
echo "    <input type='submit' value='Submit'>\n";
echo "</form>\n";
if (isset($_GET['name']) && !empty($_GET['name'])) {
    echo "Hello, " . htmlspecialchars($_GET['name']) . "!<br>\n";
}
echo "</div>\n";

// File upload test (if POST)
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "<div class='info'>\n";
    echo "<strong>POST Data Received:</strong><br>\n";
    echo "<pre>";
    print_r($_POST);
    echo "</pre>\n";
    echo "</div>\n";
}

echo "<hr>\n";
echo "<p><strong>CGI Test Results:</strong> ";
if (isset($_SERVER['GATEWAY_INTERFACE'])) {
    echo "<span class='success'>CGI is working correctly!</span></p>\n";
} else {
    echo "<span class='error'>CGI may not be configured properly.</span></p>\n";
}
echo "</body>\n";
echo "</html>\n";
?>