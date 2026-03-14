# HTTP Server from scratch written in c
This is a personal project that I started during the winter break, the goal is to learn more about linux, network, filesystem, etc..., and also learn c.

### What?
This project is a lightweight HTTP/1.1 web server written in pure C. It is designed to handle multiple simultaneous client connections efficiently using a combination of non-blocking I/O, epoll, and multithreading.

The server follows a worker-based architecture:
- A main thread accepts incoming TCP connections
- Connections are dispatched to a worker threads
- Each worker run its own epoll event loop and handles all I/O for its assigned connections



## Build
Requires:
- Linux or macOS
- GCC or Clang
- POSIX environment

Compile everything with
```gcc -pthread src/**/*.c -Iinclude -lmagic -o my_server  ```

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



### Current improvements
The goal of this section is just for me to "store" the result I have in comparaison to a nginx server.
I added a startup-time static file cache that preloads metadata for files under www/, avoiding repeated path resolution, file opening, and fstat on every request. On the 100k.bin benchmark with wrk -t8 -c400 -d30s, throughput improved from about 124k req/s to 143k req/s, a gain of roughly 16%.

To have a better understanding on what this means, nginx on my machine using the same file is doing 29k Req/seq, this webserver is currently doing 18k Req/seq (using wrk as a benchmarker)
```wrk -t8 -c400 -d30s http://127.0.0.1:8081/100k.bin```
