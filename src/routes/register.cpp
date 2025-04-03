#include "../request_handler.hpp"
#include "../settings.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

extern MySettings settings;

void registerRoutes(Router &router)
{
    router.addRoute("/", [](request_t &request, response_t &response) {
        response.status_code = 200;
        response.status_text = "OK";
    });

    router.addRoute("/echo/:data", [](request_t &request, response_t &response) {
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] =  std::to_string(request.params["data"].length());
        response.body = request.params["data"];
    });

    router.addRoute("/user-agent", [](request_t &request, response_t &response) {
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(request.headers["User-Agent"].length() - 1);
        response.body = request.headers["User-Agent"];
    });

    router.addRoute("/files/:filename", [](request_t &request, response_t &response) {
        if (request.method == "GET")
        {
            std::ifstream i_file(settings.directory + request.params["filename"], std::ios::binary | std::ios::ate);
            if (!i_file)
            {
                response.status_code = 404;
                response.status_text = "Not Found";
                return;
            }
            long file_size = i_file.tellg();
            i_file.seekg(0, std::ios::beg);

            std::string file_data(file_size, '\0');
            i_file.read(&file_data[0], file_size);

            response.status_code = 200;
            response.status_text = "OK";
            response.headers["Content-Type"] = "application/octet-stream";
            response.headers["Content-Length"] = std::to_string(file_size);
            response.body = file_data;
        }
        else if (request.method == "POST")
        {
            std::ofstream o_file(settings.directory + request.params["filename"]);
            o_file << request.body;
            o_file.close();

            response.status_code = 201;
            response.status_text = "Created";
        }
    });
}