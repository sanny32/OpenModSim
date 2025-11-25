#ifndef BUFFERINGLISTMODEL_H
#define BUFFERINGLISTMODEL_H

#include <QQueue>
#include <QAbstractListModel>

///
/// \brief The BufferingListModel class
///
template<typename T>
class  BufferingListModel : public QAbstractListModel
{
public:
    ///
    /// \brief BufferingListModel
    /// \param parent
    ///
    explicit BufferingListModel(QObject* parent)
        : QAbstractListModel(parent)
    {
    }

    ///
    /// \brief itemAt
    /// \param index
    /// \return
    ///
    const T& itemAt(int index) const {
        return _items.at(index);
    }

    ///
    /// \brief rowCount
    /// \return
    ///
    int rowCount(const QModelIndex& index = QModelIndex()) const override {
        Q_UNUSED(index)
        return _items.size();
    }

    ///
    /// \brief clear
    ///
    void clear() {
        beginResetModel();
        deleteItems();
        endResetModel();
    }

    ///
    /// \brief append
    /// \param data
    ///
    void append(const T& data) {
        if(_bufferingMode) {
            _bufferingItems.push_back(data);
        }
        else {
            while(rowCount() >= _rowLimit)
            {
                beginRemoveRows(QModelIndex(), 0, 0);
                _items.removeFirst();
                endRemoveRows();
            }

            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            _items.push_back(data);
            endInsertRows();
        }
    }

    ///
    /// \brief update
    ///
    void update(){
        emit dataChanged(index(0), index(_items.size() - 1));
    }

    ///
    /// \brief rowLimit
    /// \return
    ///
    int rowLimit() const {
         return _rowLimit;
    }

    ///
    /// \brief setRowLimit
    /// \param val
    ///
    void setRowLimit(int val){
        _rowLimit = qMax(1, val);
    }

    ///
    /// \brief isBufferingMode
    /// \return
    ///
    bool isBufferingMode() const {
        return _bufferingMode;
    }

    ///
    /// \brief setBufferingMode
    /// \param value
    ///
    void setBufferingMode(bool value) {
        _bufferingMode = value;
        if(!_bufferingMode)
        {
            for(auto&& data : _bufferingItems) {
                _items.push_back(data);

                while(_items.size() >= _rowLimit) {
                    _items.removeFirst();
                }
            }
            _bufferingItems.clear();
            update();
        }
    }

private:
    void deleteItems() {
        _items.clear();
        _bufferingItems.clear();
    }

private:
    int _rowLimit = 30;
    bool _bufferingMode = false;
    QQueue<T> _items;
    QQueue<T> _bufferingItems;
};

#endif // BUFFERINGLISTMODEL_H
