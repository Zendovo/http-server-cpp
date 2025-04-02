#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

struct headers_t {
  std::string user_agent;
  std::string accept;
  std::string host;
  std::string content_type;
  uint16_t content_length;
};

struct request_t {
  std::string method;
  std::string path;
  std::string version;
  headers_t headers;
};

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";
  if (client < 0) {
    std::cerr << "Failed to accept client connection\n";
    return 1;
  }
  
  char buffer[1024];
  ssize_t bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received < 0) {
    std::cerr << "Failed to receive data from client\n";
    close(client);
    return 1;
  }
  buffer[bytes_received] = '\0'; // Null-terminate the received data
  std::cout << "Received data from client:\n" << buffer << "\n";

  request_t req;
  headers_t req_header;

  std::string str(buffer);
  size_t pos = str.find('\r\n');
  std::string req_data = str.substr(0, pos);
  std::string header_data = str.substr(pos + 1, str.rfind('\r\n') - 1);

  req.method = req_data.substr(0, req_data.find(' '));
  req.path = req_data.substr(req_data.find(' ') + 1, req_data.rfind(' ') - req_data.find(' ') - 1);
  req.version = req_data.substr(req_data.rfind(' ') + 1);
  req.headers = req_header;

  req.headers.user_agent = header_data.substr(header_data.find("User-Agent") + 12);
  req.headers.user_agent = req.headers.user_agent.substr(0, req.headers.user_agent.find('\r'));

  std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";

  if (req.path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (req.path.substr(1, req.path.substr(1).find('/')) == "echo") {
    // Extract the string to echo from the path
    std::string echo_string = req.path.substr(req.path.substr(1).find('/') + 2);

    char buffer[1024];
    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:%zu\r\n\r\n%s", echo_string.length(), echo_string.c_str());
    response = buffer;
  } else if (req.path.substr(1, req.path.substr(1).find('/')) == "user-agent") {
    char buffer[1024];
    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:%zu\r\n\r\n%s", req.headers.user_agent.length(), req.headers.user_agent.c_str());
    response = buffer;
  }
  
  send(client, response.c_str(), response.length(), 0);
  
  close(server_fd);

  return 0;
}
