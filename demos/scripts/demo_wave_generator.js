/**************************************************************************/
/*
/* Wave Generator Demo
/* Generates sine, cosine, sawtooth and square waves in Holding Registers
/*
/* Register map (Base1):
/*   HR[1]  - Sine wave       (0..65535, centered at 32767)
/*   HR[2]  - Cosine wave     (0..65535, centered at 32767)
/*   HR[3]  - Sawtooth wave   (0..65535)
/*   HR[4]  - Square wave     (0 or 65535)
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
var phase = 0;

function tick()
{
    var phaseStep = Server.readHolding(7, deviceId);
    if(phaseStep <= 0 || phaseStep > 360) phaseStep = 2;

    var amplitudePct = Server.readHolding(8, deviceId);
    if(amplitudePct <= 0 || amplitudePct > 100) amplitudePct = 100;

    var amp = Math.round(32767 * amplitudePct / 100);
    var rad = phase * Math.PI / 180;

    var sineVal     = Math.round(32767 + amp * Math.sin(rad));
    var cosineVal   = Math.round(32767 + amp * Math.cos(rad));
    var sawVal      = Math.round((phase / 360) * 65535);
    var squareVal   = (phase < 180) ? 65535 : 0;
    var triVal      = (phase < 180)
        ? Math.round((phase / 180) * 65535)
        : Math.round(((360 - phase) / 180) * 65535);
    var noiseVal    = Math.round(Math.random() * 65535);

    Server.writeHolding(1, sineVal,   deviceId);
    Server.writeHolding(2, cosineVal, deviceId);
    Server.writeHolding(3, sawVal,    deviceId);
    Server.writeHolding(4, squareVal, deviceId);
    Server.writeHolding(5, triVal,    deviceId);
    Server.writeHolding(6, noiseVal,  deviceId);

    phase = (phase + phaseStep) % 360;
}

function init()
{
    /* Write defaults for control registers */
    Server.writeHolding(7, 2,   deviceId);  /* phase step = 2 deg */
    Server.writeHolding(8, 100, deviceId);  /* amplitude  = 100%  */

    console.log("=== Wave Generator started ===");
    console.log("HR[1] Sine | HR[2] Cosine | HR[3] Sawtooth | HR[4] Square | HR[5] Triangle | HR[6] Noise");
    console.log("HR[7] Phase step (deg/tick, 1..360) | HR[8] Amplitude (%, 1..100)");

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
