/**************************************************************************/
/*
/* PLC I/O Simulator Demo
/* Simulates a simple PLC with digital/analog inputs and outputs
/*
/* Coils (outputs, writable by client):
/*   Coil[1]  - Motor Start command (momentary, auto-cleared)
/*   Coil[2]  - Motor Stop command  (momentary, auto-cleared; priority over Start)
/*   Coil[3]  - Pump Enable         (level signal)
/*   Coil[4]  - Alarm Reset         (momentary, auto-cleared)
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

var FAULT_SECONDS = 30;   /* motor faults after running this long without a stop command */

function tick()
{
    /* The whole script is re-run every period, so top-level variables would
       reset on each tick. Values exposed as registers are read back from there;
       only the internal fault timer has no register and is kept in Storage. */
    var motorRunning  = Server.readDiscrete(1, deviceId);
    var motorFault    = Server.readDiscrete(2, deviceId);
    var tankLevel     = Server.readInput(1, deviceId);
    var actualRpm     = Server.readInput(2, deviceId);
    var motorStartMs  = Storage.getItem("motorStartMs");  /* wall-clock time the motor started, 0 if stopped */

    /* --- Read coil commands --- */
    var cmdStart  = Server.readCoil(1, deviceId);
    var cmdStop   = Server.readCoil(2, deviceId);
    var cmdPump   = Server.readCoil(3, deviceId);  /* level signal */
    var cmdReset  = Server.readCoil(4, deviceId);

    /* Start/Stop/Reset are momentary: consume them so they act as one-shot
       pulses (a held coil must not retrigger every tick). */
    if(cmdStart) Server.writeCoil(1, false, deviceId);
    if(cmdStop)  Server.writeCoil(2, false, deviceId);
    if(cmdReset) Server.writeCoil(4, false, deviceId);

    /* --- Alarm reset (only meaningful while a fault is latched) --- */
    var justReset = false;
    if(cmdReset && motorFault)
    {
        motorFault    = false;
        motorStartMs  = 0;
        actualRpm     = 0;
        justReset     = true;
        console.log("Alarm reset");
    }

    /* --- Motor logic: Stop has priority over Start; ignore commands on the
           same tick as a reset so that a fresh Start is required to restart. --- */
    if(!motorFault && !justReset)
    {
        if(cmdStop)
        {
            if(motorRunning)
            {
                motorRunning = false;
                actualRpm    = 0;
                console.log("Motor stopped");
            }
            motorStartMs = 0;
        }
        else if(cmdStart && !motorRunning)
        {
            motorRunning = true;
            motorStartMs = Date.now();
            console.log("Motor started");
        }
    }

    /* --- Fault on overrun, measured by actual elapsed time (period-independent) --- */
    if(motorRunning && motorStartMs > 0
        && (Date.now() - motorStartMs) >= FAULT_SECONDS * 1000)
    {
        motorFault   = true;
        motorRunning = false;
        actualRpm    = 0;
        motorStartMs = 0;
        console.warning("Motor FAULT: timeout without stop command!");
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

    /* --- Pump logic (derived each tick, no need to persist) --- */
    var pumpRunning = cmdPump && !motorFault;

    /* --- Tank level simulation --- */
    var hiLevel = Server.readHolding(1, deviceId);
    var loLevel = Server.readHolding(2, deviceId);
    if(loLevel >= hiLevel)   /* keep thresholds ordered, otherwise both alarms could latch */
    {
        hiLevel = 800;
        loLevel = 200;
        /* Correct the registers too, so the client sees the valid setpoints */
        Server.writeHolding(1, hiLevel, deviceId);
        Server.writeHolding(2, loLevel, deviceId);
    }

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

    /* --- Persist the internal fault timer (everything else lives in registers) --- */
    Storage.setItem("motorStartMs", motorStartMs);
}

function init()
{
    /* Seed initial state: registers are the source of truth, the internal
       fault timer lives in Storage */
    Server.writeDiscrete(1, false, deviceId);  /* motor running */
    Server.writeDiscrete(2, false, deviceId);  /* motor fault   */
    Server.writeInput(1, 500, deviceId);        /* tank level    */
    Server.writeInput(2, 0,   deviceId);        /* actual rpm    */
    Storage.setItem("motorStartMs", 0);

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
