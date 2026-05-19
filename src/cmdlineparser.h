// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file cmdlineparser.h
/// \brief Declares the cmdlineparser interfaces.
///

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
    static constexpr const char* _no_session = "no-session";

    QString projectFile() const;

};

#endif // CMDLINEPARSER_H

