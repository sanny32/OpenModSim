#ifndef FORMMODSIM_H
#define FORMMODSIM_H

#include <QWidget>
#include <QSettings>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QPrinter>
#include "displaydefinition.h"
#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "controls/outputtypes.h"
#include "controls/jscriptcontrol.h"
#include "controls/consoleoutput.h"

class MainWindow;
class QTextDocument;

class FormModSim : public QWidget
{
    Q_OBJECT
public:
    enum class FormKind
    {
        Data = 0,
        Traffic,
        Script
    };

    ~FormModSim() override;

protected:
    explicit FormModSim(QWidget* parent = nullptr);

public:

    virtual FormKind formKind() const = 0;

    virtual int formId() const = 0;

    virtual QString filename() const = 0;
    virtual void setFilename(const QString& filename) = 0;

    virtual QVector<quint16> data() const = 0;

    virtual DisplayDefinition displayDefinition() const = 0;
    virtual void setDisplayDefinition(const DisplayDefinition& dd) = 0;

    virtual ByteOrder byteOrder() const = 0;
    virtual void setByteOrder(ByteOrder order) = 0;

    virtual QString codepage() const = 0;
    virtual void setCodepage(const QString& name) = 0;

    virtual DisplayMode displayMode() const = 0;
    virtual void setDisplayMode(DisplayMode mode) = 0;

    virtual DataDisplayMode dataDisplayMode() const = 0;
    virtual void setDataDisplayMode(DataDisplayMode mode) = 0;

    virtual ScriptSettings scriptSettings() const = 0;
    virtual void setScriptSettings(const ScriptSettings& ss) = 0;

    virtual QString script() const = 0;
    virtual void setScript(const QString& text) = 0;
    virtual QTextDocument* scriptDocument() const = 0;
    virtual void setScriptDocument(QTextDocument* document) = 0;

    virtual int scriptCursorPosition() const = 0;
    virtual void setScriptCursorPosition(int pos) = 0;

    virtual int scriptScrollPosition() const = 0;
    virtual void setScriptScrollPosition(int pos) = 0;

    virtual QString searchText() const = 0;

    virtual bool displayHexAddresses() const = 0;
    virtual void setDisplayHexAddresses(bool on) = 0;

    virtual CaptureMode captureMode() const = 0;
    virtual void startTextCapture(const QString& file) = 0;
    virtual void stopTextCapture() = 0;

    virtual QColor backgroundColor() const = 0;
    virtual void setBackgroundColor(const QColor& clr) = 0;

    virtual QColor foregroundColor() const = 0;
    virtual void setForegroundColor(const QColor& clr) = 0;

    virtual QColor statusColor() const = 0;
    virtual void setStatusColor(const QColor& clr) = 0;

    virtual QFont font() const = 0;
    virtual void setFont(const QFont& font) = 0;

    virtual int zoomPercent() const = 0;
    virtual void setZoomPercent(int zoomPercent) = 0;

    virtual void print(QPrinter* painter) = 0;

    virtual ModbusSimulationMap2 simulationMap() const = 0;
    virtual void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params) = 0;

    virtual QModbusDataUnit serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const = 0;
    virtual void configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const = 0;

    virtual AddressDescriptionMap2 descriptionMap() const = 0;
    virtual void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc) = 0;

    virtual AddressColorMap colorMap() const = 0;
    virtual void setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr) = 0;

    virtual void resetCtrs() = 0;
    virtual uint requestCount() const = 0;
    virtual uint responseCount() const = 0;
    virtual void setStatisticCounters(uint requests, uint responses) = 0;

    virtual bool canRunScript() const = 0;
    virtual bool canStopScript() const = 0;

    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual bool canPaste() const = 0;

    virtual void runScript() = 0;
    virtual void stopScript() = 0;

    virtual LogViewState logViewState() const = 0;
    virtual void setLogViewState(LogViewState state) = 0;

    virtual QRect parentGeometry() const = 0;
    virtual void setParentGeometry(const QRect& geometry) = 0;

    virtual bool isAutoCompleteEnabled() const = 0;
    virtual void enableAutoComplete(bool enable) = 0;

    virtual void setScriptFont(const QFont& font) = 0;

    virtual void saveSettings(QSettings& out) const = 0;
    virtual void loadSettings(QSettings& in) = 0;
    virtual void saveXml(QXmlStreamWriter& xml) const = 0;
    virtual void loadXml(QXmlStreamReader& xml) = 0;

public slots:
    virtual void show() = 0;
    virtual void connectEditSlots() = 0;
    virtual void disconnectEditSlots() = 0;

signals:
    void showed();
    void closing();
    void helpContextRequested(const QString& helpKey);
    void byteOrderChanged(ByteOrder);
    void codepageChanged(const QString&);
    void definitionChanged();
    void pointTypeChanged(QModbusDataUnit::RegisterType);
    void displayModeChanged(DisplayMode mode);
    void scriptSettingsChanged(const ScriptSettings&);
    void scriptRunning();
    void scriptStopped();
    void consoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);
    void captureError(const QString& error);
    void doubleClicked();
    void statisticCtrsReseted();
    void statisticLogStateChanged(LogViewState state);
};

#endif // FORMMODSIM_H
