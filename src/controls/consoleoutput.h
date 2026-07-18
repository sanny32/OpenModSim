// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file consoleoutput.h
/// \brief Declares the consoleoutput interfaces.
///

#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QVector>
#include <QWidget>

class QTimer;

namespace Ui {
class ConsoleOutput;
}

///
/// \brief The ConsoleOutput class
///
class ConsoleOutput : public QWidget
{
    Q_OBJECT
public:
    enum class MessageType { Log, Debug, Warning, Error };

    explicit ConsoleOutput(QWidget* parent = nullptr);
    ~ConsoleOutput();

    void addMessage(const QString& text, MessageType type, const QString& source = {});
    void flush();
    bool isEmpty() const;
    void setMaxLines(int n);

public slots:
    void clear();

signals:
    void collapse();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_customContextMenuRequested(const QPoint& pos);
    void on_flushTimeout();
    void applyFilters();

private:
    void updateFilterButtons();
    void flushChunk();
    void insertMessage(const QString& text, MessageType type, const QString& source);
    void evictOverflow();

private:
    struct PendingMessage {
        QString text;
        MessageType type;
        QString source;
    };

    Ui::ConsoleOutput* ui;
    QTimer* _flushTimer;
    QVector<PendingMessage> _pending;
    int _logCount   = 0;
    int _warnCount  = 0;
    int _errorCount = 0;
    int _maxLines   = 500;
};

#endif // CONSOLEOUTPUT_H

