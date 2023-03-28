#include <QWidget>
#include <QMetaProperty>
#include <QStringListModel>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"
#include "jscompleter.h"

///
/// \brief The MethodMetaType enum
///
enum class MethodMetaType
{
    Method = 0,
    Property,
    Enumerator
};

///
/// \brief The MethodMetaInfo class
///
struct MethodMetaInfo
{
    QString Name;
    MethodMetaType MetaType = MethodMetaType::Method;

    bool operator==(const MethodMetaInfo& mi) const {
        return Name == mi.Name && MetaType == mi.MetaType;
    }
};
QMap<QString, QVector<MethodMetaInfo>> _completerMap = {
    { "Math",
      {
          {"E", MethodMetaType::Property}, {"LN10", MethodMetaType::Property}, {"LN2", MethodMetaType::Property}, {"LOG10E", MethodMetaType::Property},
          {"LOG2E", MethodMetaType::Property}, {"PI", MethodMetaType::Property}, {"SQRT1_2", MethodMetaType::Property}, {"SQRT2", MethodMetaType::Property},
          {"abs"}, {"acos"}, {"acosh"}, {"asin"}, {"asinh"}, {"atan"}, {"atanh"}, {"atan2"},
          {"cbrt"}, {"ceil"}, {"clz32"}, {"cos"}, {"cosh"}, {"exp"}, {"expm1"}, {"floor"},
          {"fround"}, {"hypot"}, {"imul"}, {"log"}, {"log1p"}, {"log10"}, {"log2"}, {"max"},
          {"min"}, {"pow"}, {"random"}, {"round"}, {"sign"}, {"sin"}, {"sinh"}, {"sqrt"},
          {"tan"}, {"tanh"}, {"trunc"}
      }
    },
    { "Date",
      {
          {"prototype", MethodMetaType::Property},
          {"now"}, {"parse"}, {"UTC"}
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
        vecInfo.push_back({name, MethodMetaType::Property});
    }

    // add enums
    for(int i = metaObject.enumeratorOffset(); i < metaObject.enumeratorCount(); i++)
    {
        const auto enumerator = metaObject.enumerator(i);
        for(int j = 0; j < enumerator.keyCount(); j++)
        {
            const auto name =  QString::fromLatin1(enumerator.key(j));
            vecInfo.push_back({name, MethodMetaType::Enumerator});
        }
    }

    // add methods
    for(int i = metaObject.methodOffset(); i < metaObject.methodCount(); i++)
    {
        if(metaObject.method(i).methodType() == QMetaMethod::Method)
        {
            const auto name =  QString::fromLatin1(metaObject.method(i).name());
            if(!vecInfo.contains({name})) vecInfo.push_back({name});
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
    ,_iconFunc(":/res/iconFunc.png")
    ,_iconEnum(":/res/iconEnum.png")
{
    if(!_completerMap.contains(console::staticMetaObject.className()))
    {
        addMetaObject(console::staticMetaObject);
        addMetaObject(Script::staticMetaObject);
        addMetaObject(Storage::staticMetaObject);
        addMetaObject(Server::staticMetaObject);
        addMetaObject(Register::staticMetaObject);
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
            switch(info.MetaType)
            {
                case MethodMetaType::Method:
                return _iconFunc;
                case MethodMetaType::Property:
                return _iconProp;
                case MethodMetaType::Enumerator:
                return _iconEnum;
            }
        break;

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

