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

    QString prefix() const;
    void setPrefix(const QString& prefix);

private:
    QString _prefix;
};

///
/// \brief The JSCompleter class
///
class JSCompleter : public QCompleter
{
public:
    explicit JSCompleter(QWidget* parent = nullptr);

    void setCompletionPrefix(const QString& prefix);
};

#endif // JSCOMPLETER_H
