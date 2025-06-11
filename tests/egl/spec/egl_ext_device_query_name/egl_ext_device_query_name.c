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
 * provided by EGL_EXT_device_query_name. For more details, please refer to:
 * https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_query_name.txt
 */

 #include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "piglit-util.h"
#include "piglit-util-egl.h"

#define NDEVS 1024

int
main(void)
{
	enum piglit_result result = PIGLIT_PASS;
	EGLDeviceEXT devs[NDEVS];
	EGLint i, numdevs, num_compatible_devs = 0;
	EGLDeviceEXT device = EGL_NO_DEVICE_EXT;
	const char *devstring = NULL;
	PFNEGLQUERYDEVICESEXTPROC queryDevices;
	PFNEGLQUERYDEVICESTRINGEXTPROC queryDeviceString;

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

	if (!queryDevices || !queryDeviceString) {
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
					"EGL_EXT_device_query_name")) {
			printf("Device does not support EGL_EXT_device_query_name\n");
			continue;
		}
		num_compatible_devs++;

#ifndef EGL_EXT_device_query_name
#define EGL_RENDERER_EXT                  0x335F
#endif
		devstring = queryDeviceString(device, EGL_RENDERER_EXT);
		if (devstring == NULL || strlen(devstring) == 0)
			piglit_report_result(PIGLIT_FAIL);
		printf("EGL Renderer: %s\n", devstring);

		devstring = queryDeviceString(device, EGL_VENDOR);
		if (devstring == NULL || strlen(devstring) == 0)
			piglit_report_result(PIGLIT_FAIL);
		printf("EGL Vendor: %s\n", devstring);
	}

	/* SKIP if we fetched all devices with none supporting the extension */
	if (result == PIGLIT_PASS && !num_compatible_devs)
		result = PIGLIT_SKIP;

	piglit_report_result(result);
}
