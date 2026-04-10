#ifndef FORMDATAMAPVIEW_H
#define FORMDATAMAPVIEW_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPrinter>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "modbusmultiserver.h"
#include "connectiondetails.h"
#include "controls/outputtypes.h"
#include "enums.h"
#include "datamapdatamodel.h"

class MainWindow;

namespace Ui {
class FormDataMapView;
}

///
/// \brief The FormDataMapView class
///
class FormDataMapView : public QWidget
{
    Q_OBJECT

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormDataMapView* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormDataMapView* frm);

public:
    explicit FormDataMapView(ModbusMultiServer& server, MainWindow* parent);
    ~FormDataMapView();

    DataMapViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const DataMapViewDefinitions& dd);
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);
    QColor foregroundColor() const;
    void setForegroundColor(const QColor& color);
    bool zeroBasedAddress() const { return _model && _model->zeroBased(); }
    void setZeroBasedAddress(bool zeroBased);
    bool hexView() const { return _model && _model->hexView(); }
    void setHexView(bool enabled);

    bool autoAddOnRequest() const { return _autoAddOnRequest; }
    void setAutoAddOnRequest(bool value) { _autoAddOnRequest = value; }
    bool isAutoRequestMap() const { return _autoRequestMap; }
    void setAutoRequestMap(bool value);
    bool isEmpty() const { return !_proxy || _proxy->rowCount() <= 0; }
    void print(QPrinter* printer);

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
    void on_mbTimestampChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QDateTime& timestamp);
    void on_mbDescriptionChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QString& description);
    void on_actionAdd_triggered();
    void on_actionInsert_triggered();
    void on_actionDelete_triggered();
    void on_actionClear_triggered();
    void updateActionState();

private:
    int addRowAndReturnSourceRow(int referenceSourceRow = -1);
    void editSourceRow(int sourceRow);
    void processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 count);
    void setupToolBar();
    void setupServerConnections();
    void updateWindowIcon();

    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int>& widths);

private:
    Ui::FormDataMapView*        ui;
    ModbusMultiServer&          _mbMultiServer;
    DataMapViewDefinitions  _displayDefinition;
    DataMapDataModel*       _model           = nullptr;
    DataMapFilterProxy*     _proxy           = nullptr;
    QComboBox*                  _filterTypeCombo = nullptr;
    QSpinBox*                   _filterUnitSpin  = nullptr;
    bool                        _autoRequestMap = false;
    bool                        _autoAddOnRequest = false;
};

///
/// \brief operator << (XML writer)
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormDataMapView* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormDataMapView");

    const auto panel = frm->property("SplitPanel").toString();
    if (!panel.isEmpty())
        xml.writeAttribute("Panel", panel);
    xml.writeAttribute("Title", frm->windowTitle());
    if (frm->property("SplitAutoClone").toBool())
        xml.writeAttribute("AutoClone", "1");
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

    xml.writeStartElement("DataMapViewDefinitions");
    const auto dd = frm->displayDefinition();
    xml.writeAttribute("FormName",         dd.FormName);
    xml.writeAttribute("ZeroBasedAddress", boolToString(dd.ZeroBasedAddress));
    xml.writeAttribute("HexView",          boolToString(dd.HexView));
    xml.writeAttribute("AutoAddOnRequest", boolToString(frm->autoAddOnRequest()));
    xml.writeAttribute("AutoRequestMap",   boolToString(frm->isAutoRequestMap()));
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

    xml.writeStartElement("DataMap");
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
        if (e.color.isValid())
            xml.writeAttribute("Color", e.color.name());
        if (!e.comment.isEmpty())
            xml.writeCDATA(e.comment);
        xml.writeEndElement(); // Entry
    }
    xml.writeEndElement(); // DataMap

    xml.writeEndElement(); // FormDataMapView
    return xml;
}

///
/// \brief operator >> (XML reader)
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormDataMapView* frm)
{
    if (!frm) return xml;

    if (!xml.isStartElement() || xml.name() != QLatin1String("FormDataMapView")) {
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
        else if (xml.name() == QLatin1String("DataMapViewDefinitions")) {
            const auto attrs = xml.attributes();
            DataMapViewDefinitions dd;
            dd.FormName         = attrs.value("FormName").toString();
            dd.ZeroBasedAddress = stringToBool(attrs.value("ZeroBasedAddress").toString());
            dd.HexView          = stringToBool(attrs.value("HexView").toString());
            frm->setDisplayDefinition(dd);
            frm->setAutoAddOnRequest(stringToBool(attrs.value("AutoAddOnRequest").toString()));
            frm->setAutoRequestMap(stringToBool(attrs.value("AutoRequestMap").toString()));
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
        else if (xml.name() == QLatin1String("DataMap")) {
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

                    DataMapEntry entry;
                    entry.value     = attrs.value("Value").toUShort();
                    entry.type      = enumFromString<DataType>(attrs.value("DataType").toString(), DataType::Int16);
                    entry.order     = enumFromString<RegisterOrder>(attrs.value("Order").toString(), RegisterOrder::MSRF);
                    entry.timestamp = QDateTime::fromString(attrs.value("Timestamp").toString(), Qt::ISODateWithMs);
                    if (attrs.hasAttribute("Color"))
                        entry.color = QColor(attrs.value("Color").toString());
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

#endif // FORMDATAMAPVIEW_H
