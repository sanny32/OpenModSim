/**************************************************************************/
/*
/* Counters Demo
/* Demonstrates various counter types in Holding Registers
/*
/* Register map (Base1):
/*   HR[1]  - Up counter        (counts up from 0, wraps modulo HR[5], keeps remainder)
/*   HR[2]  - Down counter      (counts down from HR[6], reloads to HR[6] when it reaches or passes 0)
/*   HR[3]  - Up/Down counter   (direction from Coil[1]: ON=up, OFF=down)
/*   HR[4]  - Free-running      (always 0..65535, wraps)
/*   HR[5]  - Up counter limit  (writable, default=100)
/*   HR[6]  - Down counter preset (writable, default=50)
/*   HR[7]  - Step size         (writable, default=1)
/*   HR[8]  - Tick count        (total ticks since start, keeps running on pause)
/*
/* Coils:
/*   Coil[1] - Up/Down counter direction (ON=up, OFF=down)
/*   Coil[2] - Pause value counters HR[1..4] (tick count HR[8] keeps running)
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

function reset()
{
    var preset = Server.readHolding(6, deviceId);
    if(preset <= 0 || preset > 65535) preset = 50;

    /* The counters live in their Holding registers, so reset writes them there.
       The whole script is re-run every period, so plain top-level variables
       would not persist - the registers are the source of truth. */
    Server.writeHolding(1, 0,      deviceId);
    Server.writeHolding(2, preset, deviceId);
    Server.writeHolding(3, 0,      deviceId);
    Server.writeHolding(4, 0,      deviceId);
    Server.writeHolding(8, 0,      deviceId);

    console.log("Counters reset. Down counter preset=" + preset);
}

///
/// Writes the status flags. The overflow/underflow flags are one-tick pulses,
/// so they must be refreshed (and cleared) on every tick, including the early
/// return paths, otherwise a pulse could stay latched.
///
function writeStatusFlags(upOverflow, downUnder, dirUp)
{
    Server.writeDiscrete(1, upOverflow, deviceId);
    Server.writeDiscrete(2, downUnder,  deviceId);
    Server.writeDiscrete(3, dirUp,      deviceId);
}

function tick()
{
    var pause  = Server.readCoil(2, deviceId);
    var doReset = Server.readCoil(3, deviceId);
    var dirUp  = Server.readCoil(1, deviceId);  /* true=up, false=down */

    if(doReset)
    {
        reset();
        Server.writeCoil(3, false, deviceId);
        writeStatusFlags(false, false, dirUp);
        return;
    }

    /* --- Load counters from the registers they live in --- */
    var upVal   = Server.readHolding(1, deviceId);
    var downVal = Server.readHolding(2, deviceId);
    var udVal   = Server.readHolding(3, deviceId);
    var freeVal = Server.readHolding(4, deviceId);

    var tickCount = (Server.readHolding(8, deviceId) + 1) & 0xFFFF;
    Server.writeHolding(8, tickCount, deviceId);

    if(pause)
    {
        console.debug("Paused (Coil[2]=ON). Tick " + tickCount + " skipped.");
        writeStatusFlags(false, false, dirUp);
        return;
    }

    var upOverflow = false;
    var downUnder  = false;

    var limit  = Server.readHolding(5, deviceId);
    if(limit <= 0 || limit > 65535) limit = 100;

    var preset = Server.readHolding(6, deviceId);
    if(preset <= 0 || preset > 65535) preset = 50;

    var step   = Server.readHolding(7, deviceId);
    if(step <= 0 || step > 1000) step = 1;

    /* --- Up counter (wraps modulo limit, keeping the remainder) --- */
    upVal += step;
    if(upVal >= limit)
    {
        upVal %= limit;
        upOverflow = true;
        console.log("Up counter overflow (limit=" + limit + ")");
    }

    /* --- Down counter --- */
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

    /* --- Write registers (the counters' source of truth) --- */
    Server.writeHolding(1, upVal,    deviceId);
    Server.writeHolding(2, downVal,  deviceId);
    Server.writeHolding(3, udVal,    deviceId);
    Server.writeHolding(4, freeVal,  deviceId);

    /* --- Write status flags --- */
    writeStatusFlags(upOverflow, downUnder, dirUp);
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
