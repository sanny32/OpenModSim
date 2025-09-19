#include <QTcpSocket>
#include "modbustcpconnection.h"

///
/// \brief ModbusTcpConnection::ModbusTcpConnection
/// \param socketDescriptor
/// \param backend
/// \param parent
///
ModbusTcpConnection::ModbusTcpConnection(qintptr socketDescriptor, QModbusTcpServer* backend, QObject* parent)
    : QObject{parent}
    ,_backend(backend)
{
    _socket = new QTcpSocket(this);
    connect(_socket, &QTcpSocket::readyRead, this, &ModbusTcpConnection::onReadyRead);
    connect(_socket, &QTcpSocket::disconnected, this, &ModbusTcpConnection::onDisconnected);
    connect(_socket, &QTcpSocket::bytesWritten, this, &ModbusTcpConnection::onBytesWritten);

    if (!_socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Failed to set socket descriptor";
        deleteLater();
        return;
    }

    qDebug() << "New Modbus TCP connection from" << _socket->peerAddress().toString();
}

///
/// \brief ModbusTcpConnection::onReadyRead
///
void ModbusTcpConnection::onReadyRead()
{
    if (_isProcessing) return;

    _buffer.append(_socket->readAll());

    while (_buffer.size() >= 6) // MBAP header size
    {
        quint16 transactionId = readUInt16(_buffer, 0);
        quint16 protocolId    = readUInt16(_buffer, 2);
        quint16 length        = readUInt16(_buffer, 4);

        if (protocolId != 0) {
            qWarning() << "Invalid Modbus protocol ID:" << protocolId;
            _socket->close();
            return;
        }

        if (_buffer.size() < 6 + length) {
            return;
        }

        if (length < 2) {
            qWarning() << "Invalid Modbus PDU length";
            _socket->close();
            return;
        }

        QByteArray message = _buffer.left(6 + length);
        _buffer.remove(0, 6 + length);

        processModbusMessage(message);
    }
}

///
/// \brief ModbusTcpConnection::processModbusMessage
/// \param rawMessage
///
void ModbusTcpConnection::processModbusMessage(const QByteArray& rawMessage)
{
    if (rawMessage.size() < 7) return;

    quint8 unitId = static_cast<quint8>(rawMessage[6]);

    // 🔁 Ваша логика вместо matchingServerAddress!
    bool shouldProcess = true; // ✅ Замените на свою логику

    // Примеры:
    // shouldProcess = (unitId != 0); // игнорировать broadcast
    // shouldProcess = (unitId == 1 || unitId == 2); // только определённые адреса
    // shouldProcess = true; // принимать всё

    qDebug() << "Modbus Unit ID:" << unitId << "→" << (shouldProcess ? "ACCEPT" : "REJECT");

    if (!shouldProcess) {
        // 🚫 Отклоняем запрос — просто ничего не делаем
        // Можно отправить exception response, если нужно
        return;
    }

    // ✅ Передаём запрос внутреннему QModbusTcpServer
    // Но: QModbusTcpServer не имеет публичного API для "принудительной обработки запроса"

    // ❗ Проблема: QModbusTcpServer ожидает, что запрос придёт через его QTcpServer
    // → Нам нужно "подсунуть" запрос так, будто он пришёл по сети

    // 💡 Решение: создать "виртуальное соединение" — но это сложно

    // ⚠️ Альтернатива: использовать QModbusServer::processRequest() — но он protected!

    // 😡 Значит, нужно либо наследовать QModbusTcpServer и открыть processRequest,
    // либо использовать дружественный класс, либо... хак с d_ptr (но мы же хотим избежать!)

    // → Давайте сделаем простой выход: если запрос принят — передадим его "как есть"
    // во внутренний сервер через его слушающий сокет? Нет, это зациклит.

    // 🤔 Лучший вариант: создать наследника QModbusTcpServer с открытым processRequest

    // Переходим к ШАГУ 3 👇
}

///
/// \brief ModbusTcpConnection::onDisconnected
///
void ModbusTcpConnection::onDisconnected()
{
    qDebug() << "Client disconnected";
    deleteLater();
}

///
/// \brief ModbusTcpConnection::onBytesWritten
/// \param bytes
///
void ModbusTcpConnection::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)
}

///
/// \brief ModbusTcpConnection::readUInt16
/// \param data
/// \param offset
/// \return
///
quint16 ModbusTcpConnection::readUInt16(const QByteArray& data, int offset) const
{
    if (offset + 1 >= data.size()) return 0;
    return static_cast<quint16>((static_cast<quint8>(data[offset]) << 8) |
                                static_cast<quint8>(data[offset + 1]));
}

///
/// \brief ModbusTcpConnection::writeUInt16
/// \param data
/// \param offset
/// \param value
///
void ModbusTcpConnection::writeUInt16(QByteArray& data, int offset, quint16 value) const
{
    if (offset + 1 >= data.size()) return;
    data[offset]     = static_cast<char>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<char>(value & 0xFF);
}
