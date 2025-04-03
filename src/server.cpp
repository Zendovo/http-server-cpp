#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <fstream>
#include <sstream>
#include "settings.hpp"
#include "request_handler.hpp"
#include "routes/register.hpp"

void handleRequest(int client, Router &router);
void extractRequest(char *buffer, request_t &req);
headers_t extractHeaders(const std::string request);

MySettings settings;

int main(int argc, char **argv)
{
  // Parse command line arguments
  settings = parse_settings(argc, argv);
  if (settings.verbose)
  {
    std::cout << "Verbose mode is enabled.\n";
    std::cout << "Directory: " << settings.directory << "\n";
  }

  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  Router &router = Router::getInstance();
  registerRoutes(router);
  registerPostProcessors(router);

  while (1)
  {
    // Accept a new client connection
    int client = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    std::cout << "Client connected\n";
    if (client < 0)
    {
      std::cerr << "Failed to accept client connection\n";
      return 1;
    }

    std::thread th(handleRequest, client, std::ref(router));
    th.detach();
  }

  close(server_fd);

  return 0;
}

void handleRequest(int client, Router &router)
{
  char buffer[1024];
  ssize_t bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received < 0)
  {
    std::cerr << "Failed to receive data from client\n";
    close(client);
    return;
  }
  buffer[bytes_received] = '\0';

  request_t req;
  extractRequest(buffer, req);

  std::string response = router.getResponse(req);

  send(client, response.c_str(), response.length(), 0);
}

void extractRequest(char *buffer, request_t &req)
{
  headers_t req_header;

  std::string str(buffer);
  size_t pos = str.find("\r\n");
  std::string req_data = str.substr(0, pos);
  std::string header_data = str.substr(pos + 2);

  size_t first_space = req_data.find(' ');
  size_t last_space = req_data.rfind(' ');

  // Request Method
  req.method = req_data.substr(0, first_space);

  // Request Path and Version
  if (first_space != std::string::npos && last_space != std::string::npos && first_space != last_space)
  {
    req.path = req_data.substr(first_space + 1, last_space - first_space - 1);
    req.version = req_data.substr(last_space + 1);
  }
  else
  {
    // Handle the case where there are not enough spaces
    req.path = "/";
    req.version = "HTTP/1.1";
  }

  // Headers
  req.headers = extractHeaders(str);

  // Request Body
  size_t l_pos = str.rfind("\r\n");
  std::string body = str.substr(l_pos + 2);
  req.body = body;
}

headers_t extractHeaders(const std::string request)
{
  headers_t headers;
  std::istringstream stream(request);
  std::string line;

  while (std::getline(stream, line) && line != "\r")
  {
    size_t pos = line.find(": ");
    if (pos != std::string::npos)
    {
      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 2);
      headers[key] = value;
    }
  }

  return headers;
}