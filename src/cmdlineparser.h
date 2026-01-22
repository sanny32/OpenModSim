#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <QCommandLineParser>

///
/// \brief The CmdLineParser class
///
class CmdLineParser : public QObject, public QCommandLineParser
{
    Q_OBJECT

public:
    explicit CmdLineParser();

public:
    static constexpr const char* _help =    "help";
    static constexpr const char* _version = "version";
    static constexpr const char* _profile =  "profile";
    static constexpr const char* _config =  "config";
    static constexpr const char* _no_session = "no-session";
};

#endif // CMDLINEPARSER_H
