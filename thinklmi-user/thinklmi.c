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
    char settings_str[TLMI_SETTINGS_MAXLEN];

    if (ioctl(fd, THINKLMI_GET_SETTINGS, &settings_count) == -1) {
        perror("query_apps ioctl get");
    } else {
	//printf("Total settings: %d\n", settings_count);
	for(i=0; i < settings_count; i++)
	{
		settings_str[0] = i;
                if (ioctl(fd, THINKLMI_GET_SETTINGS_STRING, &settings_str) >= 0)
			printf("%3.3d: %s\n", i, settings_str);
	}
    }
}

void thinklmi_get(int fd, char * argv2)
{
	char settings_str[TLMI_SETTINGS_MAXLEN];
        strncpy(settings_str, argv2, TLMI_SETTINGS_MAXLEN);
	if(ioctl(fd, THINKLMI_SHOW_SETTING, &settings_str) == -1)
	   perror(" ioctl set_ setting failed");
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
	   perror(" BIOS set_setting failed");
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

#if 0 /*To be fixed*/
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
#endif

static void show_usage(void)
{
#if 0 /*To be fixed*/
	fprintf(stdout, "Usage: thinklmi [-g | -s | -p | -c | getsettings] <options>\n");
#else
	fprintf(stdout, "Usage: thinklmi [-g | -s | -p | getsettings] <options>\n");
#endif
	fprintf(stdout, "Option details:  \n");
	fprintf(stdout, "\t getsettings - display all available BIOS options:  \n");
	fprintf(stdout, "\t -g [BIOS option] - Get the current setting and choices for given BIOS option\n");
	fprintf(stdout, "\t -s [BIOS option] [value] - Set the given BIOS option to given value\n");
	fprintf(stdout, "\t -p [password] [encoding] [kbdlang] - Set authentication details. \n");
#if 0 /*To be fixed*/
	fprintf(stdout, "\t -c [password] [new password] [password type] [encoding] [kbdlang] - Change password. \n");
#endif
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
	change_password
    } option;

    if (getuid()!=0) {
	    printf("Please run with administrator privileges\n");
	    exit(0);
    }
    switch(argc)
    {
	    case 2:
		    if (strcmp(argv[1], "getsettings") == 0) {
			    option = get_settings;
		    } else {
			    show_usage();
			    return 1;
		    }
		    break;
	    case 3:
		    if (strcmp(argv[1], "-g") == 0)
			    option = get;
		    else
			    show_usage();
		    break;
	    case 4:
		    if (strcmp(argv[1], "-s") == 0) {
			    option = set;
			    printf("%s %s \n", argv[2], argv[3]);
		    } else 
			    show_usage();
		    break;
	    case 5:
		    if (strcmp(argv[1], "-p") == 0)
			    option = authenticate;
		    else
			    show_usage();
		    break;
	    case 7:
		    if (strcmp(argv[1], "-c") == 0)
			    option = change_password;
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
#if 0 /*To be fixed*/
	    case change_password:
		    thinklmi_change_password(fd, argv[2], argv[3], argv[4], argv[5], argv[6]);
		    break;
#endif
    }
    close (fd);
 
    return 0;
} 
