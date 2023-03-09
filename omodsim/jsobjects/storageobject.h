#ifndef STORAGEOBJECT_H
#define STORAGEOBJECT_H

#include <QObject>
#include <QMap>
#include <QJSValue>

///
/// \brief The StorageObject class
///
class StorageObject : public QObject
{
    Q_OBJECT
public:
    explicit StorageObject(QObject *parent = nullptr);

    Q_PROPERTY(int length READ length);

    Q_INVOKABLE QJSValue key(int index) const;
    Q_INVOKABLE QJSValue getItem(const QString& key) const;
    Q_INVOKABLE void setItem(const QString& key, const QJSValue& value);
    Q_INVOKABLE void removeItem(const QString& key);
    Q_INVOKABLE void clear();

    int length() const;

private:
    QMap<QString, QJSValue> _storage;

};

#endif // STORAGEOBJECT_H
