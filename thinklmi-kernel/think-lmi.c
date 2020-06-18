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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Original code from Thinkpad-wmi project https://github.com/iksaif/thinkpad-wmi
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

/* LMI inteface */

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
 *  Change the BIOS setting to the desired value using the Lenovo_SetBiosSetting
 *  class. To save the settings, use the Lenovo_SaveBiosSetting class.
 *  BIOS settings and values are case sensitive.
 *  After making changes to the BIOS settings, you must reboot the computer
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
 *    BIOS settings and POP or HDP, you must reboot the system after changing
 *    one of them.
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

/* Only add an alias on this one, since it's the one used
 * in think_lmi_probe */
MODULE_ALIAS("tlmi:"LENOVO_BIOS_SETTING_GUID);

struct think_lmi_pcfg {
	uint32_t password_mode;
	uint32_t password_state;
	uint32_t min_length;
	uint32_t max_length;
	uint32_t supported_encodings;
	uint32_t supported_keyboard;
};

/*
 * think_lmi/       - debugfs root directory
 *   bios_settings
 *   bios_setting
 *   list_valid_choices
 *   set_bios_settings
 *   save_bios_settings
 *   discard_bios_settings
 *   load_default
 *   set_bios_password
 *   argument
 *   instance
 *   instance_count
 *   bios_password_settings
 */
struct think_lmi_debug {
	struct dentry *root;
	u8 instances_count;
	u8 instance;
	char argument[512];
};

struct think_lmi {
	struct wmi_device *wmi_device;

	int settings_count; 

	char password[TLMI_PWD_MAXLEN];
	char password_encoding[TLMI_ENC_MAXLEN];
	char password_kbdlang[TLMI_LANG_MAXLEN]; /* 2 bytes for \n\0 */
	char auth_string[TLMI_PWD_MAXLEN + TLMI_ENC_MAXLEN + TLMI_LANG_MAXLEN + 2]; 
	char password_type[TLMI_PWDTYPE_MAXLEN]; //Lenovo linux utility team

	bool can_set_bios_settings;
	bool can_discard_bios_settings;
	bool can_load_default_settings;
	bool can_get_bios_selections;
	bool can_set_bios_password;
	bool can_get_password_settings;

	char *settings[256];
	struct dev_ext_attribute *devattrs;
	struct think_lmi_debug debug;
	struct cdev c_dev;
};

static dev_t tlmi_dev;
static struct class *tlmi_class;

/* helpers */
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

static int think_lmi_extract_output_string(const struct acpi_buffer *output,
					      char **string)
{
	const union acpi_object *obj;

	obj = output->pointer;
	if (!obj || obj->type != ACPI_TYPE_STRING || !obj->string.pointer)
		return -EIO;

	*string = kstrdup(obj->string.pointer, GFP_KERNEL);
	kfree(obj);
	return *string ? 0 : -ENOMEM;
}

static int think_lmi_setting(int item, char **value, const char *guid_string)
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

	return think_lmi_simple_call(LENOVO_SET_BIOS_SETTINGS_GUID,
					settings);
}

static int think_lmi_set_platform_settings(const char *settings)
{

	return think_lmi_simple_call(LENOVO_SET_PLATFORM_SETTINGS_GUID,
					settings);
}

static int think_lmi_save_bios_settings(const char *password)
{
	return think_lmi_simple_call(LENOVO_SAVE_BIOS_SETTINGS_GUID,
					password);
}

static int think_lmi_discard_bios_settings(const char *password)
{
	return think_lmi_simple_call(LENOVO_DISCARD_BIOS_SETTINGS_GUID,
					password);
}

static int think_lmi_load_default(const char *password)
{
	return think_lmi_simple_call(LENOVO_LOAD_DEFAULT_SETTINGS_GUID,
					password);
}

#if 0 /*TO BE FIXED*/
static int think_lmi_set_bios_password(const char *settings)
{
	return think_lmi_simple_call(LENOVO_SET_BIOS_PASSWORD_GUID,
					settings);
}
#endif

static int think_lmi_password_settings(struct think_lmi_pcfg *pcfg)
{
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	const union acpi_object *obj;
	acpi_status status;

	status = wmi_query_block(LENOVO_BIOS_PASSWORD_SETTINGS_GUID, 0,
				 &output);
	if (ACPI_FAILURE(status))
		return -EIO;

	obj = output.pointer;
	if (!obj || obj->type != ACPI_TYPE_BUFFER || !obj->buffer.pointer)
		return -EIO;
	if (obj->buffer.length != sizeof(*pcfg)) {

		/* The size of thinkpad_wmi_pcfg on ThinkStation is larger than ThinkPad.
		 * To make the driver compatible on different brands, we permit it to get
		 * the data in below case.
		 */
		if (obj->buffer.length > sizeof(*pcfg)) {
			memcpy(pcfg, obj->buffer.pointer, sizeof(*pcfg));
			kfree(obj);
			return 0;
		} else {
			pr_warn("Unknown pcfg buffer length %d\n",
				obj->buffer.length);
			kfree(obj);
			return -EIO;
		}
	}

	memcpy(pcfg, obj->buffer.pointer, obj->buffer.length);
	kfree(obj);
	return 0;
}

/* sysfs */

#define to_ext_attr(x) container_of(x, struct dev_ext_attribute, attr)

static ssize_t show_setting(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	struct think_lmi *think = dev_get_drvdata(dev);
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	int item = (uintptr_t)ea->var;
	char *name = think->settings[item];
	char *settings = NULL, *choices = NULL, *value;
	ssize_t count = 0;
	int ret;

	ret = think_lmi_setting(item, &settings, LENOVO_BIOS_SETTING_GUID);
	if (ret)
		return ret;
	if (!settings)
		return -EIO;

	if (think->can_get_bios_selections) {
		ret = think_lmi_get_bios_selections(name, &choices);
		if (ret)
			goto error;
		if (!choices || !*choices) {
			ret = -EIO;
			goto error;
		}
	}

	value = strchr(settings, ',');
	if (!value)
		goto error;
	value++;

	count = sprintf(buf, "%s\n", value);
	if (choices)
		count += sprintf(buf + count, "%s\n", choices);

error:
	kfree(settings);
	if (choices)
		kfree(choices);
	return ret ? ret : count;
}

static ssize_t store_setting(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct think_lmi *think = dev_get_drvdata(dev);
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	int item_idx = (uintptr_t)ea->var;
	char *item = think->settings[item_idx];
	int ret;
	size_t buffer_size;
	char *buffer;

	/* Convert '\' to '/'. Please have a look at think_lmi_analyze. */
	int spleng = 0;
	int num = 0;

	spleng = strlen(item);
	for (num = 0; num < spleng; num++) {
		if (item[num] == '\\')
			item[num] = '/';
	}

	/* Format: 'Item,Value,Authstring;' */
	buffer_size = (strlen(item) + 1 + count + 1 +
		       sizeof(think->auth_string) + 2);
	buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	strcpy(buffer, item);
	strcat(buffer, ",");
	strncat(buffer, buf, count);
	if (count)
		strim(buffer);
	if (*think->auth_string) {
		strcat(buffer, ",");
		strcat(buffer, think->auth_string);
	}
	strcat(buffer, ";");

	ret = think_lmi_set_bios_settings(buffer);
	if (ret)
		goto end;

	ret = think_lmi_save_bios_settings(think->auth_string);
	if (ret) {
		/* Try to discard the settings if we failed to apply them. */
		think_lmi_discard_bios_settings(think->auth_string);
		goto end;
	}
	ret = count;

end:
	kfree(buffer);
	return ret;
}


/* Password related sysfs methods */
static ssize_t show_auth(struct think_lmi *think, char *buf,
			 const char *data, size_t size)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (!buf)
		return -EIO;

	return sprintf(buf, "%s\n", data ? : "(nil)");
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

static ssize_t store_auth(struct think_lmi *think,
			  const char *buf, size_t count,
			  char *dst, size_t size)
{
	ssize_t ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (count > size - 1)
		return -EINVAL;

	/* dst may be being reused, NUL-terminate */
	ret = strscpy(dst, buf, size);
	if (ret < 0)
		return ret;
	if (count)
		strim(dst);

	update_auth_string(think);

	return count;
}

#define THINK_LMI_CREATE_AUTH_ATTR(_name, _uname, _mode)		\
	static ssize_t show_##_name(struct device *dev,			\
				    struct device_attribute *attr,	\
				    char *buf)				\
	{								\
		struct think_lmi *think = dev_get_drvdata(dev);		\
									\
		return show_auth(think, buf,				\
				 think->_name,				\
				 sizeof(think->_name));			\
	}								\
	static ssize_t store_##_name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     const char *buf, size_t count)	\
	{								\
		struct think_lmi *think = dev_get_drvdata(dev);		\
									\
		return store_auth(think, buf, count,			\
				  think->_name,				\
				  sizeof(think->_name));		\
	}								\
	static struct device_attribute dev_attr_##_name = {		\
		.attr = {						\
			.name = _uname,					\
			.mode = _mode },				\
		.show   = show_##_name,					\
		.store  = store_##_name,				\
	}

THINK_LMI_CREATE_AUTH_ATTR(password, "password", S_IRUSR|S_IWUSR);
THINK_LMI_CREATE_AUTH_ATTR(password_encoding, "password_encoding",
			      S_IRUSR|S_IWUSR);
THINK_LMI_CREATE_AUTH_ATTR(password_kbdlang, "password_kbd_lang",
			      S_IRUSR|S_IWUSR);
THINK_LMI_CREATE_AUTH_ATTR(password_type, "password_type", S_IRUSR|S_IWUSR);

static ssize_t show_password_settings(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct think_lmi_pcfg pcfg;
	ssize_t ret;

	ret = think_lmi_password_settings(&pcfg);
	if (ret)
		return ret;
	ret += sprintf(buf,       "password_mode:       %#x\n",
		       pcfg.password_mode);
	ret += sprintf(buf + ret, "password_state:      %#x\n",
		       pcfg.password_state);
	ret += sprintf(buf + ret, "min_length:          %d\n", pcfg.min_length);
	ret += sprintf(buf + ret, "max_length:          %d\n", pcfg.max_length);
	ret += sprintf(buf + ret, "supported_encodings: %#x\n",
		       pcfg.supported_encodings);
	ret += sprintf(buf + ret, "supported_keyboard:  %#x\n",
		       pcfg.supported_keyboard);
	return ret;
}

static DEVICE_ATTR(password_settings, S_IRUSR, show_password_settings, NULL);

#if 0 /*TO BE FIXED*/
static ssize_t store_password_change(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct think_lmi *think = dev_get_drvdata(dev);
	size_t buffer_size;
	char *buffer;
	ssize_t ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	/* Format: 'PasswordType,CurrentPw,NewPw,Encoding,KbdLang;' */

	/* auth_string is the size of CurrentPassword,Encoding,KbdLang */
	buffer_size = (sizeof(think->password_type) + 1 + count + 2);

	if (*think->password)
		buffer_size += 1 + sizeof(think->password);
	if (*think->password_encoding)
		buffer_size += 1 + sizeof(think->password_encoding);
	if (*think->password_kbdlang)
		buffer_size += 1 + sizeof(think->password_kbdlang);

	buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	strcpy(buffer, think->password_type);

	if (*think->password) {
		strcat(buffer, ",");
		strcat(buffer, think->password);
	}
	strcat(buffer, ",");
	strncat(buffer, buf, count);
	if (count)
		strim(buffer);

	if (*think->password_encoding) {
		strcat(buffer, ",");
		strcat(buffer, think->password_encoding);
	}
	if (*think->password_kbdlang) {
		strcat(buffer, ",");
		strcat(buffer, think->password_kbdlang);
	}
	strcat(buffer, ";");

	ret = think_lmi_set_bios_password(buffer);
	kfree(buffer);
	if (ret)
		return ret;

	return count;
}

static struct device_attribute dev_attr_password_change = {
	.attr = {
		.name = "password_change",
		.mode = S_IWUSR },
	.store  = store_password_change,
};
#endif

static ssize_t store_load_default(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	struct think_lmi *think = dev_get_drvdata(dev);

	ret = think_lmi_load_default(think->auth_string);
	if (ret)
		return ret;
	return count;

}

static DEVICE_ATTR(load_default_settings, S_IWUSR, NULL, store_load_default);

static struct attribute *platform_attributes[] = {
	&dev_attr_password_settings.attr,
	&dev_attr_password.attr,
	&dev_attr_password_encoding.attr,
	&dev_attr_password_kbdlang.attr,
	&dev_attr_password_type.attr,
#if 0 /*TO BE FIXED*/
	&dev_attr_password_change.attr,
#endif
	&dev_attr_load_default_settings.attr,
	NULL
};

static umode_t think_sysfs_is_visible(struct kobject *kobj,
					 struct attribute *attr,
					 int idx)
{
	bool supported = true;
	return supported ? attr->mode : 0;
}

static struct attribute_group platform_attribute_group = {
	.is_visible	= think_sysfs_is_visible,
	.attrs		= platform_attributes
};

static void think_lmi_sysfs_exit(struct wmi_device *wdev)
{
	struct think_lmi *think = dev_get_drvdata(&wdev->dev);
	int i;

	sysfs_remove_group(&wdev->dev.kobj, &platform_attribute_group);

	if (!think->devattrs)
		return;

	for (i = 0; i < think->settings_count; ++i) {
		struct dev_ext_attribute *deveattr = &think->devattrs[i];
		struct device_attribute *devattr = &deveattr->attr;

		if (devattr->attr.name)
			device_remove_file(&wdev->dev, devattr);
	}
	kfree(think->devattrs);
	think->devattrs = NULL;
}

static int think_lmi_sysfs_init(struct wmi_device *wdev)
{
	struct think_lmi *think = dev_get_drvdata(&wdev->dev);
	struct dev_ext_attribute *devattrs;
	int count = think->settings_count;
	int i, ret;

	devattrs = kzalloc(sizeof(*devattrs) * count, GFP_KERNEL);
	if (!devattrs)
		return -ENOMEM;
	think->devattrs = devattrs;

	for (i = 0; i < count; ++i) {
		struct dev_ext_attribute *deveattr = &devattrs[i];
		struct device_attribute *devattr = &deveattr->attr;
		if (!think->settings[i])
			continue;
		sysfs_attr_init(&devattr->attr);
		devattr->attr.name = think->settings[i];
		devattr->attr.mode = 0644;
		devattr->show = show_setting;
		devattr->store = store_setting;
		deveattr->var = (void *)(uintptr_t)i;
		ret = device_create_file(&wdev->dev, devattr);
		if (ret) {
			/* Name is used to check is file has been created. */
			devattr->attr.name = NULL;
			return ret;
		}
	}

	return sysfs_create_group(&wdev->dev.kobj, &platform_attribute_group);
}

/*
 * Platform device
 */
static int think_lmi_platform_init(struct think_lmi *think)
{
	return think_lmi_sysfs_init(think->wmi_device);
}

static void think_lmi_platform_exit(struct think_lmi *think)
{
	think_lmi_sysfs_exit(think->wmi_device);
}

/* debugfs */

static ssize_t dbgfs_write_argument(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *pos)
{
	struct think_lmi *think = file->f_path.dentry->d_inode->i_private;
	char *kernbuf = think->debug.argument;
	size_t size = sizeof(think->debug.argument);

	if (count > PAGE_SIZE - 1)
		return -EINVAL;

	if (count > size - 1)
		return -EINVAL;

	if (copy_from_user(kernbuf, userbuf, count))
		return -EFAULT;

	kernbuf[count] = 0;

	strim(kernbuf);

	return count;
}

static int dbgfs_show_argument(struct seq_file *m, void *v)
{
	struct think_lmi *think = m->private;

	seq_printf(m, "%s\n", think->debug.argument);
	return 0;
}

static int think_lmi_debugfs_argument_open(struct inode *inode,
					      struct file *file)
{
	struct think_lmi *think = inode->i_private;

	return single_open(file, dbgfs_show_argument, think);
}

static const struct file_operations think_lmi_debugfs_argument_fops = {
	.open		= think_lmi_debugfs_argument_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= dbgfs_write_argument,
};

struct think_lmi_debugfs_node {
	struct think_lmi *think;
	char *name;
	int (*show)(struct seq_file *m, void *data);
};
//Lenovo linux utility team
//Character device open interface
static int think_lmi_chardev_open(struct inode *inode, struct file *file)
{
	struct think_lmi *think;
	think = container_of(inode->i_cdev, struct think_lmi, c_dev);
	file->private_data = think;
        return THINK_LMI_SUCCESS;
}

//Lenovo linux utility team
//Character device ioctl interface
static long think_lmi_chardev_ioctl(struct file *filp, unsigned int cmd,
					unsigned long arg)
{
        struct think_lmi *think;
	int i,j,ret,item;
	char settings_str[TLMI_SETTINGS_MAXLEN];
	char get_set_string[TLMI_GETSET_MAXLEN];
#if 0 /*TO BE FIXED*/
	char newpassword[TLMI_PWD_MAXLEN];
#endif
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
		/*Get the string for given index*/
		if (copy_from_user(settings_str, (void *)arg,
				   sizeof(settings_str)))
			return -EFAULT;
		j = settings_str[0];
		if ((j > think->settings_count) || (!think->settings[j]))
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

		if (*think->auth_string) 
			count = strlen(get_set_string) + strlen(think->auth_string) + 2;
		else
			count =  strlen(get_set_string) + 1;

		tmp_string = kmalloc(count, GFP_KERNEL);
		if (*think->auth_string) 
			snprintf(tmp_string, count, "%s,%s;", get_set_string, think->auth_string);
		else
			snprintf(tmp_string, count, "%s;", get_set_string);

		ret = think_lmi_set_bios_settings(tmp_string);
		kfree(tmp_string);
		ret = think_lmi_save_bios_settings(think->auth_string);
                if (ret) {
			/* Try to discard the settings if we failed to apply them. */
			think_lmi_discard_bios_settings(think->auth_string);
			goto error;
                }
		break;
	case THINKLMI_SHOW_SETTING:
		item = 0;
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;
		//pr_info("%s\n", get_set_string);
		for (i = 0; i <= think->settings_count; i++) {
			if (think->settings[i] != NULL) {
				if (!strcmp(get_set_string,
					    think->settings[i])) {
					pr_info("the setting is %d\n", i);
					item = i;
				}
			}
		}

		/*Do a WMI query for the settings */
		ret = think_lmi_setting(item, &settings,
					LENOVO_BIOS_SETTING_GUID);
		if (ret)
			goto error;

		if (think->can_get_bios_selections)
		{
			printk("Get choices\n");
			ret = think_lmi_get_bios_selections(get_set_string,
						    &choices);
			if (ret)
				goto error;
			if (choices) {
				value = strchr(settings, ',');
				if (!value)
					goto error;
				value++;
				/*Allocate enough space for value, choices, return chars and 
				 * null terminate*/
				tmp_string = (char *)kmalloc(strlen(value) + strlen(choices) + 5, 
								GFP_KERNEL);
				count = sprintf(tmp_string, "%s\n", value);
				count += sprintf(tmp_string + count, "%s\n",
						choices);
			}
		} else {
			/*BIOS doesn't support choices option - it's all in one string...*/
			tmp_string = (char *)kmalloc(strlen(settings) + 3, GFP_KERNEL);
			count = sprintf(tmp_string, "%s\n", settings);
		}
		tmp_string[count-1] = '\0';
		//printk("tmp_string : %s (%ld)\n", tmp_string, count); 
		if (copy_to_user((char *)arg, tmp_string, count)) {
			kfree(tmp_string);
			return -EFAULT;
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
		snprintf(think->password_encoding, TLMI_ENC_MAXLEN, "%s", value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_kbdlang, TLMI_LANG_MAXLEN, "%s", value);

		//printk("passwd %s\n", think->password);
		//printk("encoding %s\n", think->password_encoding);
		//printk("kbd lang %s\n", think->password_kbdlang);

		update_auth_string(think);
		break;

#if 0 /*TO BE FIXED*/
	case THINKLMI_CHANGE_PASSWORD:
		if (copy_from_user(get_set_string, (void *)arg,
				   sizeof(get_set_string)))
			return -EFAULT;
		tmp_string = get_set_string;

                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_type, TLMI_PWDTYPE_MAXLEN, "%s",value);
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
		snprintf(think->password_encoding, TLMI_ENC_MAXLEN, "%s",value);
                value = strsep(&tmp_string, ",");
		if (!value)
			return -EFAULT;
		snprintf(think->password_kbdlang, TLMI_LANG_MAXLEN, "%s",value);

		//printk("passtype %s\n", think->password_type);
		//printk("old passwd %s\n", think->password);
		//printk("new passwd %s\n", newpassword);
		//printk("encoding %s\n", think->password_encoding);
		//printk("kbd lang %s\n", think->password_kbdlang);

		update_auth_string(think);

	        ret = think_lmi_set_bios_password(get_set_string);
		break;
#endif
	default:
		return -EINVAL;
	}

	return THINK_LMI_SUCCESS;

error:
	kfree(settings);
	kfree(choices);
	return ret ? ret : count;

}

//Lenovo linux utility team
//Character device release interface
static int think_lmi_chardev_release(struct inode *inode, struct file *file)
{
	return THINK_LMI_SUCCESS;
}

//Lenovo linux utility team
//Character device File operation structure and entrypoints

static const struct file_operations think_lmi_chardev_fops = {
	.open           = think_lmi_chardev_open,
	.unlocked_ioctl = think_lmi_chardev_ioctl,
	.release        = think_lmi_chardev_release,
};

static void show_bios_setting_line(struct think_lmi *think,
				   struct seq_file *m, int i, bool list_valid)
{
	int ret;
	char *settings = NULL, *choices = NULL, *p;

	ret = think_lmi_setting(i, &settings, LENOVO_BIOS_SETTING_GUID);
	if (ret || !settings)
		return;

	p = strchr(settings, ',');
	if (p)
		*p = '=';
	seq_printf(m, "%s", settings);

	if (!think->can_get_bios_selections)
		goto line_feed;

	if (p)
		*p = '\0';

	ret = think_lmi_get_bios_selections(settings, &choices);
	if (ret || !choices || !*choices)
		goto line_feed;

	seq_printf(m, "\t[%s]", choices);

line_feed:
	kfree(settings);
	if (choices)
		kfree(choices);
	seq_puts(m, "\n");
}

static void show_platform_setting_line(struct think_lmi *think,
				   struct seq_file *m, int i, bool list_valid)
{
	int ret;
	char *settings = NULL, *p;

	ret = think_lmi_setting(i, &settings, LENOVO_PLATFORM_SETTING_GUID);
	if (ret || !settings)
		return;

	p = strchr(settings, ',');
	if (p)
		*p = '=';
	seq_printf(m, "%s", settings);

	kfree(settings);
	seq_puts(m, "\n");
}

static int dbgfs_bios_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;
	int i;

	for (i = 0; i < think->settings_count; ++i)
		show_bios_setting_line(think, m, i, true);

	return 0;
}

static int dbgfs_platform_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;
	int i;

	for (i = 0; i < think->settings_count; ++i)
		show_platform_setting_line(think, m, i, true);

	return 0;
}

static int dbgfs_bios_setting(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	show_bios_setting_line(m->private, m, think->debug.instance, false);
	return 0;
}

static int dbgfs_list_valid_choices(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;
	char *choices = NULL;
	int ret;

	ret = think_lmi_get_bios_selections(think->debug.argument,
					       &choices);

	if (ret || !choices || !*choices) {
		if (choices)
			kfree(choices);
		return -EIO;
	}

	seq_printf(m, "%s\n", choices);
	kfree(choices);
	return 0;
}

static int dbgfs_set_bios_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_set_bios_settings(think->debug.argument);
}

static int dbgfs_set_platform_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_set_platform_settings(think->debug.argument);
}

static int dbgfs_save_bios_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_save_bios_settings(think->debug.argument);
}

static int dbgfs_discard_bios_settings(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_discard_bios_settings(think->debug.argument);
}

static int dbgfs_load_default(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_load_default(think->debug.argument);
}

#if 0 /*TO BE FIXED*/
static int dbgfs_set_bios_password(struct seq_file *m, void *data)
{
	struct think_lmi *think = m->private;

	return think_lmi_set_bios_password(think->debug.argument);
}
#endif

static int dbgfs_bios_password_settings(struct seq_file *m, void *data)
{
	struct think_lmi_pcfg pcfg;
	int ret;

	ret = think_lmi_password_settings(&pcfg);
	if (ret)
		return ret;
	seq_printf(m, "password_mode:       %#x\n", pcfg.password_mode);
	seq_printf(m, "password_state:      %#x\n", pcfg.password_state);
	seq_printf(m, "min_length:          %d\n", pcfg.min_length);
	seq_printf(m, "max_length:          %d\n", pcfg.max_length);
	seq_printf(m, "supported_encodings: %#x\n", pcfg.supported_encodings);
	seq_printf(m, "supported_keyboard:  %#x\n", pcfg.supported_keyboard);
	return 0;
}

static struct think_lmi_debugfs_node think_lmi_debug_files[] = {
	{ NULL, "bios_settings", dbgfs_bios_settings },
	{ NULL, "bios_setting", dbgfs_bios_setting },
	{ NULL, "list_valid_choices", dbgfs_list_valid_choices },
	{ NULL, "set_bios_settings", dbgfs_set_bios_settings },
	{ NULL, "save_bios_settings", dbgfs_save_bios_settings },
	{ NULL, "discard_bios_settings", dbgfs_discard_bios_settings },
	{ NULL, "load_default", dbgfs_load_default },
#if 0 /*TO BE FIXED*/
	{ NULL, "set_bios_password", dbgfs_set_bios_password },
#endif
	{ NULL, "bios_password_settings", dbgfs_bios_password_settings },
	{ NULL, "platform_settings", dbgfs_platform_settings },
	{ NULL, "set_platform_settings", dbgfs_set_platform_settings },
};

static int think_lmi_debugfs_open(struct inode *inode, struct file *file)
{
	struct think_lmi_debugfs_node *node = inode->i_private;

	return single_open(file, node->show, node->think);
}

static const struct file_operations think_lmi_debugfs_io_ops = {
	.owner = THIS_MODULE,
	.open  = think_lmi_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void think_lmi_debugfs_exit(struct think_lmi *think)
{
	debugfs_remove_recursive(think->debug.root);
}

static int think_lmi_debugfs_init(struct think_lmi *think)
{
	struct dentry *dent;
	int i;

	think->debug.instances_count = think->settings_count;

	think->debug.root = debugfs_create_dir(THINK_LMI_FILE, NULL);
	if (!think->debug.root) {
		pr_err("failed to create debugfs directory");
		goto error_debugfs;
	}

	dent = debugfs_create_file("argument", S_IRUGO | S_IWUSR,
				   think->debug.root, think,
				   &think_lmi_debugfs_argument_fops);
	if (!dent)
		goto error_debugfs;

	debugfs_create_u8("instance", 0644, think->debug.root,
				 &think->debug.instance);

	debugfs_create_u8("instances_count", 0444, think->debug.root,
				 &think->debug.instances_count);

	for (i = 0; i < ARRAY_SIZE(think_lmi_debug_files); i++) {
		struct think_lmi_debugfs_node *node;

		node = &think_lmi_debug_files[i];

		/* Filter non-present interfaces */
		if (!strcmp(node->name, "set_bios_settings") &&
		    !think->can_set_bios_settings)
			continue;
		if (!strcmp(node->name, "dicard_bios_settings") &&
		    !think->can_discard_bios_settings)
			continue;
		if (!strcmp(node->name, "load_default_settings") &&
		    !think->can_load_default_settings)
			continue;
		if (!strcmp(node->name, "get_bios_selections") &&
		    !think->can_get_bios_selections)
			continue;
		if (!strcmp(node->name, "set_bios_password") &&
		    !think->can_set_bios_password)
			continue;
		if (!strcmp(node->name, "bios_password_settings") &&
		    !think->can_get_password_settings)
			continue;

		node->think = think;
		dent = debugfs_create_file(node->name, S_IFREG | S_IRUGO,
					   think->debug.root, node,
					   &think_lmi_debugfs_io_ops);
		if (!dent) {
			pr_err("failed to create debug file: %s\n", node->name);
			goto error_debugfs;
		}
	}

	return 0;

error_debugfs:
	think_lmi_debugfs_exit(think);
	return -ENOMEM;
}

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
	dev_ret = device_create(tlmi_class, NULL, tlmi_dev, NULL, "thinklmi");
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

/* Base driver */
static void think_lmi_analyze(struct think_lmi *think)
{
	acpi_status status;
	int i = 0;

	/* Try to find the number of valid settings of this machine
	 * and use it to create sysfs attributes */
	for (i = 0; i < 0xFF; ++i) {
		char *item = NULL;
		int spleng = 0;
		int num = 0;
		char *p;

		status = think_lmi_setting(i, &item, LENOVO_BIOS_SETTING_GUID);
		if (ACPI_FAILURE(status))
			break;
		if (!item )
			break;
		if (!*item)
			continue;

		/* It is not allowed to have '/' for file name. Convert it into '\'. */
		spleng = strlen(item);
		for (num = 0; num < spleng; num++) {
			if (item[num] == '/') {
				item[num] = '\\';
			}
		}

		/* Remove the value part */
		p = strchr(item, ',');
		if (p)
			*p = '\0';
		think->settings[i] = item; /* Cache setting name */
		think->settings_count++;
	}

	pr_info("Found %d settings", think->settings_count);

	if (wmi_has_guid(LENOVO_SET_BIOS_SETTINGS_GUID) &&
	    wmi_has_guid(LENOVO_SAVE_BIOS_SETTINGS_GUID)) {
		think->can_set_bios_settings = true;
	}

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
	int err;

	think = kzalloc(sizeof(struct think_lmi), GFP_KERNEL);
	if (!think)
		return -ENOMEM;

	think->wmi_device = wdev;
	dev_set_drvdata(&wdev->dev, think);

	think_lmi_chardev_initialize(think);

	think_lmi_analyze(think);

	err = think_lmi_platform_init(think);
	if (err)
		goto error_platform;

	err = think_lmi_debugfs_init(think);
	if (err)
		goto error_debugfs;

	return 0;

error_debugfs:
	think_lmi_platform_exit(think);
error_platform:
	kfree(think);
	return err;
}

static int think_lmi_remove(struct wmi_device *wdev)
{
	struct think_lmi *think;
	int i;

	think = dev_get_drvdata(&wdev->dev);
	think_lmi_debugfs_exit(think);
	think_lmi_platform_exit(think);
	think_lmi_chardev_exit(think);

	for (i = 0; think->settings[i]; ++i) {
		kfree(think->settings[i]);
		think->settings[i] = NULL;
	}

	kfree(think);
	return 0;
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
	// Search for Lenovo_BiosSetting
	{ .guid_string = LENOVO_BIOS_SETTING_GUID },
	{ },
};

static struct wmi_driver think_lmi_driver = {
	.driver = {
		.name = "think-lmi",
	},
	.id_table = think_lmi_id_table,
	.probe = think_lmi_probe,
	.remove = think_lmi_remove,
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
