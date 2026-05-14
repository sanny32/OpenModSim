// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file helpbrowser.h
/// \brief Declares the HelpBrowser class.
///

#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QSharedPointer>
#include <QTextBrowser>
#include <QHelpEngine>

///
/// \brief The HelpBrowser class
///
class HelpBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit HelpBrowser(QWidget* parent = nullptr);

    void setHelp(const QString& helpFile);
    void showHelp(const QString& helpKey);

    QVariant loadResource(int type, const QUrl& name) override;

public slots:
    void reload() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void applyStylesheet();

private:
    QSharedPointer<QHelpEngine> _helpEngine;
};

#endif // HELPBROWSER_H
