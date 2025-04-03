#include "../request_handler.hpp"
#include "../settings.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <zlib.h>
#include <vector>
#include <cstring>

extern MySettings settings;
std::string compress_gzip(const std::string &data);

void registerRoutes(Router &router)
{
    router.addRoute("/", [](request_t &request, response_t &response)
                    {
        response.status_code = 200;
        response.status_text = "OK"; });

    router.addRoute("/echo/:data", [](request_t &request, response_t &response)
                    {
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] =  std::to_string(request.params["data"].length());
        response.body = request.params["data"]; });

    router.addRoute("/user-agent", [](request_t &request, response_t &response)
                    {
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = "text/plain";
        response.headers["Content-Length"] = std::to_string(request.headers["User-Agent"].length() - 1);
        response.body = request.headers["User-Agent"]; });

    router.addRoute("/files/:filename", [](request_t &request, response_t &response)
                    {
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
        } });
}

void registerPostProcessors(Router &router)
{
    router.addPostProcessor([](request_t &request, response_t &response)
                            {
        if (request.headers.find("Accept-Encoding") != response.headers.end())
    {
        if (request.headers["Accept-Encoding"].find("gzip") != std::string::npos)
        {
            response.headers["Content-Encoding"] = "gzip";
        }
    } });

    router.addPostProcessor([](request_t &request, response_t &response)
                            {
        if (response.headers.find("Content-Encoding") != response.headers.end())
        {
            std::ostringstream compressed;
            std::ostringstream uncompressed(response.body);
            std::string compressed_data = compress_gzip(uncompressed.str());
            response.body = compressed_data;
            response.headers["Content-Length"] = std::to_string(compressed_data.length());
            response.headers["Content-Type"] = "application/gzip";
        } });
}

std::string compress_gzip(const std::string &data)
{
    // Placeholder for gzip compression logic
    // In a real implementation, you would use a library like zlib to compress the data
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        throw(std::runtime_error("deflateInit2 failed while compressing."));
    }

    zs.next_in = (Bytef *)data.data();
    zs.avail_in = data.size();

    int ret;
    char outbuffer[32768];
    std::vector<unsigned char> out_vector;

    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (ret < 0)
        {
            deflateEnd(&zs);
            throw(std::runtime_error("deflate failed while compressing."));
        }

        int have = sizeof(outbuffer) - zs.avail_out;
        out_vector.insert(out_vector.end(), outbuffer, outbuffer + have);
    } while (zs.avail_out == 0);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END)
    {
        throw(std::runtime_error("deflate should have returned Z_STREAM_END."));
    }

    return std::string(out_vector.begin(), out_vector.end());
}