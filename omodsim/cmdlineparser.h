#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <QCommandLineParser>

class CmdLineParser : public QCommandLineParser
{
public:
    explicit CmdLineParser();

public:
    static constexpr const char* _help =    "help";
    static constexpr const char* _version = "version";
    static constexpr const char* _config =  "config";
};

#endif // CMDLINEPARSER_H
