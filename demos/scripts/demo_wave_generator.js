/**************************************************************************/
/*
/* Wave Generator Demo
/* Generates sine, cosine, sawtooth and square waves in Holding Registers
/*
/* All waves are scaled by HR[8] amplitude around the 32767 midpoint:
/* 100% gives the full ranges below, 0% collapses every wave to 32767.
/*
/* Register map (Base1), values shown at 100% amplitude:
/*   HR[1]  - Sine wave       (0..65535, centered at 32767)
/*   HR[2]  - Cosine wave     (0..65535, centered at 32767)
/*   HR[3]  - Sawtooth wave   (0..65535)
/*   HR[4]  - Square wave     (0 or 65535 at 100%, intermediate levels when scaled)
/*   HR[5]  - Triangle wave   (0..65535)
/*   HR[6]  - Noise           (random, 0..65535)
/*   HR[7]  - Phase step (deg, writable - controls wave speed, default=2)
/*   HR[8]  - Amplitude %     (writable - controls amplitude 0..100, default=100)
/*
/* Run mode: periodic (e.g. 100 ms)
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

var deviceId = 1;

function tick()
{
    /* State must be kept in Storage: the whole script is re-run every period,
       so top-level variables would reset on each tick (only Script.onInit()
       and Storage persist). */
    var phase = Storage.getItem("phase");

    var phaseStep = Server.readHolding(7, deviceId);
    if(phaseStep <= 0 || phaseStep > 360) phaseStep = 2;

    var amplitudePct = Server.readHolding(8, deviceId);
    if(amplitudePct < 0 || amplitudePct > 100) amplitudePct = 100;

    var MID = 32767;
    var rad = phase * Math.PI / 180;

    /* Scale a full-range (0..65535) wave around the midpoint by the amplitude,
       so 0% collapses every wave to the center line and 100% is full range. */
    function applyAmp(value)
    {
        return Math.round(MID + (value - MID) * amplitudePct / 100);
    }

    var sineVal     = applyAmp(MID + MID * Math.sin(rad));
    var cosineVal   = applyAmp(MID + MID * Math.cos(rad));
    var sawVal      = applyAmp((phase / 360) * 65535);
    var squareVal   = applyAmp((phase < 180) ? 65535 : 0);
    var triVal      = applyAmp((phase < 180)
        ? (phase / 180) * 65535
        : ((360 - phase) / 180) * 65535);
    var noiseVal    = applyAmp(Math.random() * 65535);

    Server.writeHolding(1, sineVal,   deviceId);
    Server.writeHolding(2, cosineVal, deviceId);
    Server.writeHolding(3, sawVal,    deviceId);
    Server.writeHolding(4, squareVal, deviceId);
    Server.writeHolding(5, triVal,    deviceId);
    Server.writeHolding(6, noiseVal,  deviceId);

    phase = (phase + phaseStep) % 360;
    Storage.setItem("phase", phase);
}

function init()
{
    /* Seed persistent state */
    Storage.setItem("phase", 0);

    /* Write defaults for control registers */
    Server.writeHolding(7, 2,   deviceId);  /* phase step = 2 deg */
    Server.writeHolding(8, 100, deviceId);  /* amplitude  = 100%  */

    console.log("=== Wave Generator started ===");
    console.log("HR[1] Sine | HR[2] Cosine | HR[3] Sawtooth | HR[4] Square | HR[5] Triangle | HR[6] Noise");
    console.log("HR[7] Phase step (deg/tick, 1..360) | HR[8] Amplitude (%, 0..100)");

    /* Watch for external changes to control registers */
    Server.onChange(deviceId, Register.Holding, 7, function(val) {
        console.log("Phase step changed to " + val + " deg/tick");
    });
    Server.onChange(deviceId, Register.Holding, 8, function(val) {
        console.log("Amplitude changed to " + val + "%");
    });
}

Script.onInit(init);
tick();
