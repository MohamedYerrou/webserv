# Webserv - C++98 HTTP Server

This is a **custom HTTP/1.1 web server** built in C++98, implementing nginx-like configuration parsing, epoll-based I/O multiplexing, and CGI execution. It's a 42 school project recreating core web server functionality from scratch.

## Architecture Overview

**Core Components:**
- **Server**: Manages multiple listen sockets, holds Location configs and CGI context maps (`CGIstdIn`/`CGIstdOut`)
- **Client**: Per-connection state machine handling request parsing, response generation, file streaming, and CGI lifecycle
- **Request/Response**: HTTP message parsing and building
- **Location**: Route-based configuration (like nginx location blocks)
- **CGIHandler**: Manages fork/exec of CGI scripts with bidirectional pipes

**Event Flow (epoll-driven):**
1. `main.cpp`: Tokenizes config → `parser()` creates Server instances → each calls `init_server()` to bind/listen
2. `run_server()`: epoll loop distinguishes listening FDs (new connections) vs client FDs (EPOLLIN/EPOLLOUT)
3. **EPOLLIN**: `handleClientRequest()` → `Client::appendData()` → parses headers/body → sets EPOLLOUT when complete
4. **EPOLLOUT**: `handleClientResponse()` or `Client::handleCGI()` → streams response → closes connection when done
5. **Timeouts**: `handleTimeOut()` checks `REQUEST_TIMEOUT`/`RESPONSE_TIMEOUT`/`CGI_TIMEOUT` constants

## Critical Implementation Details

### Config Parsing (`main.cpp`)
- **Tokenizer**: Splits config into tokens, removing `#` comments and isolating semicolons
- **Parser**: State machine tracking `inServer`/`inLocation` flags, builds `Server` → `Location` hierarchy
- Syntax: `server { listen IP:PORT; location /path { root ...; methods GET POST; cgi .py /usr/bin/python3; } }`
- Default config: `config_files/configFile1.txt` (pass custom path as argv[1])

### Request Handling (`Client.cpp`)
**Three-stage parsing:**
1. **Headers**: Accumulate until `\r\n\r\n`, call `Request::parseRequest()` → validates method/URI/protocol
2. **Body** (POST): 
   - `oneBody=true`: Simple body (`appendBody()` writes to temp file)
   - `oneBody=false`: Multipart form-data with `boundary` parsing → `handleBodyHeaders()` extracts filename → `handleInBody()` writes chunks
3. **Complete**: `handleCompleteRequest()` → route matching → method dispatch

**Route Matching** (`findBestMatch()`):
- Longest prefix match on `Location::_path`
- Returns Location with `root`, `index`, `methods`, `cgi`, `autoindex`, `upload_store`, `_http_redirection`

### Method Implementation
- **GET** (`srcs/GET.cpp`): 
  - Redirections: `handleRedirection()` sends 301/302 with HTML body
  - Directories: Check `index` files, fallback to `listingDirectory()` if `autoindex on`
  - Files: Open `std::ifstream`, stream in chunks via `handleFile()` (called in EPOLLOUT loop)
- **POST** (`srcs/POST.cpp`): Multipart boundary parsing, writes to `upload_store` directory
- **DELETE** (`srcs/DELETE.cpp`): Recursive directory deletion or file unlink (checks `allowedDelete()`)

### CGI Execution (`CGIHandler.cpp`)
1. `Client::handleCGI()` creates CGIHandler, calls `startCGI()` with script path + environment map
2. `startCGI()`: Creates `in_fd`/`out_fd` pipes → `fork()` → child execs script with `PATH_INFO`, `QUERY_STRING`, etc.
3. **Non-blocking I/O**: Parent writes request body to `in_fd[1]`, reads response from `out_fd[0]` via epoll
4. **Timeout**: `isCgiTimedOut()` checks `_startTime` vs `CGI_TIMEOUT` (5s)
5. **Cleanup**: `cleanupCGI()` closes pipes, `waitpid()` reaps child

**CGI Context Management** (`Server.hpp`):
- `CGIContext` struct tracks `childpid`, `pipe_to_cgi`, `pipe_from_cgi`, `Client*`, buffers
- Stored in `Server::CGIstdIn`/`CGIstdOut` maps (keyed by pipe FDs) for epoll event correlation

## Build & Run

```bash
make              # Compiles webserv (C++98, fsanitize=address enabled)
./webserv [config_file]   # Defaults to config_files/configFile1.txt
```

**Test CGI Scripts**: `cgi-bin/` contains `.py`/`.php` scripts (ensure interpreters in config match system paths)

**Error Pages**: Served from `errors/` directory (400.html, 403.html, 404.html, etc.)

## Code Conventions

- **C++98 compliance**: No `nullptr`, use `.c_str()`, typedef iterators
- **Non-blocking I/O**: All sockets set via `setNonBlocking()` (fcntl O_NONBLOCK)
- **Error handling**: `throw_exception()` wraps errno messages, caught in main loop
- **Memory management**: Manual `new`/`delete` for Clients in `std::map<int, Client*>`, cleaned up on errors/timeouts
- **Header guards**: `#ifndef CLASSNAME_HPP` pattern
- **File structure**: Classes split into `includes/*.hpp` and `srcs/*.cpp` (except `main.cpp` at root)

## Common Pitfalls

1. **Pipe closure**: CGI scripts must receive EOF (`close(in_fd[1])`) to finish reading stdin
2. **Boundary parsing**: Multipart bodies need safe buffering (keep `boundary.length()` bytes unparsed until next chunk)
3. **Path normalization**: `Request::normalizePath()` resolves `..` and `.` to prevent directory traversal
4. **Response chunking**: `handleFile()` sends partial file chunks (don't load entire file into memory)
5. **Timeout granularity**: epoll_wait uses 1000ms timeout, so timeouts checked once per second
6. **Listen FD detection**: `listening_fd()` checks `servers_fd` map to distinguish server sockets from client sockets

## Key Files to Modify

- **Add HTTP method**: Edit `Client.cpp` → add handler, update `allowedMethod()`
- **New config directive**: Extend `parser()` in `main.cpp`, add Location/Server member
- **Change I/O model**: Replace epoll calls in `run_server.cpp` (current FD maps assume epoll)
- **Custom error pages**: Add HTML to `errors/`, reference in config `error_page CODE /path`

## Testing

No automated tests included. Manual testing workflow:
1. Start server: `./webserv`
2. `curl http://127.0.0.1:8080/` (GET index)
3. `curl -X POST -F "file=@test.txt" http://127.0.0.1:8080/upload` (file upload)
4. `curl http://127.0.0.1:8080/cgi-bin/test.py` (CGI execution)
5. Check timeouts: Send incomplete request, wait 50s for `REQUEST_TIMEOUT`
