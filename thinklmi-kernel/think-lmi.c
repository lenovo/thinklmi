// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Think LMI BIOS configuration driver
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Original code from Thinkpad-wmi project
 *  https://github.com/iksaif/thinkpad-wmi
 *  Copyright(C) 2017 Corentin Chary <corentin.chary@gmail.com>
 *  Distributed under the GPL-2.0 license
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/acpi.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wmi.h>
#include <linux/acpi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include "think-lmi.h"

#define	THINK_LMI_FILE	"think-lmi"

MODULE_AUTHOR("Sugumaran L <slacshiminar@lenovo.com>");
MODULE_AUTHOR("Mark Pearson <mpearson@lenovo.com>");
MODULE_AUTHOR("Corentin Chary <corentin.chary@gmail.com>");
MODULE_DESCRIPTION("Think LMI Driver");
MODULE_LICENSE("GPL");

/* LMI interface */

/**
 * Name:
 *  Lenovo_BiosSetting
 * Description:
 *  Get item name and settings for current LMI instance.
 * Type:
 *  Query
 * Returns:
 *  "Item,Value"
 * Example:
 *  "WakeOnLAN,Enable"
 */
#define LENOVO_BIOS_SETTING_GUID		\
	"51F5230E-9677-46CD-A1CF-C0B23EE34DB7"

/**
 * Name:
 *  Lenovo_SetBiosSetting
 * Description:
 *  Change the BIOS setting to the desired value using the
 *  Lenovo_SetBiosSetting class. To save the settings,
 *  use the Lenovo_SaveBiosSetting class.
 *  BIOS settings and values are case sensitive.
 *  After making changes to the BIOS settings,
 *  you must reboot the computer
 *  before the changes will take effect.
 * Type:
 *  Method
 * Arguments:
 *  "Item,Value,Password,Encoding,KbdLang;"
 * Example:
 *  "WakeOnLAN,Disable,pswd,ascii,us;"
 */
#define LENOVO_SET_BIOS_SETTINGS_GUID		\
	"98479A64-33F5-4E33-A707-8E251EBBC3A1"

/**
 * Name:
 *  Lenovo_SaveBiosSettings
 * Description:
 *  Save any pending changes in settings.
 * Type:
 *  Method
 * Arguments:
 *  "Password,Encoding,KbdLang;"
 * Example:
 * "pswd,ascii,us;"
 */
#define LENOVO_SAVE_BIOS_SETTINGS_GUID		\
	"6A4B54EF-A5ED-4D33-9455-B0D9B48DF4B3"


/**
 * Name:
 *  Lenovo_DiscardBiosSettings
 * Description:
 *  Discard any pending changes in settings.
 * Type:
 *  Method
 * Arguments:
 *  "Password,Encoding,KbdLang;"
 * Example:
 *  "pswd,ascii,us;"
 */
#define LENOVO_DISCARD_BIOS_SETTINGS_GUID	\
	"74F1EBB6-927A-4C7D-95DF-698E21E80EB5"

/**
 * Name:
 *  Lenovo_LoadDefaultSettings
 * Description:
 *  Load default BIOS settings. Use Lenovo_SaveBiosSettings to save the
 *  settings.
 * Type:
 *  Method
 * Arguments:
 *  "Password,Encoding,KbdLang;"
 * Example:
 *  "pswd,ascii,us;"
 */
#define LENOVO_LOAD_DEFAULT_SETTINGS_GUID	\
	"7EEF04FF-4328-447C-B5BB-D449925D538D"

/**
 * Name:
 *  Lenovo_BiosPasswordSettings
 * Description:
 *  Return BIOS Password settings
 * Type:
 *  Query
 * Returns:
 *  PasswordMode, PasswordState, MinLength, MaxLength,
 *  SupportedEncoding, SupportedKeyboard
 */
#define LENOVO_BIOS_PASSWORD_SETTINGS_GUID		\
	"8ADB159E-1E32-455C-BC93-308A7ED98246"

/**
 * Name:
 *  Lenovo_SetBiosPassword
 * Description:
 *  Change a specific password.
 *  - BIOS settings cannot be changed at the same boot as power-on
 *    passwords (POP) and hard disk passwords (HDP). If you want to change
 *    BIOS settings and POP or HDP, you must reboot the system after
 *    changing one of them.
 *  - A password cannot be set using this method when one does not already
 *    exist. Passwords can only be updated or cleared.
 * Type:
 *  Method
 * Arguments:
 *  "PasswordType,CurrentPassword,NewPassword,Encoding,KbdLang;"
 * Example:
 *  "pop,oldpop,newpop,ascii,us;”
 */
#define LENOVO_SET_BIOS_PASSWORD_GUID	\
	"2651D9FD-911C-4B69-B94E-D0DED5963BD7"

/**
 * Name:
 *  Lenovo_GetBiosSelections
 * Description:
 *  Return a list of valid settings for a given item.
 * Type:
 *  Method
 * Arguments:
 *  "Item"
 * Returns:
 *  "Value1,Value2,Value3,..."
 * Example:
 *  -> "FlashOverLAN"
 *  <- "Enabled,Disabled"
 */
#define LENOVO_GET_BIOS_SELECTIONS_GUID	\
	"7364651A-132F-4FE7-ADAA-40C6C7EE2E3B"

/**
 * Name:
 *  Lenovo_PlatformSettingGUID, Lenovo_SetPlatformSettingGUID
 * Description
 *  debugfs method to get/set platform setting
 * Type:
 *  Method
 * Arguments:
 *  ???
 * Example:
 *  ???
 * LMI-Internals:
 *  Return big chunk of data
 */
#define LENOVO_PLATFORM_SETTING_GUID \
	"7430019A-DCE9-4548-BAB0-9FDE0935CAFF"

#define LENOVO_SET_PLATFORM_SETTINGS_GUID \
	"7FF47003-3B6C-4E5E-A227-E979824A85D1"
/* For future use
 * #define LENOVO_QUERY_GUID "05901221-D566-11D1-B2F0-00A0C9062910"
 */


/**
 * Name:
 *  Lenovo_lmiopcode_setting_guid
 * Description
 *  Alternative setting method with advanced features
 */
#define LENOVO_LMIOPCODE_SETTING_GUID \
	"DFDDEF2C-57D4-48CE-B196-0FB787D90836"

#define TLMI_NAME "thinklmi"

/* Return values */
enum {
	/*
	 * "Success"
	 * Operation completed successfully.
	 */
	THINK_LMI_SUCCESS = 0,
	/*
	 * "Not Supported"
	 * The feature is not supported on this system.
	 */
	THINK_LMI_NOT_SUPPORTED = -ENODEV,
	/*
	 * "Invalid"
	 * The item or value provided is not valid parameter
	 */
	THINK_LMI_INVALID = -EINVAL,
	/*
	 * "Access Denied"
	 * The change could not be made due to an authentication problem.
	 * If a supervisor password exists, the correct supervisor password
	 * must be provided.
	 */
	THINK_LMI_ACCESS_DENIED = -EPERM,
	/* "System Busy"
	 * BIOS changes have already been made that need to be committed.
	 * Reboot the system and try again.
	 */
	THINK_LMI_SYSTEM_BUSY = -EBUSY
};

#define TLMI_NUM_DEVICES 1

MODULE_ALIAS("tlmi:"LENOVO_BIOS_SETTING_GUID);

struct think_lmi_pcfg {
	uint32_t password_mode;
	uint32_t password_state;
	uint32_t min_length;
	uint32_t max_length;
	uint32_t supported_encodings;
	uint32_t supported_keyboard;
};

struct think_lmi {
	struct wmi_device *wmi_device;

	int settings_count;

	char password[TLMI_PWD_MAXLEN];
	char password_encoding[TLMI_ENC_MAXLEN];
	char password_kbdlang[TLMI_LANG_MAXLEN]; /* 2 bytes for \n\0 */
	char auth_string[TLMI_PWD_MAXLEN + TLMI_ENC_MAXLEN
		                      + TLMI_LANG_MAXLEN + 2];
	char password_type[TLMI_PWDTYPE_MAXLEN];
	char tpm_type[TLMI_TPMTYPE_MAXLEN];
	char passcurr[TLMI_PWD_MAXLEN];
	char passnew[TLMI_PWD_MAXLEN];

	bool can_set_bios_settings;
	bool can_discard_bios_settings;
	bool can_load_default_settings;
	bool can_get_bios_selections;
	bool can_set_bios_password;
	bool can_get_password_settings;

	unsigned char *settings[TLMI_MAX_SETTINGS];
	struct dev_ext_attribute *devattrs;
	struct cdev c_dev;
};

static dev_t tlmi_dev;
static struct class *tlmi_class;

static int think_lmi_errstr_to_err(const char *errstr)
{
	if (!strcmp(errstr, "Success"))
		return THINK_LMI_SUCCESS;
	if (!strcmp(errstr, "Not Supported"))
		return THINK_LMI_NOT_SUPPORTED;
	if (!strcmp(errstr, "Invalid"))
		return THINK_LMI_INVALID;
	if (!strcmp(errstr, "Access Denied"))
		return THINK_LMI_ACCESS_DENIED;
	if (!strcmp(errstr, "System Busy"))
		return THINK_LMI_SYSTEM_BUSY;

	pr_debug("Unknown error string: '%s'", errstr);

	return -EINVAL;
}

static int think_lmi_extract_error(const struct acpi_buffer *output)
{
	const union acpi_object *obj;
	int ret;

	obj = output->pointer;
	if (!obj || obj->type != ACPI_TYPE_STRING || !obj->string.pointer)
		return -EIO;

	ret = think_lmi_errstr_to_err(obj->string.pointer);
	kfree(obj);
	return ret;
}

static int think_lmi_simple_call(const char *guid,
				    const char *arg)
{
	const struct acpi_buffer input = { strlen(arg), (char *)arg };
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_status status;
	/*
	 * duplicated call required to match bios workaround for behavior
	 * seen when WMI accessed via scripting on other OS
	 */
	status = wmi_evaluate_method(guid, 0, 0, &input, &output);
	status = wmi_evaluate_method(guid, 0, 0, &input, &output);

	if (ACPI_FAILURE(status))
		return -EIO;

	return think_lmi_extract_error(&output);
}

static int think_lmi_extract_output_string(const struct acpi_buffer
		                     *output,  char **string)
{
	const union acpi_object *obj;

	obj = output->pointer;
	if (!obj || obj->type != ACPI_TYPE_STRING || !obj->string.pointer)
		return -EIO;

	*string = kstrdup(obj->string.pointer, GFP_KERNEL);
	kfree(obj);
	return *string ? 0 : -ENOMEM;
}

static int think_lmi_setting(int item, char **value,
		                  const char *guid_string)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_status status;

	status = wmi_query_block(guid_string, item, &output);
	if (ACPI_FAILURE(status))
		return -EIO;

	return think_lmi_extract_output_string(&output, value);
}

static int think_lmi_get_bios_selections(const char *item, char **value)
{
	const struct acpi_buffer input = { strlen(item), (char *)item };
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_status status;

	status = wmi_evaluate_method(LENOVO_GET_BIOS_SELECTIONS_GUID,
				     0, 0, &input, &output);

	if (ACPI_FAILURE(status))
		return -EIO;

	return think_lmi_extract_output_string(&output, value);
}

static int think_lmi_set_bios_settings(const char *settings)
{
	int spleng = 0;
	int num = 0;
	char *ret, *arg;
	spleng = strlen(settings);
	ret = strstr(settings, "\\");
	num = ret - settings;

	arg = kmalloc(spleng, GFP_KERNEL);
	strcpy(arg, settings);
	arg[num] = '/';

	strcpy(settings, arg);
	kfree(arg);
	return think_lmi_simple_call(LENOVO_SET_BIOS_SETTINGS_GUID, settings);
}

static int think_lmi_save_bios_settings(const char *password)
{
	return think_lmi_simple_call(LENOVO_SAVE_BIOS_SETTINGS_GUID, password);
}

static int think_lmi_discard_bios_settings(const char *password)
{
	return think_lmi_simple_call(LENOVO_DISCARD_BIOS_SETTINGS_GUID, password);
}

static int think_lmi_set_bios_password(const char *settings)
{
	return think_lmi_simple_call(LENOVO_SET_BIOS_PASSWORD_GUID, settings);
}

static int think_lmi_set_platform_settings(const char *settings)
{
	return think_lmi_simple_call(LENOVO_SET_PLATFORM_SETTINGS_GUID, settings);
}

static int think_lmi_set_lmiopcode_settings(const char *settings)
{
	return think_lmi_simple_call(LENOVO_LMIOPCODE_SETTING_GUID, settings);
}
static int think_lmi_load_default(const char *password)
{
	return think_lmi_simple_call(LENOVO_LOAD_DEFAULT_SETTINGS_GUID, password);
}

/* Create the auth string from password chunks */
static void update_auth_string(struct think_lmi *think)
{
	if (!*think->password) {
		/* No password at all */
		think->auth_string[0] = '\0';
		return;
	}
	strcpy(think->auth_string, think->password);

	if (*think->password_encoding) {
		strcat(think->auth_string, ",");
		strcat(think->auth_string, think->password_encoding);
	}

	if (*think->password_kbdlang) {
		strcat(think->auth_string, ",");
		strcat(think->auth_string, think->password_kbdlang);
	}
}

static int validate_setting_name(struct think_lmi *think, char* setting)
{
	int i;
	for (i = 0; i <= TLMI_MAX_SETTINGS; i++) {
		if (think->settings[i] != NULL) {
			if (!strcmp(setting, think->settings[i]))
					return i;
		}
	}
	/* No match found - return error condition */
	return -EINVAL;
}

/* Character device open interface */
static int think_lmi_chardev_open(struct inode *inode, struct file *file)
{
	struct think_lmi *think;
	think = container_of(inode->i_cdev, struct think_lmi, c_dev);
	file->private_data = think;
        return THINK_LMI_SUCCESS;
}

/* Character device ioctl interface */
static long think_lmi_chardev_ioctl(struct file *filp, unsigned int cmd,
					unsigned long arg)
{
        struct think_lmi *think;
	int j,ret,item;
	unsigned char settings_str[TLMI_SETTINGS_MAXLEN];
	char get_set_string[TLMI_GETSET_MAXLEN];
	char newpassword[TLMI_PWD_MAXLEN];
	char *settings = NULL, *choices = NULL;
	char *value;
	char *tmp_string = NULL;
	ssize_t count =0;

	think = filp->private_data;
	switch(cmd){
	case THINKLMI_GET_SETTINGS:
		if (copy_to_user((int *)arg, &think->settings_count,
				 sizeof(think->settings_count)))
			return -EFAULT;
		break;
	case THINKLMI_GET_SETTINGS_STRING:
		/* Get the string for given index */
		if (copy_from_user(settings_str, (void *)arg,
				   sizeof(settings_str)))
			return -EFAULT;
		j = settings_str[0];
		if ((j > TLMI_MAX_SETTINGS) || (!think->settings[j]))
			return -EINVAL;
		strncpy(settings_str, think->settings[j],
				(TLMI_SETTINGS_MAXLEN-1));
		if (copy_to_user((char *)arg, settings_str,
				 sizeof(settings_str)))
			return -EFAULT;
		break;
	case THINKLMI_SET_SETTING:
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;

		/* First validate that this is a valid setting name */
		value = strchr(get_set_string, ',');
		if (!value) {
			ret = -EINVAL;
			goto error;
		}
		tmp_string = kmalloc(value - get_set_string + 1, GFP_KERNEL);
		snprintf(tmp_string, value - get_set_string + 1, "%s",
			                get_set_string);
		ret = validate_setting_name(think, tmp_string);
		kfree(tmp_string);
		if (ret < 0)
			goto error;

		/* If authorisation required add that to command */
		if (*think->auth_string) {
			count = strlen(get_set_string) +
				strlen(think->auth_string) + 2;
			tmp_string = kmalloc(count, GFP_KERNEL);
			snprintf(tmp_string, count, "%s,%s;",
					get_set_string, think->auth_string);
		} else {
			count =  strlen(get_set_string) + 1;
			tmp_string = kmalloc(count, GFP_KERNEL);
			snprintf(tmp_string, count, "%s;", get_set_string);
		}

		ret = think_lmi_set_bios_settings(tmp_string);
		kfree(tmp_string);

		ret = think_lmi_save_bios_settings(think->auth_string);
                if (ret) {
			/* Try to discard the settings
			 * if we failed to apply them */
			think_lmi_discard_bios_settings(think->
					         auth_string);
			goto error;
                }
		break;
	case THINKLMI_SHOW_SETTING:
		item = -1;
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;
		item = validate_setting_name(think, get_set_string);
		if (item < 0) { /* Invalid entry */
			ret = -EINVAL;
			goto error;
		}
		/* Do a WMI query for the settings */
		ret = think_lmi_setting(item, &settings,
				          LENOVO_BIOS_SETTING_GUID);
		if (ret)
			goto error;

		if (think->can_get_bios_selections)
		{
			ret = think_lmi_get_bios_selections(get_set_string,
						    &choices);
			if (ret)
				goto error;
			if (choices) {
				value = strchr(settings, ',');
				if (!value)
					goto error;
				value++;
				/* Allocate enough space for value,
				 * choices, return chars and
				 * null terminate
				 */
				tmp_string = (char *)kmalloc(strlen(value)
					        + strlen(choices) + 5,
						      GFP_KERNEL);
				count = sprintf(tmp_string, "%s\n", value);
				count += sprintf(tmp_string + count,
						"%s\n",	choices);
			}
		} else {
			/* BIOS doesn't support choices
			 * option - it's all in one string */
			tmp_string = (char *)kmalloc(strlen(settings)
				           + 3, GFP_KERNEL);
			count = sprintf(tmp_string, "%s\n", settings);
		}
		if (count > TLMI_SETTINGS_MAXLEN) {
			/* Unlikely to happen - but if the string is going
			 * to overflow the amount of space that is
			 * available then we need to truncate.
			 * Issue a warning so we know about these
			 */
			count = TLMI_SETTINGS_MAXLEN;
			pr_warn("WARNING: Result truncated to fit string buffer\n");
		}
		tmp_string[count-1] = '\0';
		if (copy_to_user((char *)arg, tmp_string, count)) {
			kfree(tmp_string);
			goto error;
		}
		kfree(tmp_string);
		break;
        case THINKLMI_AUTHENTICATE:
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;
		tmp_string = get_set_string;
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password, TLMI_PWD_MAXLEN, "%s", value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_encoding, TLMI_ENC_MAXLEN,
			                     "%s", value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_kbdlang, TLMI_LANG_MAXLEN,
			                      "%s", value);

		update_auth_string(think);
		break;

	case THINKLMI_CHANGE_PASSWORD:
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;
		snprintf(settings_str, TLMI_SETTINGS_MAXLEN, "%s",
				             get_set_string);
		tmp_string = get_set_string;

                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_type, TLMI_PWDTYPE_MAXLEN,
			                   "%s",value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password, TLMI_PWD_MAXLEN, "%s", value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(newpassword, TLMI_PWD_MAXLEN, "%s",value);
		value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_encoding, TLMI_ENC_MAXLEN,
			                    "%s",value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_kbdlang, TLMI_LANG_MAXLEN,
			                     "%s",value);

		update_auth_string(think);

	        ret = think_lmi_set_bios_password(settings_str);
		break;

	case THINKLMI_DEBUG:
		if (copy_from_user(get_set_string, (void *)arg,
				            sizeof(get_set_string)))
			return -EFAULT;
		snprintf(settings_str, TLMI_SETTINGS_MAXLEN, "%s",
				            get_set_string);
		ret = think_lmi_set_platform_settings(settings_str);
                if (ret) {
			goto error;
                }
		break;

	case THINKLMI_LMIOPCODE:
		if (copy_from_user(get_set_string, (void *)arg, sizeof(get_set_string)))
			return -EFAULT;
		snprintf(settings_str, TLMI_SETTINGS_MAXLEN, "%s", get_set_string);

		tmp_string = get_set_string;

		value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;

		snprintf(think->password, TLMI_PWD_MAXLEN, "%s", get_set_string);
		sprintf(settings_str, "WmiOpcodePasswordAdmin:%s;", think->password);

		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			goto error;

		value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;

		snprintf(think->password_type, TLMI_PWDTYPE_MAXLEN, "%s", value);
		sprintf(settings_str, "WmiOpcodePasswordType:%s;", think->password_type);

		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			goto error;

		value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;

		snprintf(think->passcurr, TLMI_PWD_MAXLEN, "%s", value);
		sprintf(settings_str, "WmiOpcodePasswordCurrent01:%s;", think->passcurr);
		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			goto error;

		value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;

		snprintf(think->passnew, TLMI_PWD_MAXLEN, "%s", value);
		sprintf(settings_str, "WmiOpcodePasswordNew01:%s", think->passnew);
		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			goto error;

		sprintf(settings_str, "WmiOpcodePasswordSetUpdate;");
		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			goto error;

		break;
	case THINKLMI_TPMTYPE:
		if (copy_from_user(get_set_string, (void *)arg,
					sizeof(get_set_string)))
			return -EFAULT;

		sprintf(settings_str, "WmiOpcodeTPM:");
		strncat(settings_str, get_set_string, TLMI_SETTINGS_MAXLEN);
		ret = think_lmi_set_lmiopcode_settings(settings_str);
		if (ret)
			return -EFAULT;
		ret = think_lmi_save_bios_settings(think->auth_string);
		if (ret)
			return -EFAULT;

		break;
	case THINKLMI_LOAD_DEFAULT:
		ret = think_lmi_load_default(think->auth_string);
		if (ret)
			return -EFAULT;
		break;
	case THINKLMI_SAVE_SETTINGS:
		ret = think_lmi_save_bios_settings(think->auth_string);
		if (ret)
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}

	return THINK_LMI_SUCCESS;

error:
	kfree(settings);
	kfree(choices);
	return ret ? ret : count;
}

static int think_lmi_chardev_release(struct inode *inode,
	                    struct file *file)
{
	return THINK_LMI_SUCCESS;
}

static const struct file_operations think_lmi_chardev_fops = {
	.open           = think_lmi_chardev_open,
	.unlocked_ioctl = think_lmi_chardev_ioctl,
	.release        = think_lmi_chardev_release,
};

static void think_lmi_chardev_initialize(struct think_lmi *think)
{
        int ret;
	struct device *dev_ret;

	ret = alloc_chrdev_region(&tlmi_dev, 0, TLMI_NUM_DEVICES, TLMI_NAME);
	if (ret < 0) {
		pr_warn("tlmi: char dev allocation failed\n");
		return;
	}

	cdev_init(&think->c_dev, &think_lmi_chardev_fops);

	ret = cdev_add(&think->c_dev, tlmi_dev, TLMI_NUM_DEVICES);
	if (ret < 0) {
		pr_warn("tlmi: char dev registration failed\n");
		unregister_chrdev_region(tlmi_dev, TLMI_NUM_DEVICES);
		return;
	}

	tlmi_class = class_create(THIS_MODULE, "char");
	if (IS_ERR(tlmi_class)) {
		pr_warn("tlmi: char dev class creation failed\n");
		cdev_del(&think->c_dev);
		unregister_chrdev_region(tlmi_dev, TLMI_NUM_DEVICES);
		return;
        }
	dev_ret = device_create(tlmi_class, NULL, tlmi_dev,
		                    NULL, "thinklmi");
	if (IS_ERR(dev_ret)) {
		pr_warn("tlmi: char dev device creation failed\n");
		class_destroy(tlmi_class);
		cdev_del(&think->c_dev);
		unregister_chrdev_region(tlmi_dev, TLMI_NUM_DEVICES);
        }
}

static void think_lmi_chardev_exit(struct think_lmi *think)
{
	device_destroy(tlmi_class, tlmi_dev);
	class_destroy(tlmi_class);
	cdev_del(&think->c_dev);
	unregister_chrdev_region(tlmi_dev, TLMI_NUM_DEVICES);
}

static void think_lmi_analyze(struct think_lmi *think)
{
	acpi_status status;
	int i = 0;

	/*
	 * Try to find the number of valid settings of this machine
	 * and use it to create sysfs attributes
	 */
	for (i = 0; i < TLMI_MAX_SETTINGS; ++i) {
		char *item = NULL;
		int spleng = 0;
		int num = 0;
		char *p;

		status = think_lmi_setting(i, &item,
			        LENOVO_BIOS_SETTING_GUID);
		if (ACPI_FAILURE(status))
			break;
		if (!item )
			break;
		if (!*item)
			continue;

		/* It is not allowed to have '/' for file name.
		 * Convert it into '\'. */
		spleng = strlen(item);
		for (num = 0; num < spleng; num++) {
			if (item[num] == '/')
				item[num] = '\\';
		}

		/* Remove the value part */
		p = strchr(item, ',');
		if (p)
			*p = '\0';
		think->settings[i] = item; /* Cache setting name */
		think->settings_count++;
	}

	if (wmi_has_guid(LENOVO_SET_BIOS_SETTINGS_GUID) &&
	    wmi_has_guid(LENOVO_SAVE_BIOS_SETTINGS_GUID))
		think->can_set_bios_settings = true;

	if (wmi_has_guid(LENOVO_DISCARD_BIOS_SETTINGS_GUID))
		think->can_discard_bios_settings = true;

	if (wmi_has_guid(LENOVO_LOAD_DEFAULT_SETTINGS_GUID))
		think->can_load_default_settings = true;

	if (wmi_has_guid(LENOVO_GET_BIOS_SELECTIONS_GUID))
		think->can_get_bios_selections = true;

	if (wmi_has_guid(LENOVO_SET_BIOS_PASSWORD_GUID))
		think->can_set_bios_password = true;

	if (wmi_has_guid(LENOVO_BIOS_PASSWORD_SETTINGS_GUID))
		think->can_get_password_settings = true;
}

static int think_lmi_add(struct wmi_device *wdev)
{
	struct think_lmi *think;

	think = kzalloc(sizeof(struct think_lmi), GFP_KERNEL);
	if (!think)
		return -ENOMEM;

	think->wmi_device = wdev;
	dev_set_drvdata(&wdev->dev, think);

	think_lmi_chardev_initialize(think);

	think_lmi_analyze(think);
	return 0;
}

static
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
void
#else
int
#endif
think_lmi_remove(struct wmi_device *wdev)
{
	struct think_lmi *think;
	int i;

	think = dev_get_drvdata(&wdev->dev);
	think_lmi_chardev_exit(think);

	for (i = 0; think->settings[i]; ++i) {
		kfree(think->settings[i]);
		think->settings[i] = NULL;
	}

	kfree(think);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	return;
#else
	return 0;
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
static int think_lmi_probe(struct wmi_device *wdev, const void *context)
#else
static int think_lmi_probe(struct wmi_device *wdev)
#endif
{
	return think_lmi_add(wdev);
}

static const struct wmi_device_id think_lmi_id_table[] = {
	/* Search for Lenovo_BiosSetting */
	{ .guid_string = LENOVO_BIOS_SETTING_GUID },
	{ },
};

static struct wmi_driver think_lmi_driver = {
	.driver = {
		.name = "think-lmi",
	},
	.id_table = think_lmi_id_table,
	.probe = think_lmi_probe,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
	.remove = think_lmi_remove,
#else
	.remove = (int (*)(struct wmi_device *))think_lmi_remove,
#endif
};

static int __init think_lmi_init(void)
{
	return wmi_driver_register(&think_lmi_driver);
}

static void __exit think_lmi_exit(void)
{
	wmi_driver_unregister(&think_lmi_driver);
}

module_init(think_lmi_init);
module_exit(think_lmi_exit);
