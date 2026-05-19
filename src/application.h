// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application interfaces.
///

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include "styles/apptheme.h"

class Application final
    : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int& argc, char** argv);

    AppTheme& theme();
    const AppTheme& theme() const;

    static Application* instance();

    QString takePendingFile();

signals:
    void fileOpenRequested(const QString& filePath);

protected:
    bool event(QEvent* event) override;

private:
    AppTheme _theme;
    QString _pendingFile;
};

inline Application* theApp()
{
    return static_cast<Application*>(qApp);
}

#endif // APPLICATION_H
