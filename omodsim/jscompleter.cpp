#include <QStringListModel>
#include "jscompleter.h"

QMap<QString, QStringList> _completerMap =
{
    { "console", {"log()", "debug()", "warning()", "error()"} },
};

///
/// \brief JSCompleterModel::JSCompleterModel
/// \param parent
///
JSCompleterModel::JSCompleterModel(QObject *parent)
    :QAbstractListModel(parent)
{
}

///
/// \brief JSCompleterModel::rowCount
/// \return
///
int JSCompleterModel::rowCount(const QModelIndex&) const
{
    return _completerMap[_prefix].size();
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
    /*switch(role)
    {
        case Qt::EditRole:
        if(index.row() < 0 || index.row() >= rowCount())
            return QVariant();
        else
            return *std::next(_completerMap.constBegin(), index.row());
    }

    return QVariant();*/

    if(index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    else
        return _completerMap[_prefix].at(index.row());
}

///
/// \brief JSCompleterModel::prefix
/// \return
///
QString JSCompleterModel::prefix() const
{
    return _prefix;
}

///
/// \brief JSCompleterModel::setPrefix
/// \param prefix
///
void JSCompleterModel::setPrefix(const QString& prefix)
{
    _prefix = prefix;
}

///
/// \brief JSCompleter::JSCompleter
/// \param parent
///
JSCompleter::JSCompleter(QObject *parent)
    : QCompleter{parent}
{
    //setModel(new JSCompleterModel(this));
    /*QStringList list;
    list << "console" << "console.log()" << "console.debug()" << "Server";
    setModel(new QStringListModel (list, this));*/

    setCaseSensitivity(Qt::CaseSensitive);
    setCompletionMode(QCompleter::PopupCompletion);
    setWrapAround(false);
}

///
/// \brief JSCompleter::setCompletionPrefix
/// \param prefix
///
void JSCompleter::setCompletionPrefix(const QString& prefix)
{
    //setModel(new QStringListModel (_completerMap[prefix], this));
    //QCompleter::setCompletionPrefix(prefix);
    //((JSCompleterModel*)model())->setPrefix(prefix);
}
