#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>
#include <map>
#include <functional>

typedef std::map<std::string, std::string> headers_t;

struct request_t
{
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    headers_t headers;
    std::map<std::string, std::string> params;
};

struct response_t
{
    std::string status_text;
    int status_code = 0;
    std::string body;
    headers_t headers;
};

class Router
{
private:
    std::map<std::string, std::function<void(request_t &, response_t &)>> routes;

    Router();

public:
    static Router &getInstance();

public:
    void addRoute(const std::string &path, std::function<void(request_t &, response_t &)> handler);

    std::string getResponse(request_t &request);

private:
    response_t handleRequest(request_t &request);

    std::string buildResponseString(response_t &response);

    std::vector<std::string> splitPath(std::string path);
};

#endif