# HTTP Server from scratch written in c
This is a personal project that I started during the winter break, the goal is to learn more about linux, network, filesystem, etc..., and also learn c.

### What?
This project is a lightweight HTTP/1.1 web server written in pure C. It is designed to handle multiple simultaneous client connections efficiently using a combination of non-blocking I/O, epoll, and multithreading.

## Performance
You can check out more in detailed on how this server has been built and the performances (compared with nginx on the same machine) here: [docs.pdf](https://github.com/user-attachments/files/26648074/docs.pdf)



## Build
Requires:
- Linux or macOS
- GCC or Clang
- POSIX environment

Compile everything with
```gcc -O2 -g -fno-omit-frame-pointer -pthread src/**/*.c -Iinclude -lmagic -o my_server  ```

By default the server listens on `http://localhost:8080`


## Architecture Overview
The server is split into layers:
- OS
    - Read/Write raw bytes
- TCP  
    - Accepts connections
- Buffer
    - Manage partial reads-write safely
- HTTP parser
    - Converts raw bytes into `http_request_t`
    - Parse request line
- Router
    - Decides how to handle requestlayer
    - Static file vs 404 (and later I hope, api)


## Limitations
- no HTTPS 
- No chunked transfer encoding 
- ~~no keep-alive (connection are closed after the  response)~~
- ~~Single threaded, blocking I/O~~


## Possible improvements
- ~~HTTP headers parsing~~
- ~~POST request & body parsing~~
- ~~keep-alive support~~
- ~~`epoll` / non-blocking I/O~~
- ~~thread pool~~
- Directory listing
- Basic api endpoints
- IPv6 support 
- Security hardening (needed)
