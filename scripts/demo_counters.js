/**************************************************************************/
/*
/* Counters Demo
/* Demonstrates various counter types in Holding Registers
/*
/* Register map (Base1):
/*   HR[1]  - Up counter        (counts up from 0, wraps at HR[5])
/*   HR[2]  - Down counter      (counts down from HR[6] to 0, reloads)
/*   HR[3]  - Up/Down counter   (direction from Coil[1]: ON=up, OFF=down)
/*   HR[4]  - Free-running      (always 0..65535, wraps)
/*   HR[5]  - Up counter limit  (writable, default=100)
/*   HR[6]  - Down counter preset (writable, default=50)
/*   HR[7]  - Step size         (writable, default=1)
/*   HR[8]  - Tick count        (total ticks since start)
/*
/* Coils:
/*   Coil[1] - Up/Down counter direction (ON=up, OFF=down)
/*   Coil[2] - Pause all counters
/*   Coil[3] - Reset all counters to initial values
/*
/* Discrete Inputs (status flags):
/*   DI[1]  - Up counter overflow (pulse for 1 tick)
/*   DI[2]  - Down counter underflow (pulse for 1 tick)
/*   DI[3]  - Up/Down counter direction (mirrors Coil[1])
/*
/* Run mode: periodic (e.g. 200 ms)
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

var deviceId = 1;

var upVal    = 0;
var downVal  = 0;
var udVal    = 0;
var freeVal  = 0;
var tickCount = 0;

var upOverflow  = false;
var downUnder   = false;

function reset()
{
    var preset = Server.readHolding(6, deviceId);
    if(preset <= 0 || preset > 65535) preset = 50;

    upVal    = 0;
    downVal  = preset;
    udVal    = 0;
    freeVal  = 0;
    tickCount = 0;

    upOverflow = false;
    downUnder  = false;

    console.log("Counters reset. Down counter preset=" + preset);
}

function tick()
{
    var pause  = Server.readCoil(2, deviceId);
    var doReset = Server.readCoil(3, deviceId);

    if(doReset)
    {
        reset();
        Server.writeCoil(3, false, deviceId);
        return;
    }

    tickCount++;

    if(pause)
    {
        console.debug("Paused (Coil[2]=ON). Tick " + tickCount + " skipped.");
        return;
    }

    var limit  = Server.readHolding(5, deviceId);
    if(limit <= 0 || limit > 65535) limit = 100;

    var preset = Server.readHolding(6, deviceId);
    if(preset <= 0 || preset > 65535) preset = 50;

    var step   = Server.readHolding(7, deviceId);
    if(step <= 0 || step > 1000) step = 1;

    var dirUp  = Server.readCoil(1, deviceId);  /* true=up, false=down */

    /* --- Up counter --- */
    upOverflow = false;
    upVal += step;
    if(upVal >= limit)
    {
        upVal = 0;
        upOverflow = true;
        console.log("Up counter overflow (limit=" + limit + ")");
    }

    /* --- Down counter --- */
    downUnder = false;
    downVal -= step;
    if(downVal <= 0)
    {
        downVal = preset;
        downUnder = true;
        console.log("Down counter underflow, reloaded to " + preset);
    }

    /* --- Up/Down counter (clamp at 0..65535) --- */
    if(dirUp)
        udVal = Math.min(65535, udVal + step);
    else
        udVal = Math.max(0, udVal - step);

    /* --- Free-running 16-bit counter --- */
    freeVal = (freeVal + step) & 0xFFFF;

    /* --- Write registers --- */
    Server.writeHolding(1, upVal,    deviceId);
    Server.writeHolding(2, downVal,  deviceId);
    Server.writeHolding(3, udVal,    deviceId);
    Server.writeHolding(4, freeVal,  deviceId);
    Server.writeHolding(8, tickCount & 0xFFFF, deviceId);

    /* --- Write status flags --- */
    Server.writeDiscrete(1, upOverflow, deviceId);
    Server.writeDiscrete(2, downUnder,  deviceId);
    Server.writeDiscrete(3, dirUp,      deviceId);
}

function init()
{
    /* Write control register defaults */
    Server.writeHolding(5, 100, deviceId);  /* up limit   */
    Server.writeHolding(6, 50,  deviceId);  /* down preset */
    Server.writeHolding(7, 1,   deviceId);  /* step        */

    /* Default coil states */
    Server.writeCoil(1, true,  deviceId);  /* direction up */
    Server.writeCoil(2, false, deviceId);  /* not paused   */
    Server.writeCoil(3, false, deviceId);  /* no reset      */

    Server.onError(deviceId, function(error) {
        console.error("Error: " + error);
    });

    /* Watch coil changes */
    Server.onChange(deviceId, Register.Coils, 1, function(val) {
        console.log("Up/Down direction: " + (val ? "UP" : "DOWN"));
    });
    Server.onChange(deviceId, Register.Coils, 2, function(val) {
        console.log(val ? "Counters PAUSED" : "Counters RESUMED");
    });

    /* Watch setpoint changes */
    Server.onChange(deviceId, Register.Holding, 5, function(val) {
        console.log("Up counter limit set to " + val);
    });
    Server.onChange(deviceId, Register.Holding, 6, function(val) {
        console.log("Down counter preset set to " + val);
    });
    Server.onChange(deviceId, Register.Holding, 7, function(val) {
        console.log("Step size set to " + val);
    });

    reset();

    console.log("=== Counters Demo started ===");
    console.log("HR[1] Up | HR[2] Down | HR[3] Up/Down | HR[4] Free-running | HR[8] Tick count");
    console.log("HR[5] Up limit | HR[6] Down preset | HR[7] Step");
    console.log("Coil[1] Direction(ON=up) | Coil[2] Pause | Coil[3] Reset");
    console.log("DI[1] UpOverflow | DI[2] DownUnderflow | DI[3] Direction");
}

Script.onInit(init);
tick();
