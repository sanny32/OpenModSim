#ifndef FORMREGISTERMAPVIEW_H
#define FORMREGISTERMAPVIEW_H

#include <QWidget>
#include <QMap>
#include <QToolBar>
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

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    void saveXml(QXmlStreamWriter& xml) const;
    void loadXml(QXmlStreamReader& xml);

    void connectEditSlots();
    void disconnectEditSlots();

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
    void on_actionClearMap_triggered();
    void on_tableWidget_cellChanged(int row, int col);

private:
    struct Entry {
        quint16 value = 0;
        QString description;
    };

    void processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 count);
    int  findRow(const ItemMapKey& key) const;
    void insertEntry(const ItemMapKey& key, quint16 value, const QString& description);
    void updateValue(int row, quint16 value);
    void setupToolbar();
    void setupServerConnections();
    QString registerTypeToString(QModbusDataUnit::RegisterType type) const;
    QString formatValue(QModbusDataUnit::RegisterType type, quint16 value) const;

private:
    Ui::FormRegisterMapView* ui;
    ModbusMultiServer& _mbMultiServer;
    RegisterMapViewDefinitions _displayDefinition;
    QMap<ItemMapKey, Entry> _registerMap;
    bool _updatingTable = false;
    QToolBar* _toolBar = nullptr;
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

    xml.writeStartElement("Colors");
    xml.writeAttribute("Background", frm->backgroundColor().name());
    xml.writeAttribute("Foreground", frm->foregroundColor().name());
    xml.writeEndElement(); // Colors

    xml.writeStartElement("Font");
    const QFont font = frm->font();
    xml.writeAttribute("Family", font.family());
    xml.writeAttribute("Size",   QString::number(font.pointSize()));
    xml.writeAttribute("Bold",   boolToString(font.bold()));
    xml.writeAttribute("Italic", boolToString(font.italic()));
    xml.writeEndElement(); // Font

    xml.writeStartElement("RegisterMapViewDefinitions");
    xml.writeAttribute("FormName", frm->_displayDefinition.FormName);
    xml.writeEndElement();

    xml.writeStartElement("RegisterMap");
    for (auto it = frm->_registerMap.cbegin(); it != frm->_registerMap.cend(); ++it) {
        xml.writeStartElement("Entry");
        xml.writeAttribute("DeviceId", QString::number(it.key().DeviceId));
        xml.writeAttribute("Type",     QString::number(it.key().Type));
        xml.writeAttribute("Address",  QString::number(it.key().Address));
        xml.writeAttribute("Value",    QString::number(it.value().value));
        if (!it.value().description.isEmpty())
            xml.writeCDATA(it.value().description);
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
        else if (xml.name() == QLatin1String("Colors")) {
            const auto attrs = xml.attributes();
            if (attrs.hasAttribute("Background")) {
                const QColor c(attrs.value("Background").toString());
                if (c.isValid()) frm->setBackgroundColor(c);
            }
            if (attrs.hasAttribute("Foreground")) {
                const QColor c(attrs.value("Foreground").toString());
                if (c.isValid()) frm->setForegroundColor(c);
            }
            xml.skipCurrentElement();
        }
        else if (xml.name() == QLatin1String("Font")) {
            const auto attrs = xml.attributes();
            QFont font = frm->font();
            if (attrs.hasAttribute("Family")) font.setFamily(attrs.value("Family").toString());
            if (attrs.hasAttribute("Size")) {
                bool ok;
                const int sz = attrs.value("Size").toInt(&ok);
                if (ok && sz > 0) font.setPointSize(sz);
            }
            if (attrs.hasAttribute("Bold"))   font.setBold(stringToBool(attrs.value("Bold").toString()));
            if (attrs.hasAttribute("Italic")) font.setItalic(stringToBool(attrs.value("Italic").toString()));
            frm->setFont(font);
            xml.skipCurrentElement();
        }
        else if (xml.name() == QLatin1String("RegisterMapViewDefinitions")) {
            RegisterMapViewDefinitions dd;
            dd.FormName = xml.attributes().value("FormName").toString();
            frm->setDisplayDefinition(dd);
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
                    const quint16 value = attrs.value("Value").toUShort();
                    const QString description = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();

                    FormRegisterMapView::Entry entry;
                    entry.value = value;
                    entry.description = description;
                    frm->_registerMap[key] = entry;
                    frm->insertEntry(key, value, description);
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
