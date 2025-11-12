#ifndef JSCRIPTCONTROL_H
#define JSCRIPTCONTROL_H

#include <QTimer>
#include <QJSEngine>
#include <QPlainTextEdit>
#include <QXmlStreamWriter>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"

namespace Ui {
class JScriptControl;
}

///
/// \brief The JScriptControl class
///
class JScriptControl : public QWidget
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, const JScriptControl* ctrl);
    friend QSettings& operator >>(QSettings& in, JScriptControl* ctrl);

    friend QDataStream& operator <<(QDataStream& out, const JScriptControl* ctrl);
    friend QDataStream& operator >>(QDataStream& in, JScriptControl* ctrl);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const JScriptControl* ctrl);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, JScriptControl* ctrl);

public:  
    explicit JScriptControl(QWidget *parent = nullptr);
    ~JScriptControl();

    void setModbusMultiServer(ModbusMultiServer* server);
    void setByteOrder(const ByteOrder* order);
    void setAddressBase(AddressBase base);

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

    QString script() const;
    void setScript(const QString& text);

    QString searchText() const;

    bool isRunning() const;

    void setFocus();

    bool canUndo() const;
    bool canRedo() const;
    bool canPaste() const;

public slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void search(const QString& text);
    void runScript(RunMode mode, int interval = 0);
    void stopScript();
    void showHelp(const QString& helpKey);

private slots:
    bool executeScript();

private:
    QJSValue newEnumObject(const QMetaObject& metaObj, const QString& enumName);

private:
    Ui::JScriptControl *ui;

    QTimer _timer;
    QJSEngine _jsEngine;
    QString _scriptCode;
    QString _searchText;

    QSharedPointer<Script> _script;
    QSharedPointer<Storage> _storage;
    QSharedPointer<Server> _server;
    QSharedPointer<console> _console;

    ByteOrder* _byteOrder = nullptr;
    AddressBase _addressBase = AddressBase::Base1;
    ModbusMultiServer* _mbMultiServer = nullptr;
};

#endif // JSCRIPTCONTROL_H
