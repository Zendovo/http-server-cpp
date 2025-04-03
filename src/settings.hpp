#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>

struct MySettings
{
    std::string directory {"/tmp/"};
    bool verbose{true};
};

MySettings parse_settings(int argc, char *argv[]);

#endif
