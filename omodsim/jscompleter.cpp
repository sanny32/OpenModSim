#include <QWidget>
#include <QMetaProperty>
#include <QStringListModel>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"
#include "jscompleter.h"

struct MethodMetaInfo
{
    QString Name;
    bool IsProperty;
};

QMap<QString, QVector<MethodMetaInfo>> _completerMap;

///
/// \brief addMetaObject
/// \param metaObject
///
void addMetaObject(const QMetaObject& metaObject)
{
    QVector<MethodMetaInfo> vecInfo;

    // add properties
    for(int i = metaObject.propertyOffset(); i < metaObject.propertyCount(); i++)
    {
        const auto name =  QString::fromLatin1(metaObject.property(i).name());
        vecInfo.push_back({name, true});
    }

    // add methods
    for(int i = metaObject.methodOffset(); i < metaObject.methodCount(); i++)
    {
        if(metaObject.method(i).methodType() == QMetaMethod::Method)
        {
            const auto name =  QString::fromLatin1(metaObject.method(i).name());
            vecInfo.push_back({name, false});
        }
    }

    _completerMap[metaObject.className()] = vecInfo;
}

///
/// \brief JSCompleterModel::JSCompleterModel
/// \param parent
///
JSCompleterModel::JSCompleterModel(QObject *parent)
    :QAbstractListModel(parent)
{
    addMetaObject(console::staticMetaObject);
    addMetaObject(Script::staticMetaObject);
    addMetaObject(Storage::staticMetaObject);
    addMetaObject(Server::staticMetaObject);
}

///
/// \brief JSCompleterModel::rowCount
/// \return
///
int JSCompleterModel::rowCount(const QModelIndex&) const
{
    return _completerMap.contains(_completionKey) ? _completerMap[_completionKey].size() : 0;
}

///
/// \brief JSCompleterModel::columnCount
/// \return
///
int JSCompleterModel::columnCount(const QModelIndex&) const
{
    return 1;
}

///
/// \brief JSCompleterModel::data
/// \param index
/// \param role
/// \return
///
QVariant JSCompleterModel::data(const QModelIndex &index, int role) const
{
    if(!_completerMap.contains(_completionKey) ||
            index.row() < 0 || index.row() >= rowCount())
    {
        return QVariant();
    }

    switch(role)
    {
        case Qt::DecorationRole:
           return _completerMap[_completionKey].at(index.row()).IsProperty ?
                        QIcon(":/res/iconProp.png") :
                        QIcon(":/res/iconFunc.png");

        case Qt::DisplayRole:
           return _completerMap[_completionKey].at(index.row()).Name;
    }

    return QVariant();
}

///
/// \brief JSCompleterModel::completionKey
/// \return
///
QString JSCompleterModel::completionKey() const
{
    return _completionKey;
}

///
/// \brief JSCompleterModel::setCompletionKey
/// \param key
///
void JSCompleterModel::setCompletionKey(const QString& key)
{
    beginResetModel();
    _completionKey = key;
    endResetModel();
}

///
/// \brief JSCompleter::JSCompleter
/// \param parent
///
JSCompleter::JSCompleter(QWidget* parent)
    : QCompleter(parent)
{
    setWidget(parent);
    setCaseSensitivity(Qt::CaseSensitive);
    setCompletionMode(QCompleter::PopupCompletion);
    setCompletionRole(Qt::DisplayRole);
    setWrapAround(false);

    setModel(new JSCompleterModel(this));
}

///
/// \brief JSCompleter::completionKey
/// \return
///
QString JSCompleter::completionKey() const
{
    return ((JSCompleterModel*)model())->completionKey();
}

///
/// \brief JSCompleter::setCompletionPrefix
/// \param prefix
///
void JSCompleter::setCompletionKey(const QString& key)
{
    ((JSCompleterModel*)model())->setCompletionKey(key);
}

