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
#include "settings.hpp"

struct headers_t
{
  std::string user_agent;
  std::string accept;
  std::string host;
  std::string content_type;
  uint16_t content_length;
};

struct request_t
{
  std::string method;
  std::string path;
  std::string version;
  headers_t headers;
};

std::string get_response(request_t &req);
void handle_request(int client);
void extract_request(char *buffer, request_t &req);

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

    std::thread th(handle_request, client);
    th.detach();
  }

  close(server_fd);

  return 0;
}

void handle_request(int client)
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
  extract_request(buffer, req);

  std::string response = get_response(req);

  send(client, response.c_str(), response.length(), 0);
}

void extract_request(char *buffer, request_t &req)
{
  headers_t req_header;

  std::string str(buffer);
  size_t pos = str.find("\r\n");
  std::string req_data = str.substr(0, pos);
  std::string header_data = str.substr(pos + 2);

  size_t first_space = req_data.find(' ');
  size_t last_space = req_data.rfind(' ');

  req.method = req_data.substr(0, first_space);

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
  req.headers = req_header;

  size_t user_agent_pos = header_data.find("User-Agent");
  if (user_agent_pos != std::string::npos)
  {
    req.headers.user_agent = header_data.substr(user_agent_pos + 12);
    req.headers.user_agent = req.headers.user_agent.substr(0, req.headers.user_agent.find('\r'));
  }
  else
  {
    req.headers.user_agent = "";
  }
}

std::string get_response(request_t &req)
{
  try
  {
    std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";

    if (req.path == "/")
    {
      response = "HTTP/1.1 200 OK\r\n\r\n";
    }
    else if (req.path.substr(1, req.path.substr(1).find('/')) == "echo")
    {
      // Extract the string to echo from the path
      std::string echo_string = req.path.substr(req.path.substr(1).find('/') + 2);

      char buffer[1024];
      sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:%zu\r\n\r\n%s", echo_string.length(), echo_string.c_str());
      response = buffer;
    }
    else if (req.path.substr(1, req.path.substr(1).find('/')) == "user-agent")
    {
      char buffer[1024];
      sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:%zu\r\n\r\n%s", req.headers.user_agent.length(), req.headers.user_agent.c_str());
      response = buffer;
    }
    else if (req.path.substr(1, req.path.substr(1).find('/')) == "files")
    {
      std::string file_path = settings.directory + req.path.substr(req.path.find('/') + 7);
      FILE *file = fopen(file_path.c_str(), "r");
      if (file)
      {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_buffer = new char[file_size + 1];
        fread(file_buffer, 1, file_size, file);
        fclose(file);
        file_buffer[file_size] = '\0';

        char buffer[file_size + 128];

        sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:%zu\r\n\r\n%s", file_size, file_buffer);
        response = buffer;

        delete[] file_buffer;
      }
      else
      {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
    }

    return response;
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
  }
}