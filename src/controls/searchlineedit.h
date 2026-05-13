// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file searchlineedit.h
/// \brief Declares the searchlineedit interfaces.
///

#ifndef SEARCHLINEEDIT_H
#define SEARCHLINEEDIT_H

#include <QLineEdit>

///
/// \brief The SearchLineEdit class
///
class SearchLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchLineEdit(QWidget* parent = nullptr);

signals:
    void searchText(const QString& text);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_returnPressed();
    void on_textEdited(const QString& text);
};

#endif // SEARCHLINEEDIT_H

