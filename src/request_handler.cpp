#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "request_handler.hpp"
#include <functional>

Router::Router() {}

Router &Router::getInstance()
{
    static Router instance;
    return instance;
}

void Router::addRoute(const std::string &path, std::function<void(request_t &, response_t &)> handler)
{
    routes[path] = handler;
}

void Router::addPostProcessor(std::function<void(request_t &, response_t &)> post_processor)
{
    post_processors.push_back(post_processor);
}

std::string Router::getResponse(request_t &request)
{
    response_t response = handleRequest(request);
    for (auto &post_processor : post_processors)
    {
        post_processor(request, response);
    }
    return buildResponseString(response);
}

response_t Router::handleRequest(request_t &request)
{
    response_t response;

    std::vector<std::string> requestParts = splitPath(request.path);

    for (auto &[route, handler] : routes)
    {
        std::vector<std::string> routeParts = splitPath(route);

        if (requestParts.size() != routeParts.size())
        {
            continue;
        }

        bool match = true;

        for (size_t i = 0; i < requestParts.size(); ++i)
        {
            if (routeParts[i] != requestParts[i] && routeParts[i][0] != ':')
            {
                match = false;
                break;
            }
            else if (routeParts[i][0] == ':')
            {
                request.params[routeParts[i].substr(1)] = requestParts[i];
            }
        }

        if (match)
        {
            handler(request, response);
            return response;
        }
    }

    response.status_code = 404;
    response.status_text = "Not Found";

    return response;
}

std::string Router::buildResponseString(response_t &response)
{
    std::string response_string;
    response_string += "HTTP/1.1 " + std::to_string(response.status_code) + " " + response.status_text + "\r\n";

    for (const auto &[key, value] : response.headers)
    {
        response_string += key + ": " + value + "\r\n";
    }

    if (!response.body.empty())
    {
        response_string += "\r\n" + response.body;
    } else {
        response_string += "\r\n";
    }
    return response_string;
}

std::vector<std::string> Router::splitPath(std::string path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            parts.push_back(part);
        }
    }
    return parts;
}