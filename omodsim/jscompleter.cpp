#include <QStringListModel>
#include "jscompleter.h"

QMultiHash<QString, QString> JSCompleterModel::_completerMap = {
    { QLatin1String("console"), QLatin1String("log") },
    { QLatin1String("console"), QLatin1String("debug") },
    { QLatin1String("console"), QLatin1String("warning") },
    { QLatin1String("console"), QLatin1String("error") },
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
    return _completerMap.size();
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
        return *std::next(_completerMap.constBegin(), index.row());
}

///
/// \brief JSCompleter::JSCompleter
/// \param parent
///
JSCompleter::JSCompleter(QObject *parent)
    : QCompleter{parent}
{
    //setModel(new JSCompleterModel(this));
    QStringList list;
    list << "console" << "console.log()" << "console.debug()";
    setModel(new QStringListModel (list, this));

    setCaseSensitivity(Qt::CaseSensitive);
    setCompletionMode(QCompleter::PopupCompletion);
    setWrapAround(false);
}
