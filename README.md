# Open ModSim

[![GitHub all releases](https://img.shields.io/github/downloads/sanny32/OpenModSim/total?logo=github)](https://github.com/sanny32/OpenModSim/releases)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/sanny32/OpenModsim?cacheSeconds=3600&logo=github)](https://github.com/sanny32/OpenModSim/releases/latest)
[![License](https://img.shields.io/github/license/sanny32/OpenModSim)](LICENSE.md)

Open ModSim is a free implimentation of modbus slave (server) utility for modbus-tcp and modbus-rtu protocols.

<img width="1292" height="759" alt="image" src="https://github.com/user-attachments/assets/f740a180-6f2b-40fc-9e79-fd93a08353c9" />

<img width="1292" height="759" alt="image" src="https://github.com/user-attachments/assets/ec3f02a6-2504-4f32-b22a-e22318afe0e4" />


# Features

The following Modbus functions are available:

- Discrete Coils/Flags
```
    0x01 - Read Coils
    0x02 - Read Discrete Inputs
    0x05 - Write Single Coil
    0x0F - Write Multiple Coils
```

- Registers
```
    0x03 - Read Holding Registers
    0x04 - Read Input Registers
    0x06 - Write Single Register
    0x10 - Write Multiple Registers
    0x16 - Mask Write Register
```
    
The following simulations are available:

- Discrete Coils/Flags
```
    Random - simulate flag randomly
    Toggle - simulate flag on/off periodicaly
```

- Registers
```
    Random - simulate register randomly
    Increment - simulate register from Low Limit to High Limit with a given Step
    Decrement - simulate register from High Limit to Low Limit with a given Step
```

# Modbus Logging

<img width="1292" height="759" alt="image" src="https://github.com/user-attachments/assets/66c32a67-1db7-46b3-8c99-6d738a91557f" />


# Extended Featues

- Modbus Message Parser

<img width="674" height="463" alt="image" src="https://github.com/user-attachments/assets/774e3ff1-1bf2-46a6-a685-e6702e2e7fe5" />

- Modbus Definitions
  
<img width="416" height="346" alt="image" src="https://github.com/user-attachments/assets/2dc6c13e-e4be-434b-9266-1de4f3ccda4a" />

- Error Simualtions

<img width="416" height="346" alt="image" src="https://github.com/user-attachments/assets/af1701d9-d576-4f1f-9bf9-7e70e85cd9da" />

  
# Scripting
  From version 1.2.0 Open ModSim supports scripting. Qt runtime implements the [ECMAScript Language Specification](http://www.ecma-international.org/publications/standards/Ecma-262.htm) standard, so Javascript is used to write code.
  
<img width="1292" height="759" alt="image" src="https://github.com/user-attachments/assets/54c0994c-4a24-4425-9b7b-cbbc6b8656b1" />

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

/* Set the server address base starts from one (1-based) */
Server.addressBase = AddressBase.Base1;

let deviceId = 1;
let address1 = 1;
let address10 = 10;

function reset()
{
    /* Write to a Holding register at address1 zero value */
    Server.writeHolding(address1, 0, deviceId);
}

/* init function */
function init()
{
    reset();

	/* Print server error if occured and stop script execution */
	Server.onError(deviceId, (error)=> {
		console.error(error);
 		Script.stop();
	});   

    /* Runs when Hodling register value at address1 was changed */
    Server.onChange(deviceId, Register.Holding, address1, (value)=>
    {
        if(value === 1)
        {
            /* Runs after 3 seconds and increase Holding register value at address10 
             * Then reset register value at address1 and stop script execution
             */
            Script.setTimeout(function()
            {
                Server.writeHolding(address10, Server.readHolding(address10, deviceId) + 1, deviceId);
				reset();
                Script.stop();
            }, 3000);
        }
     });
}

/* Runs once when script started */
Script.onInit(init);
```

# Building
  Now building is available via cmake (with installed Qt version 5.15 and above) or Qt Creator. Supports both OS Microsoft Windows and Linux.

# About supported operating systems

The following minimum operating system versions are supported for OpenModSim:
- Microsoft Windows 7
- Debian Linux 11
- Ubuntu Linux 22.04
- Mint Linux 22
- Fedora Linux 41
- OpenSuse Linux 15.6
- Alt Linux 11
- Astra Linux 1.7
- RedOS 8

# Install from binary distributions

Below are the methods for installing the OpenModSim for different OS

## Microsoft Windows
Run the installer:

- For 32-bit Windows: `qt5-omodsim_1.10.0_x86.exe`
- For 64-bit Windows: `qt5-omodsim_1.10.0_x64.exe` or `qt6-omodsim_1.10.0_x64.exe`

## Debian/Ubintu/Mint/Astra Linux
### Install
Install the DEB package from the command line:
```bash
sudo apt install -f ./qt6-omodsim_1.10.0-1_amd64.deb
```
or if you want to use Qt5 libraries:
```bash
sudo apt install -f ./qt5-omodsim_1.10.0-1_amd64.deb
```

### Remove
To remove the DEB package run:
```bash
sudo apt remove qt6-omodsim
```
or for Qt5 package:
```bash
sudo apt remove qt5-omodsim
```

## RedHat/Fedora/RedOS Linux
### Install
Install the RPM package from the command line:
```bash
sudo dnf install ./qt6-omodsim-1.10.0-1.x86_64.rpm
```

### Remove
To remove the RPM package run:
```bash
sudo dnf remove qt6-omodsim
```

## Alt Linux
### Install
Install the RPM package from the command line as root user:
```bash
apt-get install ./qt6-omodsim-1.10.0-1.x86_64.rpm
```

### Remove
To remove the RPM package run as root user:
```bash
apt-get remove qt6-omodsim
```

## SUSE/OpenSUSE Linux
### Install
Import qt6-omodsim.rpm.pubkey to rpm repository:
```bash
sudo rpm --import qt6-omodsim.rpm.pubkey
```
Install the RPM package using Zypper:
```bash
sudo zypper install ./qt6-omodsim-1.10.0-1.x86_64.rpm
```

### Remove
To remove the RPM package run:
```bash
sudo zypper remove qt6-omodsim
```
  
## MIT License
Copyright 2025 Alexandr Ananev [mail@ananev.org]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
