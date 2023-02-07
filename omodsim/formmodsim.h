#ifndef FORMMODSIM_H
#define FORMMODSIM_H

#include <QWidget>
#include <QTimer>
#include <QPrinter>
#include "modbusmultiserver.h"
#include "displaydefinition.h"

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

public:
    explicit FormModSim(int id, ModbusMultiServer& server, MainWindow* parent);
    ~FormModSim();

    int formId() const { return _formId; }

    QString filename() const;
    void setFilename(const QString& filename);

    DisplayDefinition displayDefinition() const;
    void setDisplayDefinition(const DisplayDefinition& dd);

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

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

    void resetCtrs();
    uint numberOfPolls() const;
    uint validSlaveResposes() const;

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void show();

signals:
    void showed();
    void closing();
    void numberOfPollsChanged(uint value);
    void validSlaveResposesChanged(uint value);

private slots:
    void on_timeout();
    void on_lineEditAddress_valueChanged(const QVariant&);
    void on_lineEditLength_valueChanged(const QVariant&);
    void on_lineEditDeviceId_valueChanged(const QVariant&);
    void on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value);
    void on_outputWidget_itemDoubleClicked(quint32 addr, const QVariant& value);
    void on_statisticWidget_numberOfPollsChanged(uint value);
    void on_statisticWidget_validSlaveResposesChanged(uint value);
    void on_mbDeviceIdChanged(quint8 deviceId);
    void on_mbConnected(const ConnectionDetails& cd);
    void on_mbDisconnected(const ConnectionDetails& cd);

private:
    Ui::FormModSim *ui;
    int _formId;
    QTimer _timer;
    QString _filename;
    ModbusMultiServer& _mbMultiServer;
    QMap<quint16, ModbusSimulationParams> _simulationMap;
};

#endif // FORMMODSIM_H
