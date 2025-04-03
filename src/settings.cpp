#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace std;

struct MySettings
{
    string directory;
    bool verbose{true};
};

typedef function<void(MySettings &)> NoArgHandle;
typedef function<void(MySettings &, const string &)> OneArgHandle;

const unordered_map<string, NoArgHandle> NoArgs{
    {"--quiet", [](MySettings &s)
     { s.verbose = false; }},
};

const unordered_map<string, OneArgHandle> OneArgs{
    // Writing out the whole lambda
    {"--directory", [](MySettings &s, const string &arg)
     {
         s.directory = arg;
     }}};

MySettings parse_settings(int argc, char *argv[])
{
    MySettings settings;

    for (int i{1}; i < argc; i++)
    {
        string opt{argv[i]};

        if (auto j{NoArgs.find(opt)}; j != NoArgs.end())
            j->second(settings);

        // No, how about a OneArg?
        else if (auto k{OneArgs.find(opt)}; k != OneArgs.end())
            // Yes, do we have a parameter?
            if (++i < argc)
                // Yes, handle it!
                k->second(settings, {argv[i]});
            else
                // No, and we cannot continue, throw an error
                throw std::runtime_error{"missing param after " + opt};
        else
            cerr << "unrecognized command-line option " << opt << endl;
    }

    return settings;
}