# Codecrafters HTTP Server (C++)

This project is a custom HTTP server implemented in C++. Below is an overview of the program's structure and key components, including the router, threading for concurrent requests, GZIP implementation, and post-processors.

## Project Structure

```
codecrafters-http-server-cpp/
├── src/
│   ├── request_handler.cpp         # Router implementation
│   ├── server.cpp                  # Core server logic
│   ├── settings.cpp                # For flags
│   └── routes
│     └── register.cpp              # Routes and Postprocessors
└── README.md                       # Project documentation
```

## Router Implementation

The router is responsible for mapping HTTP requests to specific handlers based on the request's method and path. It uses a map to store routes and their corresponding handler functions. The router supports dynamic route parameters (e.g., `/users/:id`) and resolves them at runtime.

### Key Features:
- **Route Registration**: Routes are registered with their HTTP method and path.
- **Dynamic Parameters**: Extracts parameters from the URL and passes them to the handler.

### Example:
```cpp
router.addRoute("GET", "/users/:id", [](request_t &req, response_t &res) {
   res.body = "User ID: " + req.params["id"];
});
```

## Threading for Concurrent Requests

The server uses threads to handle multiple client requests concurrently.

## GZIP Implementation

The server supports GZIP compression to reduce the size of HTTP responses. The `register.cpp` file registers a compression post-processor to compress response data using the zlib library.

### Key Features:
- **Compression on Demand**: Responses are compressed if the client supports GZIP (via the `Accept-Encoding` header).
- **Efficient Compression**: Uses zlib's deflate algorithm for optimal performance.
- **Transparent Integration**: Compression is applied automatically in the response pipeline.

## Post-Processors

Post-processors are middleware functions that modify the response before it is sent to the client. They can be used for tasks like logging, adding headers, or transforming response data.

### Key Features:
- **Chaining**: Multiple post-processors can be applied in sequence.
- **Custom Logic**: Developers can define custom post-processors for specific use cases.
- **Integration**: Post-processors are invoked after the main handler generates the response.

## Conclusion

This HTTP server demonstrates a modular and efficient design, supporting routing, concurrency, compression, and response post-processing. Each component is designed to be reusable and extensible, making it a robust foundation for building web applications.
