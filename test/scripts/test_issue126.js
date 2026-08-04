/**************************************************************************/
/*
/* Regression script for issue #126
/*
/* Reproduces the reported freeze: a large initial fill from Script.onInit()
/* followed by a self-rescheduling Script.setTimeout() chain.
/*
/* Expected behaviour:
/*   - the initial fill completes without blocking the UI
/*   - input registers keep updating once per second afterwards
/*
/* Run mode: Once (the setTimeout chain keeps the script alive)
/*
***************************************************************************/

Server.addressBase = AddressBase.Base0;

var REGISTER_COUNT = 300;

var REGISTER_HEARTBEAT = 0;
var REGISTER_SECONDS = 1;
var REGISTER_TICKS = 2;

var ticks = 0;

function fillInitialValues()
{
    var holdings = [];
    var inputs = [];

    for(var i = 0; i < REGISTER_COUNT; i++)
    {
        holdings.push(i);
        inputs.push(REGISTER_COUNT - i);
    }

    /* Batch API: one call per register block instead of 300 separate writes */
    Server.writeHoldings(0, holdings);
    Server.writeInputs(0, inputs);
}

function runOncePerSecond()
{
    ticks++;

    var now = new Date();
    Server.writeInput(REGISTER_HEARTBEAT, ticks % 2);
    Server.writeInput(REGISTER_SECONDS, now.getSeconds());
    Server.writeInput(REGISTER_TICKS, ticks);

    console.log("tick " + ticks + " seconds=" + now.getSeconds());
}

function timer()
{
    Script.setTimeout(function() {
        runOncePerSecond();
        timer();
    }, 1000);
}

Script.onInit(function() {
    fillInitialValues();
    console.log("initial fill done: " + REGISTER_COUNT + " holdings + " + REGISTER_COUNT + " inputs");

    timer();

    console.log("Init done");
});
