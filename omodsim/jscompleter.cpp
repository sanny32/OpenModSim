#include <QWidget>
#include <QMetaProperty>
#include <QStringListModel>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"
#include "jscompleter.h"

///
/// \brief The MethodMetaInfo class
///
struct MethodMetaInfo
{
    QString Name;
    bool IsProperty;

    bool operator==(const MethodMetaInfo& mi) const {
        return Name == mi.Name && IsProperty == mi.IsProperty;
    }
};
QMap<QString, QVector<MethodMetaInfo>> _completerMap = {
    { "Math",
      {
          {"E", 1}, {"LN10", 1}, {"LN2", 1}, {"LOG10E", 1}, {"LOG2E", 1}, {"PI", 1}, {"SQRT1_2", 1}, {"SQRT2", 1},
          {"abs", 0}, {"acos", 0}, {"acosh", 0}, {"asin", 0}, {"asinh", 0}, {"atan", 0}, {"atanh", 0}, {"atan2", 0},
          {"cbrt", 0}, {"ceil", 0}, {"clz32", 0}, {"cos", 0}, {"cosh", 0}, {"exp", 0}, {"expm1", 0}, {"floor", 0},
          {"fround", 0}, {"hypot", 0}, {"imul", 0}, {"log", 0}, {"log1p", 0}, {"log10", 0}, {"log2", 0}, {"max", 0},
          {"min", 0}, {"pow", 0}, {"random", 0}, {"round", 0}, {"sign", 0}, {"sin", 0}, {"sinh", 0}, {"sqrt", 0},
          {"tan", 0}, {"tanh", 0}, {"trunc", 0}
      }
    },
    { "Date",
      {
          {"prototype", 1},
          {"now", 0}, {"parse", 0}, {"UTC", 0}
      }
    }
};

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
            if(!vecInfo.contains({name, false})) vecInfo.push_back({name, false});
        }
    }

    _completerMap[metaObject.className()] = vecInfo;
}

///
/// \brief JSCompleterModel::JSCompleterModel
/// \param parent
///
JSCompleterModel::JSCompleterModel(QObject *parent)
    : QAbstractListModel(parent)
    ,_iconProp(":/res/iconProp.png")
    ,_icomFunc(":/res/iconFunc.png")
{
    if(!_completerMap.contains(console::staticMetaObject.className()))
    {
        addMetaObject(console::staticMetaObject);
        addMetaObject(Script::staticMetaObject);
        addMetaObject(Storage::staticMetaObject);
        addMetaObject(Server::staticMetaObject);
    }
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

    const auto info = _completerMap[_completionKey].at(index.row());
    switch(role)
    {
        case Qt::DecorationRole:
           return info.IsProperty ? _iconProp : _icomFunc;

        case Qt::DisplayRole:
           return info.Name;
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

