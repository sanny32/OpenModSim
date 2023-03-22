#ifndef FORMMODSIM_H
#define FORMMODSIM_H

#include <QWidget>
#include <QTimer>
#include <QPrinter>
#include <QVersionNumber>
#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "displaydefinition.h"
#include "scriptsettings.h"

class MainWindow;

namespace Ui {
class FormModSim;
}

///
/// \brief The FormModSim class
///
class FormModSim : public QWidget
{
    Q_OBJECT

    friend QDataStream& operator <<(QDataStream& out, const FormModSim* frm);
    friend QDataStream& operator >>(QDataStream& in, FormModSim* frm);

public:
    static QVersionNumber VERSION;

    explicit FormModSim(int id, ModbusMultiServer& server, QSharedPointer<DataSimulator> simulator, MainWindow* parent);
    ~FormModSim();

    int formId() const { return _formId; }

    QString filename() const;
    void setFilename(const QString& filename);

    QVector<quint16> data() const;

    DisplayDefinition displayDefinition() const;
    void setDisplayDefinition(const DisplayDefinition& dd);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder order);

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    ScriptSettings scriptSettings() const;
    void setScriptSettings(const ScriptSettings& ss);

    QString script() const;
    void setScript(const QString& text);

    QString searchText() const;

    bool displayHexAddresses() const;
    void setDisplayHexAddresses(bool on);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor statusColor() const;
    void setStatusColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    void print(QPrinter* painter);

    ModbusSimulationMap simulationMap() const;
    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);

    bool canRunScript() const;
    bool canStopScript();

    void runScript();
    void stopScript();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    void show();
    void connectEditSlots();
    void disconnectEditSlots();

signals:
    void showed();
    void closing();
    void byteOrderChanged(ByteOrder);
    void displayModeChanged(DisplayMode mode);
    void scriptSettingsChanged(const ScriptSettings&);

private slots:
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value);
    void on_mbDeviceIdChanged(quint8 deviceId);
    void on_mbConnected(const ConnectionDetails& cd);
    void on_mbDisconnected(const ConnectionDetails& cd);
    void on_mbRequest(const QModbusRequest& req);
    void on_mbResponse(const QModbusResponse& resp);
    void on_mbDataChanged(const QModbusDataUnit& data);
    void on_simulationStarted(QModbusDataUnit::RegisterType type, quint16 addr);
    void on_simulationStopped(QModbusDataUnit::RegisterType type, quint16 addr);
    void on_dataSimulated(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value);

private:
    void updateStatus();
    void onDefinitionChanged();

private:
    Ui::FormModSim *ui;
    MainWindow* _parent;
    int _formId;
    QString _filename;
    ScriptSettings _scriptSettings;
    ModbusMultiServer& _mbMultiServer;
    QSharedPointer<DataSimulator> _dataSimulator;
};

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QSettings& operator <<(QSettings& out, const FormModSim* frm)
{
    if(!frm) return out;

    out.setValue("Font", frm->font());
    out.setValue("ForegroundColor", frm->foregroundColor());
    out.setValue("BackgroundColor", frm->backgroundColor());
    out.setValue("StatusColor", frm->statusColor());

    const auto wnd = frm->parentWidget();
    out.setValue("ViewMaximized", wnd->isMaximized());
    if(!wnd->isMaximized() && !wnd->isMinimized())
    {
        out.setValue("ViewSize", wnd->size());
    }

    out << frm->displayMode();
    out << frm->dataDisplayMode();
    out << frm->byteOrder();
    out << frm->displayDefinition();
    out.setValue("DisplayHexAddresses", frm->displayHexAddresses());
    out << frm->scriptSettings();
    out.setValue("Script", frm->script().toUtf8().toBase64());

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QSettings& operator >>(QSettings& in, FormModSim* frm)
{
    if(!frm) return in;

    DisplayMode displayMode;
    in >> displayMode;

    DataDisplayMode dataDisplayMode;
    in >> dataDisplayMode;

    ByteOrder byteOrder;
    in >> byteOrder;

    DisplayDefinition displayDefinition;
    in >> displayDefinition;

    ScriptSettings scriptSettings;
    in >> scriptSettings;

    bool isMaximized;
    isMaximized = in.value("ViewMaximized").toBool();

    QSize wndSize;
    wndSize = in.value("ViewSize").toSize();

    auto wnd = frm->parentWidget();
    frm->setFont(in.value("Font", wnd->font()).value<QFont>());
    frm->setForegroundColor(in.value("ForegroundColor", QColor(Qt::black)).value<QColor>());
    frm->setBackgroundColor(in.value("BackgroundColor", QColor(Qt::lightGray)).value<QColor>());
    frm->setStatusColor(in.value("StatusColor", QColor(Qt::red)).value<QColor>());

    wnd->resize(wndSize);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDisplayMode(displayMode);
    frm->setDataDisplayMode(dataDisplayMode);
    frm->setByteOrder(byteOrder);
    frm->setDisplayDefinition(displayDefinition);
    frm->setDisplayHexAddresses(in.value("DisplayHexAddresses").toBool());
    frm->setScriptSettings(scriptSettings);

    const auto script = QByteArray::fromBase64(in.value("Script").toString().toUtf8());
    if(!script.isEmpty()) frm->setScript(script);

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const FormModSim* frm)
{
    if(!frm) return out;

    out << frm->formId();

    const auto wnd = frm->parentWidget();
    out << wnd->isMaximized();
    out << ((wnd->isMinimized() || wnd->isMaximized()) ?
              wnd->sizeHint() : wnd->size());

    out << frm->displayMode();
    out << frm->dataDisplayMode();
    out << frm->displayHexAddresses();

    out << frm->backgroundColor();
    out << frm->foregroundColor();
    out << frm->statusColor();
    out << frm->font();
    out << frm->displayDefinition();
    out << frm->byteOrder();
    out << frm->simulationMap();
    out << frm->script();
    out << frm->scriptSettings();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param frm
/// \return
///
inline QDataStream& operator >>(QDataStream& in, FormModSim* frm)
{
    if(!frm) return in;

    bool isMaximized;
    in >> isMaximized;

    QSize windowSize;
    in >> windowSize;

    DisplayMode displayMode;
    in >> displayMode;

    DataDisplayMode dataDisplayMode;
    in >> dataDisplayMode;

    bool hexAddresses;
    in >> hexAddresses;

    QColor bkgClr;
    in >> bkgClr;

    QColor fgClr;
    in >> fgClr;

    QColor stCrl;
    in >> stCrl;

    QFont font;
    in >> font;

    DisplayDefinition dd;
    in >> dd;

    const auto ver = frm->property("Version").value<QVersionNumber>();

    ModbusSimulationMap simulationMap;
    ByteOrder byteOrder = ByteOrder::LittleEndian;
    if(ver >= QVersionNumber(1, 1))
    {
        in >> byteOrder;
        in >> simulationMap;
    }

    QString script;
    ScriptSettings scriptSettings;
    if(ver >=  QVersionNumber(1, 2))
    {
        in >> script;
        in >> scriptSettings;
    }

    if(in.status() != QDataStream::Ok)
        return in;

    auto wnd = frm->parentWidget();
    wnd->resize(windowSize);
    wnd->setWindowState(Qt::WindowActive);
    if(isMaximized) wnd->setWindowState(Qt::WindowMaximized);

    frm->setDisplayMode(displayMode);
    frm->setDataDisplayMode(dataDisplayMode);
    frm->setDisplayHexAddresses(hexAddresses);
    frm->setBackgroundColor(bkgClr);
    frm->setForegroundColor(fgClr);
    frm->setStatusColor(stCrl);
    frm->setFont(font);
    frm->setDisplayDefinition(dd);
    frm->setByteOrder(byteOrder);
    frm->setScript(script);
    frm->setScriptSettings(scriptSettings);

    for(auto&& k : simulationMap.keys())
        frm->startSimulation(k.first, k.second,  simulationMap[k]);

    return in;
}

#endif // FORMMODSIM_H
