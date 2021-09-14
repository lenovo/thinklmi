# think-lmi : Userspace Utlity

Linux userspace utility for Think WMI interface.
When used in conjunction with the thinklmi kernel driver this utility allows you 
to easily control many BIOS settings from Linux using simple commands

To compile: gcc -o thinklmi thinklmi.c

## display available settings 
./thinklmi getsettings 

This retrieves all available settings that are available from the BIOS

## Get setting value
./thinklmi -g [BIOS Setting]

Retrieves the given settings current value and lists available options for setting

eg: ./thinklmi -g WakeOnLANDock
The above command will get the available options for WakeOnLANDock

## Set setting value
./thinklmi -s [BIOS Setting] [option]

Sets the given setting to the given value

eg: ./thinklmi -s WakeOnLANDock Enable
The above command will enable the WakeOnLANDock feature

## password authentication
./thinklmi -p [Password] [encoding] [keyboard language]

Performs password authentication

Must contain the BIOS supervisor password (aka 'pap'), if set, to be able to do
any change.

Every subsequent password change will be authorized with this password. The
password may be unloaded by writing an empty string. Writing an invalid
password may trigger the BIOS' invalid password limit, such that a reboot will
be required in order to make any further BIOS changes.

The encoding used for the password is either '', 'scancode' or 'ascii'.

Scan-code encoding appears to require the key-down scan codes, e.g. 0x1e, 0x30,
0x2e for the ASCII encoded password 'abc'.

The keyboard language mapping, can be '', 'us', 'fr' or 'gr'.

eg: ./thinklmi -p hello ascii us
If the supervisor password is set as hello, with ascii encoding
and the keyboard type is US, the above command will authenticate the BIOS setting
Once authenticated, it remains valid till the next restart.

## Password change
./thinklmi -c [Password] [New Password] [Password Type] [encoding] [keyboard language]

This configures a new BIOS password. Use with care.

The password type can be either 'pap' for the supervisor password or 'pop' for the power on password

eg: ./thinklmi -p hello newpd pap ascii us
Change the supervisor password or power on password using this command.
Do a reboot for the new password to take effect.

## Debug
./thinklmi -d [BIOS Setting] [option]

This configures a the BIOS setting in debug mode. Use with care.

eg: ./thinklmi -d FnSticky Enable


## Change Password using LMIOPCODE
./thinklmi -w [Admin Password] [Password Type] [Old Password] [New Pasword]

This command changes the password in thinkstation systems, with Admin password

eg: ./thinklmi -w hello pop oldpass newpass

## Change Password using LMIOPCODE, without Admin Password
./thinklmi -w [Password Type] [Old Password] [New Password]

This command changes the password in thinkstation systems, without an Admin password

eg: ./thinklmi -w pop oldpass newpass

## TPMTYPE Change
./thinklmi -t [TPM Type]

This command changes the TPM Type in thinkstation systems

./thinklmi -t tpm12


## Load Default Settings
./thinklmi -l

This command loads the Default BIOS Settings


## Save Settings
./thinklmi save settings

This command saves the BIOS Settings

## Discard Default Settings
./thinklmi discard settings

This command discards the Default BIOS Settings

## References
Thinkpad WMI interface documentation:
http://download.lenovo.com/ibmdl/pub/pc/pccbbs/thinkcentre_pdf/hrdeploy_en.pdf

## Legal
This code is distributed under GPL-v2 license with full text of the license 
located in the COPYING file.
