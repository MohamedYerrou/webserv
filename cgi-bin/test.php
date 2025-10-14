#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html; charset=UTF-8");

$message = isset($_POST["message"]) ? htmlspecialchars($_POST["message"]) : "";
$uploadMsg = "";

if (!empty($_FILES["file"]["name"])) {
    $uploadDir = "/home/myerrou/webserv/upload/";
    if (!is_dir($uploadDir)) mkdir($uploadDir, 0777, true);
    $filename = basename($_FILES["file"]["name"]);
    $path = $uploadDir . $filename;
    if (move_uploaded_file($_FILES["file"]["tmp_name"], $path)) {
        $uploadMsg = "<p class='success'> File <b>$filename</b> uploaded successfully!</p>";
    } else {
        $uploadMsg = "<p class='error'> Upload failed.</p>";
    }
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>PHP CGI | Webserv</title>
<link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet">
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
        font-family: 'Roboto', sans-serif;
        background: linear-gradient(135deg, #4b6cb7, #182848);
        color: #222;
        padding: 2rem;
    }
    .container {
        max-width: 850px;
        margin: auto;
        background: #ffffff;
        border-radius: 15px;
        padding: 2.5rem;
        box-shadow: 0 10px 30px rgba(0,0,0,0.25);
        animation: fadeIn 0.8s ease;
    }
    @keyframes fadeIn {
        from { opacity: 0; transform: translateY(25px); }
        to { opacity: 1; transform: translateY(0); }
    }
    h1 {
        text-align: center;
        background: linear-gradient(90deg, #4b6cb7, #182848);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        font-size: 2.2rem;
        margin-bottom: 1rem;
    }
    h2 {
        color: #182848;
        margin-top: 2rem;
        margin-bottom: 1rem;
        border-bottom: 2px solid #4b6cb7;
        display: inline-block;
        padding-bottom: 4px;
    }
    .info {
        background: #f5f7ff;
        padding: 1rem;
        border-radius: 10px;
        margin-bottom: 1.5rem;
        box-shadow: inset 0 0 10px rgba(0,0,0,0.05);
    }
    input[type=text], input[type=file] {
        width: 100%;
        padding: 0.6rem;
        border: 1px solid #ccc;
        border-radius: 10px;
        margin-bottom: 1rem;
        font-size: 1rem;
    }
    button {
        background: linear-gradient(90deg, #4b6cb7, #182848);
        color: white;
        padding: 0.6rem 1.2rem;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        font-size: 1rem;
        transition: all 0.3s ease;
    }
    button:hover {
        transform: translateY(-2px);
        box-shadow: 0 5px 15px rgba(0,0,0,0.2);
    }
    .success {
        color: #1a7f37;
        font-weight: bold;
        margin-top: 1rem;
    }
    .error {
        color: #b91c1c;
        font-weight: bold;
        margin-top: 1rem;
    }
    footer {
        text-align: center;
        margin-top: 2rem;
        color: #888;
        font-size: 0.9rem;
    }
</style>
</head>
<body>
<div class="container">
    <h1>PHP CGI Script</h1>

    <section class="info">
        <p><b>Server Time:</b> <?= date("Y-m-d H:i:s") ?></p>
        <p><b>Server Software:</b> <?= $_SERVER["SERVER_SOFTWARE"] ?? "Custom Webserv" ?></p>
        <p><b>Request Method:</b> <?= $_SERVER["REQUEST_METHOD"] ?? "N/A" ?></p>
    </section>

    <section>
        <h2>Send a Message</h2>
        <form method="post">
            <input type="text" name="message" placeholder="Type your message here...">
            <button type="submit">Send</button>
        </form>
        <?php if (!empty($message)): ?>
            <p class="success">You said: <b><?= $message ?></b></p>
        <?php endif; ?>
    </section>

    <section>
        <h2>Upload a File</h2>
        <form enctype="multipart/form-data" method="post">
            <input type="file" name="file">
            <button type="submit">Upload</button>
        </form>
        <?= $uploadMsg ?>
    </section>

    <footer>Webserv PHP CGI © 2025</footer>
</div>
</body>
</html>
