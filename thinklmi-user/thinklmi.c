/*
 * Think LMI BIOS configuration application
 *
 * Copyright(C) 2019-2020 Lenovo
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Application to provide ioctl access to BIOS settings*/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
 
#include "../thinklmi-kernel/think-lmi.h"

void get_settings_all(int fd)
{
    int i, settings_count;
    unsigned char settings_str[TLMI_SETTINGS_MAXLEN];

    if (ioctl(fd, THINKLMI_GET_SETTINGS, &settings_count) == -1) {
        perror("query_apps ioctl get");
    } else {
	printf("Total settings: %d\n", settings_count);
	for(i=0; i <= 0xFF; i++)
	{
		settings_str[0] = i;
                
                if (ioctl(fd, THINKLMI_GET_SETTINGS_STRING, &settings_str) >= 0)
			printf("%3.3d: %s\n", i, settings_str);
                /*else
			printf("%3.3d:\n", i);*/
	}
    }
}

void thinklmi_get(int fd, char * argv2)
{
	char settings_str[TLMI_SETTINGS_MAXLEN];
	int err;
        strncpy(settings_str, argv2, TLMI_SETTINGS_MAXLEN);
	err = ioctl(fd, THINKLMI_SHOW_SETTING, &settings_str);
	if(err == -1)
	   perror("Invalid setting name");
	else
           printf("%s\n", settings_str);
}

void thinklmi_set(int fd, char * argv2, char* argv3)
{
	char setting_string[TLMI_GETSET_MAXLEN];
        strncpy(setting_string, argv2, TLMI_SETTINGS_MAXLEN);
	strcat(setting_string, ",");
	strncat(setting_string, argv3, TLMI_SETTINGS_MAXLEN);

	if(ioctl(fd, THINKLMI_SET_SETTING, &setting_string) == -1) {
	   perror("Unable to change setting");
	} else {
	   printf("BIOS Setting changed\n");
           printf("Setting will not change until reboot\n");
	}
}

void thinklmi_authenticate(int fd, char *passwd, char *encode, char *lang )
{
	char setting_string[TLMI_GETSET_MAXLEN];

	snprintf(setting_string, TLMI_GETSET_MAXLEN, "%s,%s,%s", passwd, encode, lang);
        if(ioctl(fd, THINKLMI_AUTHENTICATE, &setting_string) == -1) {
	   perror("BIOS authenticate failed");
	} else {
	   printf("BIOS authentication completed\n");
           printf("This will be valid till the next reboot\n");
	}
}

void thinklmi_change_password(int fd, char *oldpass, char *newpass, char *passtype, char *encode, char *lang)
{
	char setting_string[TLMI_GETSET_MAXLEN];

	snprintf(setting_string, TLMI_GETSET_MAXLEN, "%s,%s,%s,%s,%s;", passtype, oldpass, newpass, encode, lang);
        if(ioctl(fd, THINKLMI_CHANGE_PASSWORD, &setting_string) == -1) {
	   perror("BIOS password change failed");
	} else {
	   printf("BIOS password changed\n");
           printf("Setting will not change until reboot\n");
	}
}

void thinklmi_debug(int fd, char *settingname, char *value)
{
	char setting_string[TLMI_GETSET_MAXLEN];
        strncpy(setting_string, settingname, TLMI_SETTINGS_MAXLEN);
	strcat(setting_string, ",");
	strncat(setting_string, value, TLMI_SETTINGS_MAXLEN);
	if(ioctl(fd, THINKLMI_DEBUG, &setting_string) == -1) {
	   perror("Debug Setting Error");
	} else {
	   printf("Debug Setting changed\n");
	}
}

void thinklmi_lmiopcode(int fd, char *admin, char *passtype, char *oldpass, char *newpass )
{
	char setting_string[TLMI_GETSET_MAXLEN];
	snprintf(setting_string, TLMI_GETSET_MAXLEN, "%s,%s,%s,%s;", admin, passtype, oldpass, newpass);
        if(ioctl(fd, THINKLMI_LMIOPCODE, &setting_string) == -1) {
	   perror("BIOS password change failed");
	} else {
	   printf("BIOS password changed\n");
           printf("Setting will not change until reboot\n");
	}
}

void thinklmi_lmiopcode_nopap(int fd, char *passtype, char *oldpass, char *newpass )
{
	char setting_string[TLMI_GETSET_MAXLEN];
	snprintf(setting_string, TLMI_GETSET_MAXLEN, "%s,%s,%s;", passtype, oldpass, newpass);
        if(ioctl(fd, THINKLMI_LMIOPCODE_NOPAP, &setting_string) == -1) {
	   perror("BIOS password change failed");
	} else {
	   printf("BIOS password changed\n");
           printf("Setting will not change until reboot\n");
	}
}
void thinklmi_tpmtype(int fd, char *tpmtype)
{
	char setting_string[TLMI_GETSET_MAXLEN];
        snprintf(setting_string, TLMI_GETSET_MAXLEN, "%s;", tpmtype);
        if(ioctl(fd, THINKLMI_TPMTYPE, &setting_string) == -1) {
           perror("Tpm type change failed");
        } else {
           printf("Tpm type changed\n");
           printf("Setting will not change until reboot\n");
        }

}

void thinklmi_load_default(int fd)
{
	if(ioctl(fd, THINKLMI_LOAD_DEFAULT) == -1) {
	   perror(" Error loading Default Settings\n");
	} else {
	   printf("Default Settings Loaded\n");
	}
}


void thinklmi_save_settings(int fd)
{
	if(ioctl(fd, THINKLMI_SAVE_SETTINGS) == -1) {
	   perror(" Error saving Settings\n");
	} else {
	   printf("Settings saved\n");
	}
}

void thinklmi_discard_settings(int fd)
{
	if(ioctl(fd, THINKLMI_DISCARD_SETTINGS) == -1) {
	   perror(" Error discarding Settings\n");
	} else {
	   printf("Settings Discarded\n");
	}
}

static void show_usage(void)
{
	fprintf(stdout, "Usage: thinklmi [-g | -s | -p | -c | -d | -l | -w | getsettings| save settings | discard settings] <options>\n");
	fprintf(stdout, "Option details:  \n");
	fprintf(stdout, "\t getsettings - display all available BIOS options:  \n");
	fprintf(stdout, "\t -g [BIOS option] - Get the current setting and choices for given BIOS option\n");
	fprintf(stdout, "\t -s [BIOS option] [value] - Set the given BIOS option to given value\n");
	fprintf(stdout, "\t -p [password] [encoding] [kbdlang] - Set authentication details. \n");
	fprintf(stdout, "\t -c [password] [new password] [password type] [encoding] [kbdlang] - Change password. \n");
	fprintf(stdout, "\t -d [debug setting] [option]\n");
	fprintf(stdout, "\t -l load default settings\n");
	fprintf(stdout, "\t -w [Admin password] [password type] [current password] [new password] - Change password using lmiopcode. \n");
	fprintf(stdout, "\t -w [password type] [current password] [new password] - Change password using lmiopcode, no Admin password set. \n");
	fprintf(stdout, "\t -t [tpm type] - Change tpm type\n");
	fprintf(stdout, "\t save settings - save BIOS settings \n");
	fprintf(stdout, "\t discard settings - discard loaded settings \n");
	fprintf(stdout, "Notes:  \n");
	fprintf(stdout, "\t password type can be \"pap\" or \"pop\" \n");
	fprintf(stdout, "\t encoding can be \"ascii\" or \"scancode\" \n");
	fprintf(stdout, "\t kbdland can be \"us\" or \"fr\" or \"gr\"\n");
	exit(1);
}

int main(int argc, char *argv[])
{
    char *file_name = "/dev/thinklmi";
    int fd;
    enum {
	get_settings,
	get,
	set,
	authenticate,
	change_password,
	debug,
	lmiopcode,
	lmiopcode_nopap,
	tpmtype,
	load_default,
	save_settings,
	discard_settings
    } option;

    if (getuid()!=0) {
	    printf("Please run with administrator privileges\n");
	    exit(0);
    }
    switch(argc)
    {

	    case 2:
		    if (strcmp(argv[1], "getsettings") == 0)
			    option = get_settings;
		    else

	            if (strcmp(argv[1], "-l") == 0)
		            option = load_default;
		    else
			    show_usage();
		    break;
	    case 3:
		    if (strcmp(argv[1], "-g") == 0)
			    option = get;
		    else

	            if (strcmp(argv[1], "save") == 0)
			    option = save_settings;
		    else

	            if (strcmp(argv[1], "discard") == 0)
			    option = discard_settings;

		    else

		    if (strcmp(argv[1], "-t") == 0)
			    option = tpmtype;

		    else
			    show_usage();
		    break;
	    case 4:
		    if (strcmp(argv[1], "-s") == 0) {
			    option = set;
		    } else

	            if (strcmp(argv[1], "-d") == 0) {
		            option = debug;
		    } else
			    show_usage();
		    break;
	    case 5:
		    if (strcmp(argv[1], "-p") == 0)
			    option = authenticate;
		    else

		    if (strcmp(argv[1], "-w") == 0)
		            option = lmiopcode_nopap;

		    else
			    show_usage();
		    break;
	    case 6:

                    if (strcmp(argv[1], "-w") == 0)
                            option = lmiopcode;
		    else
			    show_usage();
		    break;
	    case 7:
		    if (strcmp(argv[1], "-c") == 0)
			    option = change_password;
	            else
			    show_usage();
		    break;
	    case 8:
		    if (strcmp(argv[1], "-l") == 0)
			    option = load_default;
		    else
			    show_usage();
		    break;
	    case 9:
		    if (strcmp(argv[1], "discard settings") == 0)
			    option = discard_settings;
		    else
			    show_usage();
		    break;

	    default:
		    show_usage();
		    return 1;
    }
    fd = open(file_name, O_RDWR);
    if (fd == -1) {
	    perror("query_apps open");
	    return 2;
    }
 
    switch (option) {
	    case get_settings:
		    get_settings_all(fd);
		    break;
	    case get:
		    thinklmi_get(fd, argv[2]);
		    break;
	    case set:
		    thinklmi_set(fd, argv[2], argv[3]);
		    break;
	    case authenticate:
		    thinklmi_authenticate(fd, argv[2], argv[3], argv[4]);
		    break;
	    case change_password:
		    thinklmi_change_password(fd, argv[2], argv[3], argv[4], argv[5], argv[6]);
		    break;
	    case debug:
		    thinklmi_debug(fd, argv[2], argv[3]);
		    break;
	    case lmiopcode:
		    thinklmi_lmiopcode(fd, argv[2], argv[3], argv[4], argv[5]);
		    break;
	    case lmiopcode_nopap:
		    thinklmi_lmiopcode_nopap(fd, argv[2], argv[3], argv[4]);
		    break;
	    case tpmtype:
		    thinklmi_tpmtype(fd, argv[2]);
		    break;
	    case load_default:
		    thinklmi_load_default(fd);
		    break;
	    case save_settings:
		    thinklmi_save_settings(fd);
		    break;
	    case discard_settings:
		    thinklmi_discard_settings(fd);
		    break;
    }
    close (fd);
 
    return 0;
} 
