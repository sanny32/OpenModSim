/**************************************************************************/
/*
/* Test script for Server.onRequest handler
/* Tests all supported function codes and response formats
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

var deviceId = 1;

function init()
{
    Server.onError(deviceId, function(error) {
        console.error("Error: " + error);
    });

    /* Fill some initial data */
    for(var i = 0; i < 10; i++)
    {
        Server.writeHolding(i + 1, 100 + i, deviceId);
        Server.writeInput(i + 1, 200 + i, deviceId);
        Server.writeCoil(i + 1, i % 2 === 0, deviceId);
        Server.writeDiscrete(i + 1, i % 3 === 0, deviceId);
    }

    console.log("=== onRequest test script started ===");
    console.log("Initial data written: HR[1..10], IR[1..10], Coils[1..10], DI[1..10]");

    Server.onRequest(function(req)
    {
        var fc = req.functionCode;
        console.log("--- FC " + fc + " from device " + req.deviceId + " ---");

        /* Log raw data bytes */
        var hex = "";
        for(var i = 0; i < req.data.length; i++)
        {
            var b = req.data[i].toString(16).toUpperCase();
            hex += (b.length < 2 ? "0" : "") + b + " ";
        }
        console.log("  raw data: [" + hex.trim() + "]");

        /* ---- FC 1: Read Coils ---- */
        if(fc === 1)
        {
            console.log("  ReadCoils addr=" + req.address + " count=" + req.count);
            /* Return custom coil pattern: alternating true/false */
            var coils = [];
            for(var i = 0; i < req.count; i++)
                coils.push(i % 2 === 0);
            console.log("  -> responding with " + coils.length + " coils");
            return coils;
        }

        /* ---- FC 2: Read Discrete Inputs ---- */
        if(fc === 2)
        {
            console.log("  ReadDiscreteInputs addr=" + req.address + " count=" + req.count);
            /* Let default handler respond */
            return null;
        }

        /* ---- FC 3: Read Holding Registers ---- */
        if(fc === 3)
        {
            console.log("  ReadHoldingRegisters addr=" + req.address + " count=" + req.count);
            /* Return custom values: address * 10 */
            var regs = [];
            for(var i = 0; i < req.count; i++)
                regs.push((req.address + i) * 10);
            console.log("  -> responding with registers: [" + regs.join(", ") + "]");
            return regs;
        }

        /* ---- FC 4: Read Input Registers ---- */
        if(fc === 4)
        {
            console.log("  ReadInputRegisters addr=" + req.address + " count=" + req.count);
            /* Let default handler respond */
            return null;
        }

        /* ---- FC 5: Write Single Coil ---- */
        if(fc === 5)
        {
            console.log("  WriteSingleCoil addr=" + req.address + " value=" + req.value);
            /* Echo back using raw data response */
            return { data: req.data };
        }

        /* ---- FC 6: Write Single Register ---- */
        if(fc === 6)
        {
            console.log("  WriteSingleRegister addr=" + req.address + " value=" + req.value);
            /* Let default handler do the write */
            return null;
        }

        /* ---- FC 7: Read Exception Status ---- */
        if(fc === 7)
        {
            console.log("  ReadExceptionStatus");
            /* Return status byte 0xA5 */
            return { data: [0xA5] };
        }

        /* ---- FC 8: Diagnostics ---- */
        if(fc === 8)
        {
            var subFunc = (req.data[0] << 8) | req.data[1];
            console.log("  Diagnostics subFunction=" + subFunc);
            /* Echo back (Return Query Data) */
            return { data: req.data };
        }

        /* ---- FC 15: Write Multiple Coils ---- */
        if(fc === 15)
        {
            console.log("  WriteMultipleCoils addr=" + req.address + " count=" + req.count);
            console.log("  values: [" + Array.prototype.slice.call(req.values).join(", ") + "]");
            /* Standard response: echo address and count */
            return { data: [
                req.data[0], req.data[1],   /* address hi, lo */
                req.data[2], req.data[3]    /* count hi, lo */
            ]};
        }

        /* ---- FC 16: Write Multiple Registers ---- */
        if(fc === 16)
        {
            console.log("  WriteMultipleRegisters addr=" + req.address + " count=" + req.count);
            console.log("  values: [" + Array.prototype.slice.call(req.values).join(", ") + "]");
            /* Standard response: echo address and count */
            return { data: [
                req.data[0], req.data[1],
                req.data[2], req.data[3]
            ]};
        }

        /* ---- FC 17: Report Server ID ---- */
        if(fc === 17)
        {
            console.log("  ReportServerId");
            /* ServerID=1, RunStatus=0xFF (ON), additional data = "OModSim" */
            return { data: [0x01, 0xFF, 0x4F, 0x4D, 0x6F, 0x64, 0x53, 0x69, 0x6D] };
        }

        /* ---- FC 22: Mask Write Register ---- */
        if(fc === 22)
        {
            console.log("  MaskWriteRegister addr=" + req.address
                + " AND=" + req.andMask.toString(16)
                + " OR=" + req.orMask.toString(16));
            /* Standard response: echo the request */
            return { data: req.data };
        }

        /* ---- FC 23: Read/Write Multiple Registers ---- */
        if(fc === 23)
        {
            console.log("  ReadWriteMultipleRegisters");
            console.log("    read: addr=" + req.readAddress + " count=" + req.readCount);
            console.log("    write: addr=" + req.writeAddress + " count=" + req.writeCount);
            console.log("    write values: [" + Array.prototype.slice.call(req.values).join(", ") + "]");
            /* Return read data: address * 100 */
            var regs = [];
            for(var i = 0; i < req.readCount; i++)
                regs.push((req.readAddress + i) * 100);
            /* Build raw response: byteCount + register pairs */
            var resp = [req.readCount * 2];
            for(var i = 0; i < regs.length; i++)
            {
                resp.push((regs[i] >> 8) & 0xFF);
                resp.push(regs[i] & 0xFF);
            }
            return { data: resp };
        }

        /* ---- Any other FC: return IllegalFunction exception ---- */
        console.log("  Unknown FC " + fc + " -> returning IllegalFunction");
        return { exception: 1 };
    });

    console.log("Handler registered. Send Modbus requests to test.");
}

Script.onInit(init);
