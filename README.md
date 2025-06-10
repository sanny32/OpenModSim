# Open ModSim
Open ModSim is a free implimentation of modbus slave (server) utility for modbus-tcp and modbus-rtu protocols.

![image](https://github.com/user-attachments/assets/cfa669f5-4018-4db4-bc43-ca060d469182)

![image](https://github.com/user-attachments/assets/16653017-d479-466a-8302-5960a2e47fc4)



## Features

The following Modbus functions are available:

Discrete Coils/Flags

    0x01 - Read Coils
    0x02 - Read Discrete Inputs
    0x05 - Write Single Coil
    0x0F - Write Multiple Coils

Registers

    0x03 - Read Holding Registers
    0x04 - Read Input Registers
    0x06 - Write Single Register
    0x10 - Write Multiple Registers
    0x16 - Mask Write Register
    
The following simulations are available:

Discrete Coils/Flags

    Random - simulate flag randomly
    Toggle - simulate flag on/off periodicaly
    
Registers

    Random - simulate register randomly
    Increment - simulate register from Low Limit to High Limit with a given Step
    Decrement - simulate register from High Limit to Low Limit with a given Step

## Modbus Logging

![image](https://github.com/user-attachments/assets/f62d721f-dd0b-4db7-a239-2a22045e99cd)


## Extended Featues

- Modbus Message Parser

![image](https://github.com/sanny32/OpenModSim/assets/13627951/7e9744b8-f4b3-439a-a312-79cbdc426dc2)

  
## Scripting
  From version 1.2.0 Open ModSim supports scripting. Qt runtime implements the [ECMAScript Language Specification](http://www.ecma-international.org/publications/standards/Ecma-262.htm) standard, so Javascript is used to write code.
  
![image](https://github.com/user-attachments/assets/5b448fcd-1ca7-4cfc-af0f-175fbb660f80)


  Scripts can be launched in two modes: Once or Periodically. If you run script in Once mode the script will stop after it finishes executing. In Periodically mode, the script will start after a certain period of time until the user stops it or the method is called
  ```javascript
  Script.stop();
  ```
Here is an example of using the script in the Periodically mode
```javascript
/**************************************************************************/
/*
/* Example script that store value after 3 seconds
/*
***************************************************************************/

function clear()
{
    /* Write to a Holding register at address 1 zero value */
    Server.writeHolding(1, 0);
}

/* init function */
function init()
{
    /* Set the server address base starts from one (1-based) */
    Server.addressBase = AddressBase.Base1;

    clear();
    
    /* Runs when Hodling register value at address 1 was changed */
    Server.onChange(Register.Holding, 1, (value)=>
    {
        if(value === 1)
        {
            /* Runs after 3 seconds and increase Holding register value at address 10 
             * Then stop script execution
             */
            Script.setTimeout(function()
            {
                Server.writeHolding(10, Server.readHolding(10) + 1);
                Script.stop();
            }, 3000);
        }
     });
}

/* Runs once when script started */
Script.onInit(init);
```

## Building
  Now building is available via cmake (with installed Qt version 5.15 and above) or Qt Creator. Supports both OS Microsoft Windows and Linux.
  
## MIT License
Copyright 2024 Alexandr Ananev [mail@ananev.org]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
