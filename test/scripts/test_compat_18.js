/**************************************************************************/
/*
/* Backward-compatibility script for issue #126
/*
/* Uses the 1.8.x call shapes that dropped their leading deviceId argument:
/*   Server.onChange(Register, address, callback)
/*   Server.onError(callback)
/*   Server.writeHolding(address, value)
/*
/* Expected behaviour: no errors, and the onChange callback mirrors
/* Holding[1] into Input[1].
/*
/* Run mode: Periodically, 500 ms
/*
***************************************************************************/

Server.addressBase = AddressBase.Base1;

/* Every call below targets this unit without repeating it each time */
Server.deviceId = 1;

Script.onInit(function() {
    Server.onError(function(error) {
        console.error("Error: " + error);
    });

    /* 1.8.x form: no deviceId argument */
    Server.onChange(Register.Holding, 1, function(value) {
        console.log("Holding[1] changed to " + value);
        Server.writeInput(1, value);
    });

    Server.writeHolding(1, 0);
    Server.writeInput(1, 0);

    console.log("compat script initialized");
});

/* Runs every period: bump Holding[1] and let onChange mirror it */
var current = Server.readHolding(1);
Server.writeHolding(1, (current + 1) % 1000);
