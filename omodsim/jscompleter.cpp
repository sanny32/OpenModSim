#include <QWidget>
#include <QStringListModel>
#include "jscompleter.h"

QMap<QString, QStringList> _completerMap =
{
    { "console",    {"log", "debug", "warning", "error", "clear"} },
    { "Script",     {"stop"} },
    { "Storage",    {"length", "key", "getItem", "setItem", "removeItem", "clear"} },
    { "Server",     {"readHolding", "readInput", "readDiscrete", "readCoil", "writeHolding", "writeInput", "writeDiscrete", "writeCoil"} }
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
    switch(role)
    {
        case Qt::DisplayRole:
            if(index.row() < 0 || index.row() >= rowCount())
                return QVariant();
            else
                return _completerMap.contains(_completionKey) ? _completerMap[_completionKey].at(index.row()) : QString();
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
void JSCompleterModel::setCompletionKey(const QString& prefix)
{
    beginResetModel();
    _completionKey = prefix;
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

