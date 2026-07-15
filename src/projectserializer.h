// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectserializer.h
/// \brief Declares the project XML serializer.
///

#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include <QStringList>
#include "connectiondetails.h"
#include "modbusdefinitions.h"

class QIODevice;
class QXmlStreamWriter;
class AppProject;
class DataSimulator;
class MainWindow;
class MdiArea;
class MdiAreaEx;
class ModbusMultiServer;

///
/// \brief The ProjectSerializer class streams the project XML: it restores forms,
/// view state and address-space data on load and writes them back on save.
/// It owns no state between calls; global application state (connections,
/// preferences, pending window activation) is returned in LoadResult and applied
/// by AppProject.
///
class ProjectSerializer
{
public:
    ///
    /// \brief The LoadResult struct carries the project-global settings parsed on
    /// load that AppProject applies after the forms are restored.
    ///
    struct LoadResult
    {
        ModbusDefinitions Definitions;
        QList<ConnectionDetails> Connections;
        bool SplitView = false;
        bool HasGlobalZeroBasedAddress = false;
        bool GlobalZeroBasedAddress = false;
        bool HasGlobalHexView = false;
        bool GlobalHexView = false;
        QStringList PrimaryTabOrder;
        QStringList SecondaryTabOrder;
        QString ActivePrimaryWindow;
        QString ActiveSecondaryWindow;
        QString ActivePanel;
    };

    ProjectSerializer(AppProject& project,
                      ModbusMultiServer& mbServer,
                      DataSimulator* dataSimulator,
                      MdiAreaEx* mdiArea,
                      MainWindow* mainWindow);

    LoadResult load(QIODevice& device, bool replace);
    bool save(QIODevice& device);

private:
    void saveOpenFormsFromArea(QXmlStreamWriter& w, MdiArea* area, const char* panel, bool autoClonesOnly);
    void writeTabOrder(QXmlStreamWriter& w, MdiArea* area, const char* panel);

private:
    AppProject& _project;
    ModbusMultiServer& _mbServer;
    DataSimulator* _dataSimulator;
    MdiAreaEx* _mdiArea;
    MainWindow* _mainWindow;
};

#endif // PROJECTSERIALIZER_H
