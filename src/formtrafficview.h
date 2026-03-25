#ifndef FORMTRAFFICVIEW_H
#define FORMTRAFFICVIEW_H

#include <QWidget>
#include <QTimer>
#include <QXmlStreamWriter>
#include <QQueue>
#include <QPrinter>
#include "fontutils.h"
#include "modbusmultiserver.h"
#include "displaydefinition.h"
#include "apppreferences.h"

///
/// \brief Forward declaration of the MainWindow
///
class MainWindow;
class QTextDocument;
class QSpinBox;
class QComboBox;
class QLabel;
class QWidget;
class QAction;
class QCheckBox;

namespace Ui {
class FormTrafficView;
}

///
/// \brief The FormTrafficView class
///
class FormTrafficView : public QWidget
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, FormTrafficView* frm);
    friend QSettings& operator >>(QSettings& in, FormTrafficView* frm);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormTrafficView* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormTrafficView* frm);

public:
    explicit FormTrafficView(ModbusMultiServer& server, MainWindow* parent);
    ~FormTrafficView();

    TrafficViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const TrafficViewDefinitions& dd);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    LogViewState logViewState() const;
    void setLogViewState(LogViewState state);

    void linkTo(FormTrafficView* other);

    bool isLogEmpty() const;
    void print(QPrinter* printer);

    void saveSettings(QSettings& out) const;
    void loadSettings(QSettings& in);
    void saveXml(QXmlStreamWriter& xml) const;
    void loadXml(QXmlStreamReader& xml);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    void show();
    void connectEditSlots();
    void disconnectEditSlots();

signals:
    void showed();
    void closing();
    void definitionChanged();
    void logViewStateChanged(LogViewState state);

private slots:
    void on_logUiFlushTimeout();
    void on_mbConnected(const ConnectionDetails& cd);
    void on_mbDisconnected(const ConnectionDetails& cd);
    void on_mbRequest(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msg);
    void on_mbResponse(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp);
    void on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data);
    void on_mbDefinitionsChanged(const ModbusDefinitions& defs);
    void on_actionPauseTraffic_toggled(bool checked);
    void on_actionClearTraffic_triggered();
    void on_actionExportTrafficLog_triggered();
    void on_actionHexView_toggled(bool checked);

private:
    struct TrafficLogEntry {
        ConnectionDetails Connection;
        QSharedPointer<const ModbusMessage> DisplayMessage;
        QSharedPointer<const ModbusMessage> FilterMessage;
        bool IsRequest = false;
    };

    void setupToolbarActions();
    void setupFilterControls();
    void setupToolbarLayout();
    void initializeDisplayDefinition();
    void setupServerConnections();
    void addToolbarSpacer(int width);
    void resetTrafficCounters();
    void setDisplayDefinitionSilent(const TrafficViewDefinitions& dd);
    void updateSourceFilter();
    QString sourceFilterText(const ConnectionDetails& cd) const;
    bool matchesTrafficFilter(const ConnectionDetails& cd,
                              QSharedPointer<const ModbusMessage> filterMsg,
                              QSharedPointer<const ModbusMessage> displayMsg) const;
    void updateExportActionState();
    void clearTrafficLog();
    void flushPendingTrafficUiAll();
    void scheduleTrafficUiFlush();
    void trimTrafficBufferToLimit();
    void rebuildVisibleTraffic();
    void appendTrafficEntry(const ConnectionDetails& cd,
                            const QSharedPointer<const ModbusMessage>& displayMessage,
                            const QSharedPointer<const ModbusMessage>& filterMessage,
                            bool isRequest);

private:
    Ui::FormTrafficView *ui;
    TrafficViewDefinitions _displayDefinition;
    ModbusMultiServer& _mbMultiServer;
    uint _requestCount = 0;
    uint _responseCount = 0;
    LogViewState _logViewState = LogViewState::Unknown;
    QQueue<TrafficLogEntry> _trafficBuffer;
    QQueue<QSharedPointer<const ModbusMessage>> _pendingLogViewUpdates;
    QTimer* _logUiFlushTimer = nullptr;

    QLabel* _labelUnitId = nullptr;
    QSpinBox* _unitIdFilter = nullptr;
    QLabel* _labelFuncCode = nullptr;
    QComboBox* _funcCodeFilter = nullptr;
    QLabel* _labelSource = nullptr;
    QComboBox* _sourceFilter = nullptr;
    QLabel* _labelRowLimit = nullptr;
    QComboBox* _rowLimitCombo = nullptr;
    QCheckBox* _exceptionsFilter = nullptr;
    QCheckBox* _autoscrollCheck = nullptr;
    QWidget* _trafficFilterStretch = nullptr;
};

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QSettings& operator <<(QSettings& out, FormTrafficView* frm)
{
    if(!frm) return out;

    out.setValue("Font", frm->font());
    out.setValue("ForegroundColor", frm->foregroundColor());
    out.setValue("BackgroundColor", frm->backgroundColor());

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMinimized", wnd->isMinimized());
    out.setValue("ViewMaximized", wnd->isMaximized());
    const auto currentRect = (!wnd->isMaximized() && !wnd->isMinimized())
        ? wnd->geometry()
        : frm->property("ParentGeometry").toRect();
    out.setValue("ViewRect", currentRect);

    out << frm->displayDefinition();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QSettings& operator >>(QSettings& in, FormTrafficView* frm)
{
    if(!frm) return in;

    TrafficViewDefinitions displayDefinition;
    in >> displayDefinition;

    bool isMinimized;
    isMinimized = in.value("ViewMinimized").toBool();
    bool isMaximized;
    isMaximized = in.value("ViewMaximized").toBool();

    QRect wndRect;
    wndRect = in.value("ViewRect").toRect();

    frm->setFont(in.value("Font", defaultMonospaceFont()).value<QFont>());
    frm->setForegroundColor(in.value("ForegroundColor", QColor(Qt::black)).value<QColor>());
    frm->setBackgroundColor(in.value("BackgroundColor", QColor(Qt::white)).value<QColor>());

    frm->setProperty("ParentGeometry", wndRect);
    frm->setProperty("AutoCompleteEnabled", AppPreferences::instance().codeAutoComplete());

    auto wnd = frm->parentWidget();
    if(isMinimized) wnd->setWindowState(Qt::WindowMinimized);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDisplayDefinition(displayDefinition);

    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormTrafficView* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormTrafficView");

    const auto panel = frm->property("SplitPanel").toString();
    if(!panel.isEmpty())
        xml.writeAttribute("Panel", panel);
    if(frm->property("Closed").toBool())
        xml.writeAttribute("Closed", "1");

    const auto wnd = frm->parentWidget();
    xml.writeStartElement("Window");
    xml.writeAttribute("Maximized", boolToString(wnd->isMaximized()));
    xml.writeAttribute("Minimized", boolToString(wnd->isMinimized()));

    const auto windowPos = wnd->pos();
    xml.writeAttribute("Left", QString::number(windowPos.x()));
    xml.writeAttribute("Top", QString::number(windowPos.y()));

    const auto windowSize = (wnd->isMinimized() || wnd->isMaximized()) ? wnd->sizeHint() : wnd->size();
    xml.writeAttribute("Width", QString::number(windowSize.width()));
    xml.writeAttribute("Height", QString::number(windowSize.height()));
    xml.writeEndElement();

    xml.writeStartElement("Colors");
    xml.writeAttribute("Background", frm->backgroundColor().name());
    xml.writeAttribute("Foreground", frm->foregroundColor().name());
    xml.writeEndElement();

    xml.writeStartElement("Font");
    const QFont font = frm->font();
    xml.writeAttribute("Family", font.family());
    xml.writeAttribute("Size", QString::number(font.pointSize()));
    xml.writeAttribute("Bold", boolToString(font.bold()));
    xml.writeAttribute("Italic", boolToString(font.italic()));
    xml.writeEndElement();

    const auto dd = frm->displayDefinition();
    xml << dd;

    xml.writeEndElement(); // FormTrafficView

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormTrafficView* frm)
{
    if (!frm) return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("FormTrafficView")) {
        TrafficViewDefinitions dd;
        QHash<quint16, quint16> data;

        const QXmlStreamAttributes attributes = xml.attributes();

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("Window")) {
                const QXmlStreamAttributes windowAttrs = xml.attributes();

                const auto wnd = frm->parentWidget();
                if (wnd) {
                    if(windowAttrs.hasAttribute("Left") && windowAttrs.hasAttribute("Top")) {
                        bool okLeft, okTop;
                        const int left = windowAttrs.value("Left").toInt(&okLeft);
                        const int top = windowAttrs.value("Top").toInt(&okTop);
                        if(okLeft && okTop) {
                            wnd->move(left, top);
                        }
                    }

                    if (windowAttrs.hasAttribute("Width") && windowAttrs.hasAttribute("Height")) {
                        bool okWidth, okHeight;
                        const int width = windowAttrs.value("Width").toInt(&okWidth);
                        const int height = windowAttrs.value("Height").toInt(&okHeight);

                        if (okWidth && okHeight && !wnd->isMaximized() && !wnd->isMinimized()) {
                            wnd->resize(width, height);
                        }
                    }

                    if (windowAttrs.hasAttribute("Maximized")) {
                        const bool maximized = stringToBool(windowAttrs.value("Maximized").toString());
                        if (maximized) wnd->showMaximized();
                    }

                    if (windowAttrs.hasAttribute("Minimized")) {
                        const bool minimized = stringToBool(windowAttrs.value("Minimized").toString());
                        if (minimized) wnd->showMinimized();
                    }


                }
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("Colors")) {
                const QXmlStreamAttributes colorAttrs = xml.attributes();

                if (colorAttrs.hasAttribute("Background")) {
                    QColor color(colorAttrs.value("Background").toString());
                    if (color.isValid()) frm->setBackgroundColor(color);
                }

                if (colorAttrs.hasAttribute("Foreground")) {
                    QColor color(colorAttrs.value("Foreground").toString());
                    if (color.isValid()) frm->setForegroundColor(color);
                }

                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("Font")) {
                const QXmlStreamAttributes fontAttrs = xml.attributes();

                QFont font = frm->font();

                if (fontAttrs.hasAttribute("Family")) {
                    font.setFamily(fontAttrs.value("Family").toString());
                }

                if (fontAttrs.hasAttribute("Size")) {
                    bool ok; const int size = fontAttrs.value("Size").toInt(&ok);
                    if (ok && size > 0) font.setPointSize(size);
                }

                if (fontAttrs.hasAttribute("Bold")) {
                    font.setBold(stringToBool(fontAttrs.value("Bold").toString()));
                }

                if (fontAttrs.hasAttribute("Italic")) {
                    font.setItalic(stringToBool(fontAttrs.value("Italic").toString()));
                }

                frm->setFont(font);
                xml.skipCurrentElement();
            }
            else if(xml.name() == QLatin1String("Zoom")) {
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("TrafficViewDefinitions")) {
                xml >> dd;
                xml.skipCurrentElement();
                frm->setDisplayDefinition(dd);
            }
            else {
                xml.skipCurrentElement();
            }
        }

    }
    else {
        xml.skipCurrentElement();
    }

    return xml;
}

#endif // FORMTRAFFICVIEW_H



