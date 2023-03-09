#ifndef STORAGE_H
#define STORAGE_H

#include <QMap>
#include <QObject>
#include <QJSValue>

///
/// \brief The Storage class
///
class Storage : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit Storage();

    Q_PROPERTY(int length READ length CONSTANT);

    Q_INVOKABLE QJSValue key(int index) const;
    Q_INVOKABLE QJSValue getItem(const QString& key) const;
    Q_INVOKABLE void setItem(const QString& key, const QJSValue& value);
    Q_INVOKABLE void removeItem(const QString& key);
    Q_INVOKABLE void clear();

    int length() const;

private:
    QMap<QString, QJSValue> _storage;

};

#endif // STORAGE_H
