#include "storageobject.h"

///
/// \brief StorageObject::StorageObject
/// \param parent
///
StorageObject::StorageObject(QObject *parent)
    : QObject{parent}
{
}

///
/// \brief StorageObject::length
/// \return
///
int StorageObject::length() const
{
    return _storage.size();
}

///
/// \brief StorageObject::key
/// \param index
/// \return
///
QString StorageObject::key(int index) const
{
    const auto it = std::next(_storage.constBegin(), index);
    if(it != _storage.end()) return *it;
    else return QString();
}

///
/// \brief StorageObject::getItem
/// \param key
/// \return
///
QString StorageObject::getItem(const QString& key) const
{
    const auto it  = _storage.find(key);
    if(it != _storage.end()) return *it;
    else return QString();
}

///
/// \brief StorageObject::setItem
/// \param key
/// \param value
///
void StorageObject::setItem(const QString& key, const QString& value)
{
    _storage[key] = value;
}

///
/// \brief StorageObject::removeItem
/// \param key
///
void StorageObject::removeItem(const QString& key)
{
    _storage.remove(key);
}

///
/// \brief StorageObject::clear
///
void StorageObject::clear()
{
    _storage.clear();
}
