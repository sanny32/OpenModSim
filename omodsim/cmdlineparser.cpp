#include "cmdlineparser.h"

///
/// \brief CmdLineParser::CmdLineParser
///
CmdLineParser::CmdLineParser()
    : QCommandLineParser()
{
    QCommandLineOption helpOption(QStringList() << _help, tr("Displays this help."));
    addOption(helpOption);

    QCommandLineOption versionOption(QStringList() << _version, tr("Displays version information."));
    addOption(versionOption);

    QCommandLineOption configOption(QStringList() << _config, tr("Setup test config file."), tr("file path"));
    addOption(configOption);
}
