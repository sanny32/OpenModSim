#ifndef ERRORSIMULATIONS_H
#define ERRORSIMULATIONS_H

#include <QObject>
#include "modbusmultiserver.h"

///
/// \brief The ErrorSimulations class
///
class ErrorSimulations : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int responseDelayTime READ responseDelayTime WRITE setResponseDelayTime NOTIFY responseDelayTimeChanged)
    Q_PROPERTY(int responseRandomDelayUpToTime READ responseRandomDelayUpToTime WRITE setResponseRandomDelayUpToTime NOTIFY responseRandomDelayUpToTimeChanged)
    Q_PROPERTY(bool noResponse READ noResponse WRITE setNoResponse NOTIFY noResponseChanged)
    Q_PROPERTY(bool responseIncorrectId READ responseIncorrectId WRITE setResponseIncorrectId NOTIFY responseIncorrectIdChanged)
    Q_PROPERTY(bool responseIllegalFunction READ responseIllegalFunction WRITE setResponseIllegalFunction NOTIFY responseIllegalFunctionChanged)
    Q_PROPERTY(bool responseDeviceBusy READ responseDeviceBusy WRITE setResponseDeviceBusy NOTIFY responseDeviceBusyChanged)
    Q_PROPERTY(bool responseIncorrectCrc READ responseIncorrectCrc WRITE setResponseIncorrectCrc NOTIFY responseIncorrectCrcChanged)
    Q_PROPERTY(bool responseDelay READ responseDelay WRITE setResponseDelay NOTIFY responseDelayChanged)
    Q_PROPERTY(bool responseRandomDelay READ responseRandomDelay WRITE setResponseRandomDelay NOTIFY responseRandomDelayChanged)

public:
    explicit ErrorSimulations(ModbusMultiServer* server);

    int  responseDelayTime() const { return _responseDelayTime; }
    int  responseRandomDelayUpToTime() const { return _responseRandomDelayUpToTime; }

    bool noResponse() const { return _noResponse; }
    bool responseIncorrectId() const { return _responseIncorrectId; }
    bool responseIllegalFunction() const { return _responseIllegalFunction; }
    bool responseDeviceBusy() const { return _responseDeviceBusy; }
    bool responseIncorrectCrc() const { return _responseIncorrectCrc; }
    bool responseDelay() const { return _responseDelay; }
    bool responseRandomDelay() const { return _responseRandomDelay; }

    void setResponseDelayTime(int value);
    void setResponseRandomDelayUpToTime(int value);
    void setNoResponse(bool value);
    void setResponseIncorrectId(bool value);
    void setResponseIllegalFunction(bool value);
    void setResponseDeviceBusy(bool value);
    void setResponseIncorrectCrc(bool value);
    void setResponseDelay(bool value);
    void setResponseRandomDelay(bool value);

signals:
    void responseDelayTimeChanged(int);
    void responseRandomDelayUpToTimeChanged(int);

    void noResponseChanged(bool);
    void responseIncorrectIdChanged(bool);
    void responseIllegalFunctionChanged(bool);
    void responseDeviceBusyChanged(bool);
    void responseIncorrectCrcChanged(bool);
    void responseDelayChanged(bool);
    void responseRandomDelayChanged(bool);

private slots:
    void updateServerDefinitions();

private:
    ModbusMultiServer* _mbMultiServer;

    int  _responseDelayTime = 0;
    int  _responseRandomDelayUpToTime = 1000;

    bool _noResponse = false;
    bool _responseIncorrectId = false;
    bool _responseIllegalFunction = false;
    bool _responseDeviceBusy = false;
    bool _responseIncorrectCrc = false;
    bool _responseDelay = false;
    bool _responseRandomDelay = false;
};

#endif // ERRORSIMULATIONS_H
