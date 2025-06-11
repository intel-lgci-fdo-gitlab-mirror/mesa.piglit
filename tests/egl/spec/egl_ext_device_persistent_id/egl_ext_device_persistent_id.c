/*
 * Copyright © 2016 Red Hat, Inc.
 * Copyright 2015-2025 Intel Corporation
 * Copyright 2018 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

 /*
 * Test that enumerates all EGL devices and attempts to access the attributes
 * provided by EGL_EXT_device_persistent_id. For more details, please refer to:
 * https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_persistent_id.txt
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "piglit-util.h"
#include "piglit-util-egl.h"

#define NDEVS 1024
#define MAX_UUID_SIZE 16

/* Converts a hex value to an ASCII char. */
static char
hex_to_char(unsigned char hexval)
{
	if (hexval < 10) {
		return '0' + hexval;
	} else if (hexval < 16) {
		return 'a' + (hexval - 10);
	} else {
		printf("hex_to_char failed\n");
		piglit_report_result(PIGLIT_FAIL);
		return 'X';
	}
}

/* Converts a byte value to a hex string. */
static void
byte_to_string(unsigned char byteval, char* hexstr)
{
	unsigned char hexval;
	for (int i = 0; i < 2; i++) {
		hexval = byteval;
		if (i == 0) {
			hexval = hexval >> 4;
		}
		hexval = hexval % 16;
		hexstr[i] = hex_to_char(hexval);
	}
}

/* Converts a 16 byte UUID to a hex string with dashes. */
static void
uuid_to_string(const unsigned char* uuid, char* uuidstr)
{
	for (int i = 0; i < MAX_UUID_SIZE; i++) {
		byte_to_string(uuid[i], uuidstr);
		uuidstr += 2;
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			*uuidstr = '-';
			uuidstr += 1;
		}
	}
}

#ifndef EGL_EXT_device_persistent_id
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEVICEBINARYEXTPROC) (EGLDeviceEXT device, EGLint name, EGLint max_size, void *value, EGLint *size);
#endif

int
main(void)
{
	enum piglit_result result = PIGLIT_PASS;
	EGLDeviceEXT devs[NDEVS];
	EGLint i, numdevs, num_compatible_devs = 0;
	EGLDeviceEXT device = EGL_NO_DEVICE_EXT;
	const char *devstring = NULL;
	EGLBoolean retval;
	EGLint uuidsize;
	unsigned char uuid[MAX_UUID_SIZE];
	/* uuidstr uses hex encoding with four dashes. */
	char uuidstr[MAX_UUID_SIZE * 2 + 5];
	PFNEGLQUERYDEVICESEXTPROC queryDevices;
	PFNEGLQUERYDEVICESTRINGEXTPROC queryDeviceString;
	PFNEGLQUERYDEVICEBINARYEXTPROC queryDeviceBinary;

	memset(uuidstr, 0, MAX_UUID_SIZE * 2 + 5);

	const char *client_exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	bool has_client_ext =
		client_exts &&
		((piglit_is_extension_in_string(client_exts,
			"EGL_EXT_device_query") &&
		  piglit_is_extension_in_string(client_exts,
			"EGL_EXT_device_enumeration")) ||
		 piglit_is_extension_in_string(client_exts,
			"EGL_EXT_device_base"));

	if (!has_client_ext) {
		printf("EGL_EXT_device_query not supported\n");
		piglit_report_result(PIGLIT_SKIP);
	}

	queryDevices = (void *)eglGetProcAddress("eglQueryDevicesEXT");

	queryDeviceString =
		(void *)eglGetProcAddress("eglQueryDeviceStringEXT");
	queryDeviceBinary =
		(void *)eglGetProcAddress("eglQueryDeviceBinaryEXT");

	if (!queryDevices || !queryDeviceString || !queryDeviceBinary) {
		printf("No device query/enumeration entrypoints\n");
		piglit_report_result(PIGLIT_SKIP);
	}

	if (!queryDevices(0, NULL, &numdevs)) {
		printf("Failed to get device count\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	if (numdevs > NDEVS) {
		printf("More than %d devices, please fix this test\n", NDEVS);
		result = PIGLIT_WARN;
		numdevs = NDEVS;
	}

	memset(devs, 0, sizeof devs);
	if (!queryDevices(numdevs, devs, &numdevs)) {
		printf("Failed to enumerate devices\n");
		piglit_report_result(PIGLIT_FAIL);
	}
	if (!numdevs) {
		printf("Zero devices enumerated\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	for (i = 0; i < numdevs; i++) {
		printf("------------------------------------------------\n");
		printf("Device #%d\n", i);
		printf("------------------------------------------------\n");

		device = devs[i];
		devstring = queryDeviceString(device, EGL_EXTENSIONS);
		if (devstring == NULL) {
			printf("Empty device extension string\n");
			result = PIGLIT_WARN;
			continue;
		}

		if (!piglit_is_extension_in_string(devstring,
					"EGL_EXT_device_persistent_id")) {
			printf("Device does not support EGL_EXT_device_persistent_id\n");
			continue;
		}
		num_compatible_devs++;

#ifndef EGL_EXT_device_persistent_id
#define EGL_DEVICE_UUID_EXT               0x335C
#define EGL_DRIVER_UUID_EXT               0x335D
#define EGL_DRIVER_NAME_EXT               0x335E
#endif
		devstring = queryDeviceString(device, EGL_DRIVER_NAME_EXT);
		if (devstring == NULL || strlen(devstring) == 0)
			piglit_report_result(PIGLIT_FAIL);
		printf("EGL Driver Name: %s\n", devstring);

		uuidsize = 0;
		retval = queryDeviceBinary(device, EGL_DEVICE_UUID_EXT, MAX_UUID_SIZE, NULL, &uuidsize);
		if (retval != EGL_TRUE || uuidsize != MAX_UUID_SIZE)
			piglit_report_result(PIGLIT_FAIL);
		uuidsize = 0;
		retval = queryDeviceBinary(device, EGL_DEVICE_UUID_EXT, MAX_UUID_SIZE, uuid, &uuidsize);
		/* uuidsize is now the number of bytes that were actually written. */
		if (retval != EGL_TRUE || uuidsize != MAX_UUID_SIZE)
			piglit_report_result(PIGLIT_FAIL);
		uuid_to_string(uuid, uuidstr);
		printf("EGL Device UUID: %s\n", uuidstr);

		uuidsize = 0;
		retval = queryDeviceBinary(device, EGL_DRIVER_UUID_EXT, MAX_UUID_SIZE, NULL, &uuidsize);
		if (retval != EGL_TRUE || uuidsize != MAX_UUID_SIZE)
			piglit_report_result(PIGLIT_FAIL);
		uuidsize = 0;
		retval = queryDeviceBinary(device, EGL_DRIVER_UUID_EXT, MAX_UUID_SIZE, uuid, &uuidsize);
		/* uuidsize is now the number of bytes that were actually written. */
		if (retval != EGL_TRUE || uuidsize != MAX_UUID_SIZE)
			piglit_report_result(PIGLIT_FAIL);
		uuid_to_string(uuid, uuidstr);
		printf("EGL Driver UUID: %s\n", uuidstr);
	}

	/* SKIP if we fetched all devices with none supporting the extension */
	if (result == PIGLIT_PASS && !num_compatible_devs)
		result = PIGLIT_SKIP;

	piglit_report_result(result);
}
