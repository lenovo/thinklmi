# think-lmi

Linux Driver for Think WMI interface.
This dirver allows you to control many BIOS settings from Linux via sysfs
entrypoints or ioctls.

## sysfs interface

Directory: /sys/bus/wmi/drivers/think-lmi/

Each setting exposed by the WMI interface is available under its own name
in this sysfs directory. Read from the file to get the current value (line 1)
and list of options (line 2), and write an option to the file to set it.

Additionally, there are extra files for querying and managing BIOS password(s).

### password

Must contain the BIOS supervisor password (aka 'pap'), if set, to be able to do
any change.

Every subsequent password change will be authorized with this password. The
password may be unloaded by writing an empty string. Writing an invalid
password may trigger the BIOS' invalid password limit, such that a reboot will
be required in order to make any further BIOS changes.

### password_encoding

Encoding used for the password, either '', 'scancode' or 'ascii'.

Scan-code encoding appears to require the key-down scan codes, e.g. 0x1e, 0x30,
0x2e for the ASCII encoded password 'abc'.

### password_kbd_lang

Keyboard language mapping, can be '', 'us', 'fr' or 'gr'.

### password_type

Specify the password type to be changed when password_change is written to.
Can be:
* 'pap': supervisor password
* 'pop': power-on-password

Other types may be valid, e.g. for user and master disk passwords.

### password_change

Writing to this file will change the password specified by password_type. The
new password will not take effect until the next reboot.

### password_settings

Display password related settings. This includes:

* password_state: which passwords are set, if any
  * bit 0: user password (power on password) is installed / 'pop'
  * bit 1: admin/supervisor password is installed / 'pap'
  * bit 2: hdd password(s) installed
* supported_encodings: supported keyboard encoding(s)
  * bit 0: ASCII
  * bit 1: scancode
* supported_keyboard: support keyboard language(s)
  * bit 0: us
  * bit 1: fr
  * bit 2: gr

### load_default_settings

Reset all settings to factory default.

## debugfs interface

The debugfs interface maps closely to the WMI Interface (see driver and doc).

* bios_settings: show all BIOS settings
* bios_setting: show BIOS setting for <instance>
* list_valid_choices: list settings for <argument>
* set_bios_settings: call set bios settings command with <argument>.
* save_bios_settings call save bios settings command with <argument>.
* discard_bios_settings: call discard bios settings command with <argument>.
* load_default: call load default with <argument>.
* set_bios_password: call set BIOS password with <argument>.
* argument: argument to be used in various commands.
* instance: setting instance.
* instance_count: number of settings.
* password_settings: password settings.

## References

Thinkpad WMI interface documentation:
http://download.lenovo.com/ibmdl/pub/pc/pccbbs/thinkcentre_pdf/hrdeploy_en.pdf

## Legal
This code is distributed under GPL-v2 license with full text of the license 
located in the COPYING file.

## Compile
make
make install - loads this module after a reboot
