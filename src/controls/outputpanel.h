// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file outputpanel.h
/// \brief Declares the outputpanel interfaces.
///

#ifndef OUTPUTPANEL_H
#define OUTPUTPANEL_H

#include <QWidget>

namespace Ui {
class OutputPanel;
}

class AppLogOutput;
class ConsoleOutput;

///
/// \brief The OutputPanel class вЂ” tabbed bottom panel (Log + JavaScript Console)
///
class OutputPanel : public QWidget
{
    Q_OBJECT
public:
    explicit OutputPanel(QWidget* parent = nullptr);
    ~OutputPanel();

    AppLogOutput* appLog() const;
    ConsoleOutput* jsConsole() const;

    void switchToAppLog();
    void switchToJsConsole();

signals:
    void collapse();

protected:
    void changeEvent(QEvent* event) override;

private:
    Ui::OutputPanel* ui;
};

#endif // OUTPUTPANEL_H

