#ifndef JSCOMPLETER_H
#define JSCOMPLETER_H

#include <QIcon>
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

    QString completionKey() const;
    void setCompletionKey(const QString& key);

private:
    QIcon _iconProp;
    QIcon _icomFunc;
    QString _completionKey;
};

///
/// \brief The JSCompleter class
///
class JSCompleter : public QCompleter
{
public:
    explicit JSCompleter(QWidget* parent = nullptr);

    QString completionKey() const;
    void setCompletionKey(const QString& key);
};

#endif // JSCOMPLETER_H
