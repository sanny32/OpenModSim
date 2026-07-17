// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectloadresult.h
/// \brief Declares project document validation and loading results.
///

#ifndef PROJECTLOADRESULT_H
#define PROJECTLOADRESULT_H

#include <QString>

struct ProjectLoadResult
{
    bool Success = false;
    QString Error;
};

#endif // PROJECTLOADRESULT_H
