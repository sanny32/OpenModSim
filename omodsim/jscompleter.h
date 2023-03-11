#ifndef JSCOMPLETER_H
#define JSCOMPLETER_H

#include <QCompleter>
#include <QAbstractItemModel>

///
/// \brief The JSCompleterModel class
///
class JSCompleterModel : public QAbstractListModel
{
public:
    explicit JSCompleterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    static QMultiHash<QString, QString> _completerMap;
};

///
/// \brief The JSCompleter class
///
class JSCompleter : public QCompleter
{
public:
    explicit JSCompleter(QObject *parent = nullptr);
};

#endif // JSCOMPLETER_H
