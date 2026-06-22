/**************************************************************************/
/*
/* Data Logger Demo
/* Monitors register changes and logs them to the console with timestamps.
/* Also keeps statistics: min, max, change count per register.
/*
/* Watches:
/*   Holding Registers HR[1..10]
/*   Coils             Coil[1..8]
/*
/* Holding Registers (control, writable):
/*   HR[100] - Log level: 0=all changes, 1=only out-of-range, 2=statistics only
/*   HR[101] - Low  threshold for range check (default 100)
/*   HR[102] - High threshold for range check (default 900)
/*
/* Run mode: periodic (e.g. 1000 ms). The script is event-driven (it only
/* registers onChange/setTimeout handlers), but it must run periodically so the
/* engine keeps the handlers alive - in Once mode the engine tears them down
/* right after the first run.
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

var deviceId = 1;

/* Statistics per register. The whole script is re-run every period, so the
   stats object is kept in Storage to survive re-execution (only Script.onInit()
   and Storage persist). */
var stats = Storage.getItem("stats");
if(stats === null)
{
    stats = {};
    Storage.setItem("stats", stats);
}

function initStat(name)
{
    stats[name] = { min: null, max: null, count: 0, last: null };
}

function updateStat(name, value)
{
    var s = stats[name];
    s.count++;
    s.last = value;
    if(s.min === null || value < s.min) s.min = value;
    if(s.max === null || value > s.max) s.max = value;
}

function timestamp()
{
    var d = new Date();
    var hh = ("0" + d.getHours()).slice(-2);
    var mm = ("0" + d.getMinutes()).slice(-2);
    var ss = ("0" + d.getSeconds()).slice(-2);
    var ms = ("00" + d.getMilliseconds()).slice(-3);
    return hh + ":" + mm + ":" + ss + "." + ms;
}

function getLogLevel()
{
    var lvl = Server.readHolding(100, deviceId);
    if(lvl < 0 || lvl > 2) return 0;
    return lvl;
}

function getThresholds()
{
    var lo = Server.readHolding(101, deviceId);
    var hi = Server.readHolding(102, deviceId);
    if(lo > hi) { lo = 100; hi = 900; }   /* defensive fallback; HR is normalized in the onChange handlers */
    return { lo: lo, hi: hi };
}

///
/// Corrects HR[101]/HR[102] in place when the client writes a misordered pair,
/// so the registers immediately reflect valid setpoints.
///
function normalizeThresholds()
{
    var lo = Server.readHolding(101, deviceId);
    var hi = Server.readHolding(102, deviceId);
    if(lo > hi)
    {
        Server.writeHolding(101, 100, deviceId);
        Server.writeHolding(102, 900, deviceId);
    }
}

function makeHoldingHandler(addr)
{
    var name = "HR[" + addr + "]";
    return function(value) {
        updateStat(name, value);
        var lvl = getLogLevel();
        if(lvl === 2) return; /* statistics only - no per-change log */

        var t = getThresholds();
        var outOfRange = (value < t.lo || value > t.hi);

        if(lvl === 1 && !outOfRange) return; /* only log out-of-range */

        var msg = timestamp() + "  " + name + " = " + value;
        if(outOfRange)
            console.warning(msg + "  [OUT OF RANGE " + t.lo + ".." + t.hi + "]");
        else
            console.log(msg);
    };
}

function makeCoilHandler(addr)
{
    var name = "Coil[" + addr + "]";
    return function(value) {
        updateStat(name, value ? 1 : 0);
        var lvl = getLogLevel();
        /* lvl 1 = out-of-range only (not applicable to coils), lvl 2 = stats only */
        if(lvl >= 1) return;
        console.log(timestamp() + "  " + name + " = " + (value ? "ON" : "OFF"));
    };
}

function printStats()
{
    console.log("--- Statistics snapshot at " + timestamp() + " ---");
    for(var name in stats)
    {
        var s = stats[name];
        if(s.count === 0) continue;
        console.log("  " + name + ": changes=" + s.count
            + "  min=" + s.min + "  max=" + s.max + "  last=" + s.last);
    }
}

function init()
{
    /* Control registers defaults */
    Server.writeHolding(100, 0,   deviceId);  /* log level: all */
    Server.writeHolding(101, 100, deviceId);  /* low  threshold */
    Server.writeHolding(102, 900, deviceId);  /* high threshold */

    /* Init stats tracking */
    for(var a = 1; a <= 10; a++) initStat("HR[" + a + "]");
    for(var a = 1; a <= 8;  a++) initStat("Coil[" + a + "]");

    /* Register onChange for HR[1..10] */
    for(var a = 1; a <= 10; a++)
        Server.onChange(deviceId, Register.Holding, a, makeHoldingHandler(a));

    /* Register onChange for Coil[1..8] */
    for(var a = 1; a <= 8; a++)
        Server.onChange(deviceId, Register.Coils, a, makeCoilHandler(a));

    /* Watch control registers */
    Server.onChange(deviceId, Register.Holding, 100, function(val) {
        var labels = ["all changes", "out-of-range only", "statistics only"];
        console.log("Log level set to " + val + " (" + (labels[val] || "?") + ")");
    });
    Server.onChange(deviceId, Register.Holding, 101, function(val) {
        console.log("Low threshold set to "  + val);
        normalizeThresholds();
    });
    Server.onChange(deviceId, Register.Holding, 102, function(val) {
        console.log("High threshold set to " + val);
        normalizeThresholds();
    });

    /* Print statistics every 30 seconds */
    function schedStats() {
        printStats();
        Script.setTimeout(schedStats, 30000);
    }
    Script.setTimeout(schedStats, 30000);

    Server.onError(deviceId, function(error) {
        console.error("Error: " + error);
    });

    console.log("=== Data Logger started ===");
    console.log("Watching HR[1..10] and Coil[1..8]");
    console.log("HR[100] Log level (0=all/1=out-of-range/2=stats) | HR[101] Lo thr | HR[102] Hi thr");
    console.log("Statistics will be printed every 30 seconds.");
}

Script.onInit(init);
