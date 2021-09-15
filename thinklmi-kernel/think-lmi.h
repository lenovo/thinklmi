/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _THINK_LMI_H_
#define _THINK_LMI_H_

#include <linux/ioctl.h>

#define TLMI_SETTINGS_MAXLEN 512
#define TLMI_PWD_MAXLEN       64
#define TLMI_PWDTYPE_MAXLEN   64
#define TLMI_TPMTYPE_MAXLEN   64
#define TLMI_ENC_MAXLEN       64
#define TLMI_LANG_MAXLEN       4
#define TLMI_MAX_SETTINGS    255
/*
 * Longest string should be in the set command: allow size of BIOS
 * option and choice
 */
#define TLMI_GETSET_MAXLEN (TLMI_SETTINGS_MAXLEN + TLMI_SETTINGS_MAXLEN)

#define THINKLMI_GET_SETTINGS        _IOR('T', 1, int *)
#define THINKLMI_GET_SETTINGS_STRING _IOWR('T', 2, char *)
#define THINKLMI_SET_SETTING         _IOW('T', 3, char *)
#define THINKLMI_SHOW_SETTING        _IOWR('T', 4, char *)
#define THINKLMI_AUTHENTICATE        _IOW('T', 5, char *)
#define THINKLMI_CHANGE_PASSWORD     _IOW('T', 6, char *)
#define THINKLMI_DEBUG               _IOW('T', 7, char *)
#define THINKLMI_LMIOPCODE           _IOW('T', 8, char *)
#define THINKLMI_LMIOPCODE_NOPAP     _IOW('T', 9, char *)
#define THINKLMI_TPMTYPE             _IOW('T', 10, char *)
#define THINKLMI_LOAD_DEFAULT        _IOW('T', 11, char *)
#define THINKLMI_SAVE_SETTINGS       _IOW('T', 12, char *)
#define THINKLMI_DISCARD_SETTINGS    _IOW('T', 13, char *)

#endif /* !_THINK_LMI_H_ */

