# wcio - Websocket library, implemented in C

## Description 
wcio library is implemented according to RFC 6455. It may provide as high level access to sending the messages through websocket, and the low level access, which is building your own frame manually and then converting it, sending to the server. The library is still in the early development, and may contain some bugs and unimplemented features. Planning to be extended in the future. 

## Plans for the future:
- Implement wcio_send_binary 
- Improve the error handling with ws_status in multiple other parts of the code 
- Add more examples 
- Add parallelism 
- Port to other systems 

## Download and build
1. Clone the repository:
```bash
git clone && cd wcio
```
2. Build the library:
```bash
make
```
3. Install the library:
```bash
make install
```
4. Build examples:
```
make examples
```

# Notes
- The library is currently built and developed for macOS x86_64 only, in future exist plans to port it into Linux and BSD. 
- Some parts of the code may segfault/work unexpectedly on ARM or other architectures
