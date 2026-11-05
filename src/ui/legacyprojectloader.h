// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file legacyprojectloader.h
/// \brief Declares helpers for loading legacy project file formats.
///

#ifndef LEGACYPROJECTLOADER_H
#define LEGACYPROJECTLOADER_H

#include <QString>

class AppProject;
class FormDataView;
class QXmlStreamReader;

///
/// \brief The LegacyProjectLoader class isolates project file compatibility code.
///
class LegacyProjectLoader
{
public:
    static bool isDataViewElement(const QString& name) noexcept;
    static void loadDataView(QXmlStreamReader& xml, FormDataView& form, AppProject& project);
};

#endif // LEGACYPROJECTLOADER_H
