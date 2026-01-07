#include "storage.h"

///
/// \brief Storage::Storage
/// \param parent
///
Storage::Storage()
{
}

///
/// \brief Storage::length
/// \return
///
int Storage::length() const
{
    return _storage.size();
}

///
/// \brief Storage::key
/// \param index
/// \return
///
QJSValue Storage::key(int index) const
{
    const auto it = std::next(_storage.constBegin(), index);
    if(it != _storage.end()) return *it;
    else return QJSValue(QJSValue::NullValue);
}

///
/// \brief Storage::getItem
/// \param key
/// \return
///
QJSValue Storage::getItem(const QString& key) const
{
    const auto it  = _storage.find(key);
    if(it != _storage.end()) return *it;
    else return QJSValue(QJSValue::NullValue);
}

///
/// \brief Storage::setItem
/// \param key
/// \param value
///
void Storage::setItem(const QString& key, const QJSValue& value)
{
    _storage[key] = value;
}

///
/// \brief Storage::removeItem
/// \param key
///
void Storage::removeItem(const QString& key)
{
    _storage.remove(key);
}

///
/// \brief Storage::clear
///
void Storage::clear()
{
    _storage.clear();
}
