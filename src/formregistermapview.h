#ifndef FORMREGISTERMAPVIEW_H
#define FORMREGISTERMAPVIEW_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "modbusmultiserver.h"
#include "connectiondetails.h"
#include "controls/outputtypes.h"
#include "enums.h"
#include "registermapdatamodel.h"

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
    bool    ZeroBasedAddress = false;
    bool    HexView          = false;
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

    bool autoAddOnRequest() const { return _autoAddOnRequest; }
    void setAutoAddOnRequest(bool value) { _autoAddOnRequest = value; }

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
    void on_actionInsert_triggered();
    void on_actionDelete_triggered();
    void on_actionClear_triggered();
    void on_actionHexView_toggled(bool checked);
    void updateActionState();

private:
    int addRowAndReturnSourceRow(int referenceSourceRow = -1);
    void editSourceRow(int sourceRow);
    void processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 count);
    void setupToolBar();
    void setupServerConnections();

    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int>& widths);

private:
    Ui::FormRegisterMapView*    ui;
    ModbusMultiServer&          _mbMultiServer;
    RegisterMapViewDefinitions  _displayDefinition;
    RegisterMapDataModel*       _model           = nullptr;
    RegisterMapFilterProxy*     _proxy           = nullptr;
    QComboBox*                  _filterTypeCombo = nullptr;
    QSpinBox*                   _filterUnitSpin  = nullptr;
    QComboBox*                  _addrBaseCombo   = nullptr;
    bool                        _autoAddOnRequest = false;
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
    xml.writeAttribute("FormName",         frm->displayDefinition().FormName);
    xml.writeAttribute("ZeroBasedAddress", boolToString(frm->displayDefinition().ZeroBasedAddress));
    xml.writeAttribute("HexView",          boolToString(frm->displayDefinition().HexView));
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
    const auto& orderedKeys = frm->_model->keys();
    const auto& map = frm->_model->entries();
    for (const auto& key : orderedKeys) {
        const auto& e = map[key];
        xml.writeStartElement("Entry");
        xml.writeAttribute("DeviceId",  QString::number(key.DeviceId));
        xml.writeAttribute("Type",      QString::number(key.Type));
        xml.writeAttribute("Address",   QString::number(key.Address));
        xml.writeAttribute("DataType",  enumToString(e.type));
        if (isMultiRegisterType(e.type))
            xml.writeAttribute("Order", enumToString(e.order));
        xml.writeAttribute("Value",     QString::number(e.value));
        xml.writeAttribute("Timestamp", e.timestamp.toString(Qt::ISODateWithMs));
        if (!e.comment.isEmpty())
            xml.writeCDATA(e.comment);
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
            const auto attrs = xml.attributes();
            RegisterMapViewDefinitions dd;
            dd.FormName        = attrs.value("FormName").toString();
            dd.ZeroBasedAddress = stringToBool(attrs.value("ZeroBasedAddress").toString());
            dd.HexView          = stringToBool(attrs.value("HexView").toString());
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

                    RegisterMapEntry entry;
                    entry.value     = attrs.value("Value").toUShort();
                    entry.type      = enumFromString<DataType>(attrs.value("DataType").toString(), DataType::Int16);
                    entry.order     = enumFromString<RegisterOrder>(attrs.value("Order").toString(), RegisterOrder::MSRF);
                    entry.timestamp = QDateTime::fromString(attrs.value("Timestamp").toString(), Qt::ISODateWithMs);
                    entry.comment   = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();

                    if (key.Type == QModbusDataUnit::Coils ||
                        key.Type == QModbusDataUnit::DiscreteInputs) {
                        entry.type  = DataType::Binary;
                        entry.order = RegisterOrder::MSRF;
                    }

                    frm->_model->addEntry(key, entry);
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
