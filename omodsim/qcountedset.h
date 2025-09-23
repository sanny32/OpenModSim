#ifndef QCOUNTEDSET_H
#define QCOUNTEDSET_H

#include <QList>
#include <QHash>

///
/// \brief The QCountedSet class
///
template<typename T>
class QCountedSet
{
public:
    void clear() {
        _counts.clear();
    }

    void insert(int value) {
        _counts[value]++;
    }

    bool remove(int value) {
        auto it = _counts.find(value);
        if (it == _counts.end())
            return false;

        it.value()--;
        if (it.value() == 0)
            _counts.erase(it);
        return true;
    }

    int count(int value) const {
        return _counts.value(value, 0);
    }

    bool contains(int value) const {
        return _counts.contains(value);
    }

    QList<int> values() const {
        return _counts.keys();
    }

    int size() const {
        return _counts.size();
    }

    bool isEmpty() const {
        return _counts.isEmpty();
    }

private:
    QHash<T, int> _counts;
};

#endif // QCOUNTEDSET_H
