#include <QWidget>
#include <QStringListModel>
#include "jscompleter.h"

QMap<QString, QStringList> _completerMap =
{
    { "console", {"log()", "debug()", "warning()", "error()", "clear()"} },
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
    return _completerMap.contains(_prefix) ? _completerMap[_prefix].size() : 0;
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
                return _completerMap.contains(_prefix) ? _completerMap[_prefix].at(index.row()) : QString();
    }

    return QVariant();
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
    beginResetModel();
    _prefix = prefix;
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
/// \brief JSCompleter::setCompletionPrefix
/// \param prefix
///
void JSCompleter::setCompletionPrefix(const QString& prefix)
{
    //setModel(new QStringListModel (_completerMap[prefix], this));
    ((JSCompleterModel*)model())->setPrefix(prefix);
}

