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

    QCommandLineOption profileOption(QStringList() << _profile, tr("Load settings profile from ini file."), tr("file path"));
    addOption(profileOption);

    QCommandLineOption configOption(QStringList() << _config, tr("Setup test config file."), tr("file path"));
    addOption(configOption);

    QCommandLineOption noSessionOption(QStringList() << _no_session, tr("Do not use program session."));
    addOption(noSessionOption);
}
