#ifndef FORMREGISTERMAPVIEW_H
#define FORMREGISTERMAPVIEW_H

#include <QWidget>
#include <QMap>
#include <QDateTime>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSettings>
#include "modbusmultiserver.h"
#include "connectiondetails.h"
#include "controls/outputtypes.h"
#include "enums.h"

class MainWindow;

namespace Ui {
class FormRegisterMapView;
}

///
/// \brief The RegisterMapViewDefinitions struct
///
struct RegisterMapViewDefinitions
{
    QString FormName;
    void normalize() {}
};
Q_DECLARE_METATYPE(RegisterMapViewDefinitions)

///
/// \brief The FormRegisterMapView class
///
class FormRegisterMapView : public QWidget
{
    Q_OBJECT

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormRegisterMapView* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormRegisterMapView* frm);

public:
    explicit FormRegisterMapView(ModbusMultiServer& server, MainWindow* parent);
    ~FormRegisterMapView();

    RegisterMapViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const RegisterMapViewDefinitions& dd);

    void saveXml(QXmlStreamWriter& xml) const;
    void loadXml(QXmlStreamReader& xml);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    void show();

signals:
    void showed();
    void closing();
    void definitionChanged();

private slots:
    void on_mbRequest(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msg);
    void on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data);
    void on_actionAdd_triggered();
    void on_actionDelete_triggered();
    void on_actionClear_triggered();
    void on_tableWidget_cellChanged(int row, int col);

private:
    struct Entry {
        quint16 value = 0;
        QString comment;
        DataType      type  = DataType::Int16;
        RegisterOrder order = RegisterOrder::MSRF;
        QDateTime timestamp;
    };

    void processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 count);
    int  findRow(const ItemMapKey& key) const;
    void insertEntry(const ItemMapKey& key, const Entry& entry);
    void updateValue(int row, const ItemMapKey& key, quint16 value);
    void updateAddressCells();
    void setupServerConnections();

    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int>& widths);

    QString registerTypeToString(QModbusDataUnit::RegisterType type) const;
    QModbusDataUnit::RegisterType stringToRegisterType(const QString& str) const;
    QVector<quint16> regsForKey(const ItemMapKey& key, DataType type) const;
    QString formatValue(QModbusDataUnit::RegisterType regType, DataType type, RegisterOrder order, const QVector<quint16>& regs) const;
    QString addressToDisplay(quint16 addr) const;
    quint16 addressFromDisplay(const QString& text, bool* ok = nullptr) const;
    ItemMapKey keyFromRow(int row) const;

private:
    Ui::FormRegisterMapView* ui;
    ModbusMultiServer& _mbMultiServer;
    RegisterMapViewDefinitions _displayDefinition;
    QMap<ItemMapKey, Entry> _registerMap;
    bool _updatingTable = false;
};

///
/// \brief operator << (XML writer)
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormRegisterMapView* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormRegisterMapView");

    const auto panel = frm->property("SplitPanel").toString();
    if (!panel.isEmpty())
        xml.writeAttribute("Panel", panel);
    if (frm->property("Closed").toBool())
        xml.writeAttribute("Closed", "1");

    const auto wnd = frm->parentWidget();
    xml.writeStartElement("Window");
    xml.writeAttribute("Maximized", boolToString(wnd->isMaximized()));
    xml.writeAttribute("Minimized", boolToString(wnd->isMinimized()));
    const auto windowPos = wnd->pos();
    xml.writeAttribute("Left",   QString::number(windowPos.x()));
    xml.writeAttribute("Top",    QString::number(windowPos.y()));
    const auto windowSize = (wnd->isMinimized() || wnd->isMaximized()) ? wnd->sizeHint() : wnd->size();
    xml.writeAttribute("Width",  QString::number(windowSize.width()));
    xml.writeAttribute("Height", QString::number(windowSize.height()));
    xml.writeEndElement(); // Window

    xml.writeStartElement("RegisterMapViewDefinitions");
    xml.writeAttribute("FormName", frm->displayDefinition().FormName);
    xml.writeEndElement();

    xml.writeStartElement("ColumnWidths");
    const auto ws = frm->columnWidths();
    if (ws.size() == 8) {
        xml.writeAttribute("Unit",      QString::number(ws[0]));
        xml.writeAttribute("Type",      QString::number(ws[1]));
        xml.writeAttribute("Address",   QString::number(ws[2]));
        xml.writeAttribute("DataType",  QString::number(ws[3]));
        xml.writeAttribute("Order",     QString::number(ws[4]));
        xml.writeAttribute("Comment",   QString::number(ws[5]));
        xml.writeAttribute("Value",     QString::number(ws[6]));
        xml.writeAttribute("Timestamp", QString::number(ws[7]));
    }
    xml.writeEndElement();

    xml.writeStartElement("RegisterMap");
    for (auto it = frm->_registerMap.cbegin(); it != frm->_registerMap.cend(); ++it) {
        xml.writeStartElement("Entry");
        xml.writeAttribute("DeviceId",  QString::number(it.key().DeviceId));
        xml.writeAttribute("Type",      QString::number(it.key().Type));
        xml.writeAttribute("Address",   QString::number(it.key().Address));
        xml.writeAttribute("DataType",   enumToString(it.value().type));
        if(isMultiRegisterType(it.value().type))
            xml.writeAttribute("Order", enumToString(it.value().order));
        xml.writeAttribute("Value",     QString::number(it.value().value));
        xml.writeAttribute("Timestamp", it.value().timestamp.toString(Qt::ISODateWithMs));
        if (!it.value().comment.isEmpty())
            xml.writeCDATA(it.value().comment);
        xml.writeEndElement(); // Entry
    }
    xml.writeEndElement(); // RegisterMap

    xml.writeEndElement(); // FormRegisterMapView
    return xml;
}

///
/// \brief operator >> (XML reader)
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormRegisterMapView* frm)
{
    if (!frm) return xml;

    if (!xml.isStartElement() || xml.name() != QLatin1String("FormRegisterMapView")) {
        xml.skipCurrentElement();
        return xml;
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("Window")) {
            const auto attrs = xml.attributes();
            if (auto* wnd = frm->parentWidget()) {
                bool ok;
                if (attrs.hasAttribute("Left") && attrs.hasAttribute("Top")) {
                    const int left = attrs.value("Left").toInt(&ok);
                    const int top  = attrs.value("Top").toInt();
                    if (ok) wnd->move(left, top);
                }
                if (attrs.hasAttribute("Width") && attrs.hasAttribute("Height")) {
                    const int w = attrs.value("Width").toInt(&ok);
                    const int h = attrs.value("Height").toInt();
                    if (ok && !wnd->isMaximized() && !wnd->isMinimized())
                        wnd->resize(w, h);
                }
                if (attrs.hasAttribute("Maximized") && stringToBool(attrs.value("Maximized").toString()))
                    wnd->showMaximized();
                if (attrs.hasAttribute("Minimized") && stringToBool(attrs.value("Minimized").toString()))
                    wnd->showMinimized();
            }
            xml.skipCurrentElement();
        }
        else if (xml.name() == QLatin1String("RegisterMapViewDefinitions")) {
            RegisterMapViewDefinitions dd;
            dd.FormName = xml.attributes().value("FormName").toString();
            frm->setDisplayDefinition(dd);
            xml.skipCurrentElement();
        }
        else if (xml.name() == QLatin1String("ColumnWidths")) {
            const auto& a = xml.attributes();
            auto w = [&](const char* name) {
                bool ok;
                const int v = a.value(QLatin1String(name)).toInt(&ok);
                return (ok && v > 0) ? v : -1;
            };
            frm->setColumnWidths({w("Unit"), w("Type"), w("Address"), w("DataType"), w("Order"), w("Comment"), w("Value"), w("Timestamp")});
            xml.skipCurrentElement();
        }
        else if (xml.name() == QLatin1String("RegisterMap")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("Entry")) {
                    const auto attrs = xml.attributes();
                    bool ok;
                    ItemMapKey key;
                    key.DeviceId = static_cast<quint8>(attrs.value("DeviceId").toUShort(&ok));
                    if (!ok) { xml.skipCurrentElement(); continue; }
                    key.Type = static_cast<QModbusDataUnit::RegisterType>(attrs.value("Type").toInt(&ok));
                    if (!ok) { xml.skipCurrentElement(); continue; }
                    key.Address = attrs.value("Address").toUShort(&ok);
                    if (!ok) { xml.skipCurrentElement(); continue; }

                    FormRegisterMapView::Entry entry;
                    entry.value  = attrs.value("Value").toUShort();
                    entry.type   = enumFromString<DataType>(attrs.value("DataType").toString(), DataType::Int16);
                    entry.order  = enumFromString<RegisterOrder>(attrs.value("Order").toString(), RegisterOrder::MSRF);
                    entry.timestamp = QDateTime::fromString(attrs.value("Timestamp").toString(), Qt::ISODateWithMs);
                    entry.comment = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();

                    frm->_registerMap[key] = entry;
                    frm->insertEntry(key, entry);
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }

    return xml;
}

#endif // FORMREGISTERMAPVIEW_H
