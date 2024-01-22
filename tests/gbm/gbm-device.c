/* Copyright (c) 2024 Collabora Ltd
 * SPDX-License-Identifier: MIT
 */

/**
 * \file
 * \brief Tests for libgbm.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <gbm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "piglit-util.h"

int
main(int argc, char **argv)
{
	int drm_fd;
	char *nodename;
	struct gbm_device *gbm;

	/* Strip common piglit args. */
	piglit_strip_arg(&argc, argv, "-fbo");
	piglit_strip_arg(&argc, argv, "-auto");

	nodename = getenv("WAFFLE_GBM_DEVICE");
	if (!nodename)
		nodename = "/dev/dri/renderD128";
	drm_fd = open(nodename, O_RDWR);
	if (drm_fd == -1) {
		perror("Error opening render node");
		piglit_report_result(PIGLIT_SKIP);
	}

	gbm = gbm_create_device(drm_fd);
	if (!gbm)
		piglit_report_result(PIGLIT_FAIL);

	if (gbm_device_get_fd(gbm) != drm_fd)
		piglit_report_result(PIGLIT_FAIL);

	gbm_device_destroy(gbm);
	close(drm_fd);

	piglit_report_result(PIGLIT_PASS);
}
