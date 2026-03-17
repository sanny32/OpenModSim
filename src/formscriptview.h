#ifndef FORMSCRIPTVIEW_H
#define FORMSCRIPTVIEW_H

#include <QWidget>
#include <QTimer>
#include <QXmlStreamWriter>
#include "fontutils.h"
#include "displaydefinition.h"
#include "jscriptcontrol.h"
#include "consoleoutput.h"
#include "apppreferences.h"

///
/// \brief Forward declaration of the MainWindow
///
class MainWindow;
class ModbusMultiServer;
class DataSimulator;
class QTextDocument;
class QCheckBox;
class QSpinBox;
class RunModeComboBox;

namespace Ui {
class FormScriptView;
}

///
/// \brief The FormScriptView class
///
class FormScriptView : public QWidget
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, FormScriptView* frm);
    friend QSettings& operator >>(QSettings& in, FormScriptView* frm);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormScriptView* frm);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormScriptView* frm);

public:
    explicit FormScriptView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent);
    ~FormScriptView();

    int formId() const { return _formId; }

    ScriptViewDefinitions displayDefinition() const;
    void setDisplayDefinition(const ScriptViewDefinitions& dd);
    FormDisplayDefinition displayDefinitionValue() const;
    void setDisplayDefinitionValue(const FormDisplayDefinition& dd);

    ScriptSettings scriptSettings() const;
    void setScriptSettings(const ScriptSettings& ss);

    QString script() const;
    void setScript(const QString& text);
    QTextDocument* scriptDocument() const;
    void setScriptDocument(QTextDocument* document);

    int scriptCursorPosition() const;
    void setScriptCursorPosition(int pos);

    int scriptScrollPosition() const;
    void setScriptScrollPosition(int pos);

    QString searchText() const;

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int zoomPercent() const;
    void setZoomPercent(int zoomPercent);

    bool canRunScript() const;
    bool canStopScript() const;

    bool canUndo() const;
    bool canRedo() const;
    bool canPaste() const;

    void runScript();
    void stopScript();

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

    void saveSettings(QSettings& out) const;
    void loadSettings(QSettings& in);
    void saveXml(QXmlStreamWriter& xml) const;
    void loadXml(QXmlStreamReader& xml);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

public slots:
    void show();
    void connectEditSlots();
    void disconnectEditSlots();

signals:
    void showed();
    void closing();
    void helpContextRequested(const QString& helpKey);
    void scriptSettingsChanged(const ScriptSettings&);
    void scriptRunning();
    void scriptStopped();
    void consoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);

private:
    JScriptControl* scriptControl();

    void setupScriptBar();
    void updateScriptBar();

private:
    Ui::FormScriptView *ui;
    MainWindow* _parent;
    int _formId;
    ScriptSettings _scriptSettings;
    ScriptViewDefinitions _displayDefinition;

    RunModeComboBox* _scriptRunModeCombo = nullptr;
    QSpinBox* _scriptIntervalSpin = nullptr;
    QCheckBox* _scriptRunOnStartupCheck = nullptr;
};

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QSettings& operator <<(QSettings& out, FormScriptView* frm)
{
    if(!frm) return out;

    out.setValue("Font", frm->font());
    out.setValue("ForegroundColor", frm->foregroundColor());
    out.setValue("BackgroundColor", frm->backgroundColor());
    out.setValue("ZoomPercent", frm->zoomPercent());

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMinimized", wnd->isMinimized());
    out.setValue("ViewMaximized", wnd->isMaximized());
    out.setValue("ViewRect", wnd->geometry());

    out << frm->displayDefinition();
    out << frm->scriptControl();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QSettings& operator >>(QSettings& in, FormScriptView* frm)
{
    if(!frm) return in;

    ScriptViewDefinitions displayDefinition;
    in >> displayDefinition;

    in >> frm->scriptControl();

    bool isMinimized;
    isMinimized = in.value("ViewMinimized").toBool();
    bool isMaximized;
    isMaximized = in.value("ViewMaximized").toBool();

    QRect wndRect;
    wndRect = in.value("ViewRect").toRect();

    auto wnd = frm->parentWidget();
    frm->setFont(in.value("Font", defaultMonospaceFont()).value<QFont>());
    frm->setForegroundColor(in.value("ForegroundColor", QColor(Qt::black)).value<QColor>());
    frm->setBackgroundColor(in.value("BackgroundColor", QColor(Qt::white)).value<QColor>());
    frm->setZoomPercent(in.value("ZoomPercent", 100).toInt());
    frm->setFont(AppPreferences::instance().scriptFont());
    if(wndRect.isValid())
        wnd->setGeometry(wndRect);
    if(isMinimized) wnd->setWindowState(Qt::WindowMinimized);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDisplayDefinition(displayDefinition);

    if(displayDefinition.ScriptCfg.RunOnStartup) {
        frm->runScript();
    }

    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, FormScriptView* frm)
{
    if (!frm) return xml;

    xml.writeStartElement("FormScriptView");

    const auto panel = frm->property("SplitPanel").toString();
    if(!panel.isEmpty())
        xml.writeAttribute("Panel", panel);

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

    xml.writeStartElement("Zoom");
    xml.writeAttribute("Value", QString("%1%").arg(frm->zoomPercent()));
    xml.writeEndElement();

    const auto dd = frm->displayDefinition();
    xml << dd;

    xml << frm->scriptControl();

    xml.writeEndElement(); // FormScriptView

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param frm
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, FormScriptView* frm)
{
    if (!frm) return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("FormScriptView")) {
        ScriptViewDefinitions dd;

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
                const QXmlStreamAttributes zoomAttrs = xml.attributes();
                if (zoomAttrs.hasAttribute("Value")) {
                    bool ok; const int zoom = zoomAttrs.value("Value").toString().remove("%").toInt(&ok);
                    if(ok) frm->setZoomPercent(zoom);
                    xml.skipCurrentElement();
                }
            }
            else if (xml.name() == QLatin1String("ScriptViewDefinitions")) {
                xml >> dd;
                frm->setDisplayDefinition(dd);
            }
            else if (xml.name() == QLatin1String("JScriptControl")) {
                auto scriptControl = frm->scriptControl();
                xml >> scriptControl;
            }
            else {
                xml.skipCurrentElement();
            }
        }

        frm->setFont(AppPreferences::instance().scriptFont());

        if(dd.ScriptCfg.RunOnStartup) {
            frm->runScript();
        }
    }
    else {
        xml.skipCurrentElement();
    }

    return xml;
}

#endif // FORMSCRIPTVIEW_H



