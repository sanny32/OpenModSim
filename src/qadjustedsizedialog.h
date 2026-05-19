// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file qadjustedsizedialog.h
/// \brief Declares the qadjustedsizedialog interfaces.
///

#ifndef QADJUSTEDSIZEDIALOG_H
#define QADJUSTEDSIZEDIALOG_H

#include <QDialog>

///
/// \brief The QAdjustedSizeDialog class
///
class QAdjustedSizeDialog : public QDialog
{
    Q_OBJECT
public:
    QAdjustedSizeDialog(QWidget *parent = nullptr, Qt::WindowFlags f =
                                                Qt::Dialog |
                                                Qt::CustomizeWindowHint |
                                                Qt::WindowCloseButtonHint |
                                                Qt::WindowTitleHint);

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent* e) override;

private:
    bool _shown = false;
};

#endif // QADJUSTEDSIZEDIALOG_H

