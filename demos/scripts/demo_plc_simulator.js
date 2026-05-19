/**************************************************************************/
/*
/* PLC I/O Simulator Demo
/* Simulates a simple PLC with digital/analog inputs and outputs
/*
/* Coils (outputs, writable by client):
/*   Coil[1]  - Motor Start command
/*   Coil[2]  - Motor Stop command
/*   Coil[3]  - Pump Enable
/*   Coil[4]  - Alarm Reset
/*
/* Discrete Inputs (read-only status):
/*   DI[1]  - Motor Running (set automatically from coils)
/*   DI[2]  - Motor Fault (set if running > 30 sec without stop)
/*   DI[3]  - Pump Running
/*   DI[4]  - High Level Alarm
/*   DI[5]  - Low Level Alarm
/*
/* Holding Registers (writable setpoints):
/*   HR[1]  - Setpoint: Tank High Level (default 800)
/*   HR[2]  - Setpoint: Tank Low Level  (default 200)
/*   HR[3]  - Motor Speed Setpoint RPM  (0..1500)
/*
/* Input Registers (process values, read-only):
/*   IR[1]  - Tank Level   (0..1000, simulated)
/*   IR[2]  - Motor Speed  (actual RPM, follows setpoint with ramp)
/*   IR[3]  - Motor Current (simulated 0..100, 10ths of Amp)
/*   IR[4]  - Pump Flow    (0..500 L/min)
/*
/* Run mode: periodic (e.g. 500 ms)
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

var deviceId = 1;

var motorRunning  = false;
var motorFault    = false;
var motorRunTicks = 0;
var actualRpm     = 0;
var tankLevel     = 500;
var pumpRunning   = false;

var FAULT_TICKS   = 60;   /* fault after 60 ticks without stop */

function tick()
{
    /* --- Read coil commands --- */
    var cmdStart  = Server.readCoil(1, deviceId);
    var cmdStop   = Server.readCoil(2, deviceId);
    var cmdPump   = Server.readCoil(3, deviceId);
    var cmdReset  = Server.readCoil(4, deviceId);

    /* --- Alarm reset --- */
    if(cmdReset && motorFault)
    {
        motorFault    = false;
        motorRunning  = false;
        motorRunTicks = 0;
        actualRpm     = 0;
        Server.writeCoil(4, false, deviceId);
        console.log("Alarm reset");
    }

    /* --- Motor logic --- */
    if(!motorFault)
    {
        if(cmdStart && !motorRunning)
        {
            motorRunning = true;
            motorRunTicks = 0;
            console.log("Motor started");
        }
        if(cmdStop && motorRunning)
        {
            motorRunning  = false;
            motorRunTicks = 0;
            actualRpm     = 0;
            console.log("Motor stopped");
        }
    }

    if(motorRunning)
    {
        motorRunTicks++;
        if(motorRunTicks >= FAULT_TICKS)
        {
            motorFault   = true;
            motorRunning = false;
            actualRpm    = 0;
            console.warning("Motor FAULT: timeout without stop command!");
        }
    }

    /* --- Speed ramp --- */
    var setpointRpm = Server.readHolding(3, deviceId);
    if(setpointRpm < 0)   setpointRpm = 0;
    if(setpointRpm > 1500) setpointRpm = 1500;

    var targetRpm = motorRunning ? setpointRpm : 0;
    var ramp = 50; /* RPM per tick */
    if(actualRpm < targetRpm)
        actualRpm = Math.min(actualRpm + ramp, targetRpm);
    else if(actualRpm > targetRpm)
        actualRpm = Math.max(actualRpm - ramp, targetRpm);

    /* --- Pump logic --- */
    pumpRunning = cmdPump && !motorFault;

    /* --- Tank level simulation --- */
    var hiLevel = Server.readHolding(1, deviceId);
    var loLevel = Server.readHolding(2, deviceId);

    /* Pump drains tank, time fills it slowly */
    if(pumpRunning)
        tankLevel -= 15;
    else
        tankLevel += 5;

    tankLevel += (Math.random() - 0.5) * 10;
    tankLevel = Math.max(0, Math.min(1000, Math.round(tankLevel)));

    var hiAlarm = (tankLevel >= hiLevel);
    var loAlarm = (tankLevel <= loLevel);

    /* --- Motor current simulation --- */
    var motorCurrent = motorRunning
        ? Math.round(20 + (actualRpm / 1500) * 60 + (Math.random() - 0.5) * 10)
        : 0;

    /* --- Pump flow simulation --- */
    var pumpFlow = pumpRunning
        ? Math.round(200 + (Math.random() - 0.5) * 50)
        : 0;

    /* --- Write outputs --- */
    Server.writeDiscrete(1, motorRunning,  deviceId);
    Server.writeDiscrete(2, motorFault,    deviceId);
    Server.writeDiscrete(3, pumpRunning,   deviceId);
    Server.writeDiscrete(4, hiAlarm,       deviceId);
    Server.writeDiscrete(5, loAlarm,       deviceId);

    Server.writeInput(1, tankLevel,     deviceId);
    Server.writeInput(2, actualRpm,     deviceId);
    Server.writeInput(3, motorCurrent,  deviceId);
    Server.writeInput(4, pumpFlow,      deviceId);
}

function init()
{
    /* Default setpoints */
    Server.writeHolding(1, 800, deviceId);  /* high level */
    Server.writeHolding(2, 200, deviceId);  /* low level  */
    Server.writeHolding(3, 900, deviceId);  /* speed setpoint */

    /* Initial coil states */
    Server.writeCoil(1, false, deviceId);
    Server.writeCoil(2, false, deviceId);
    Server.writeCoil(3, false, deviceId);
    Server.writeCoil(4, false, deviceId);

    Server.onError(deviceId, function(error) {
        console.error("Communication error: " + error);
    });

    console.log("=== PLC Simulator started ===");
    console.log("Coils: [1]Start [2]Stop [3]PumpEnable [4]AlarmReset");
    console.log("DI:    [1]MotorRunning [2]MotorFault [3]PumpRunning [4]HiAlarm [5]LoAlarm");
    console.log("HR:    [1]HiLevel(800) [2]LoLevel(200) [3]SpeedSP(900 RPM)");
    console.log("IR:    [1]TankLevel [2]ActualRPM [3]Current(x0.1A) [4]PumpFlow(L/min)");
}

Script.onInit(init);
tick();
