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

    QCommandLineOption noSessionOption(QStringList() << _no_session, tr("Do not use program session."));
    addOption(noSessionOption);

    addPositionalArgument("project", tr("Project file to open."), tr("[project]"));
}

///
/// \brief CmdLineParser::projectFile
/// \return
///
QString CmdLineParser::projectFile() const
{
    const auto args = positionalArguments();
    return args.isEmpty() ? QString() : args.first();
}

