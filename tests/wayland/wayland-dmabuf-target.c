/*
 * Copyright 2021 Eric Engestrom
 * Copyright 2023 Collabora, Ltd.
 * SPDX-License-Identifier: MIT
  */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#include <xf86drm.h>
#include <wayland-client.h>
#include "linux-dmabuf-unstable-v1-client-protocol.h"

#include "piglit-util.h"


#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct wayland_display_target {
    unsigned int roundtrips_needed;
    struct wl_display *dpy;
    struct wl_registry *registry;
    struct zwp_linux_dmabuf_v1 *dmabuf;
    struct zwp_linux_dmabuf_feedback_v1 *feedback;
    bool feedback_done;
    char *primary_driver_name;
};


static int
open_drm_by_devid(dev_t devid)
{
    drmDevice *device;
    int ret = -1;
    int err;

    err = drmGetDeviceFromDevId(devid, 0, &device);
    if (err != 0) {
        printf("libdrm reports no devices for our devid\n");
        piglit_report_result(PIGLIT_FAIL);
    }

    if (device->available_nodes & (1 << DRM_NODE_RENDER))
        ret = open(device->nodes[DRM_NODE_RENDER], O_RDWR);
    if (ret == -1 && device->available_nodes & (1 << DRM_NODE_PRIMARY))
        ret = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
    if (ret == -1) {
        printf("Couldn't open any libdrm devices for our devid\n");
        piglit_report_result(PIGLIT_FAIL);
    }

    drmFreeDevice(&device);

    return ret;
}


static void
feedback_handle_done(void *_data, struct zwp_linux_dmabuf_feedback_v1 *feedback)
{
    struct wayland_display_target *data = _data;

    /* We've got all our feedback events now, so we can remove the roundtrip we
     * added when binding the interface */
    data->feedback_done = true;
}

static void
feedback_handle_format_table(void *_data,
                             struct zwp_linux_dmabuf_feedback_v1 *feedback,
                             int fd, uint32_t size)
{
    /* We don't need the format table for anything */
    close(fd);
}

static void
feedback_handle_main_device(void *_data,
                            struct zwp_linux_dmabuf_feedback_v1 *feedback,
                            struct wl_array *dev_array)
{
    struct wayland_display_target *data = _data;
    drmVersionPtr version;
    dev_t dev;
    int fd;

    /* This is basically a malformed compositor */
    if (dev_array->size != sizeof(dev)) {
        printf("Expected main_device size to be %zu (dev_t), but it was %zu\n",
               sizeof(dev), dev_array->size);
        piglit_report_result(PIGLIT_FAIL);
    }

    memcpy(&dev, dev_array->data, sizeof(dev));
    fd = open_drm_by_devid(dev);
    if (fd < 0) {
        printf("Couldn't open DRM device for main_device\n");
        piglit_report_result(PIGLIT_FAIL);
    }

    version = drmGetVersion(fd);
    if (!version || !version->name_len || !version->name) {
        printf("drmGetVersion failed\n");
        piglit_report_result(PIGLIT_FAIL);
    }

    data->primary_driver_name = malloc(version->name_len + 1);
    assert(data->primary_driver_name);
    memcpy(data->primary_driver_name, version->name, version->name_len);
    data->primary_driver_name[version->name_len] = '\0';

    drmFreeVersion(version);
    close(fd);
}

static void
feedback_handle_tranche_done(void *_data,
                             struct zwp_linux_dmabuf_feedback_v1 *feedback)
{
    /* We don't care about the content of the format/modifier tranches */
}

static void
feedback_handle_tranche_target_device(void *_data,
                                      struct zwp_linux_dmabuf_feedback_v1 *feedback,
                                      struct wl_array *dev_arr)
{
    /* We don't care about per-tranche target devices (e.g. scanout) */
}

static void
feedback_handle_tranche_formats(void *_data,
                                struct zwp_linux_dmabuf_feedback_v1 *feedback,
                                struct wl_array *indices)
{
    /* We don't care about per-tranche formats */
}

static void
feedback_handle_tranche_flags(void *_data,
                              struct zwp_linux_dmabuf_feedback_v1 *feedback,
                              uint32_t flags)
{
    /* We don't care about per-tranche flags */
}

static const struct zwp_linux_dmabuf_feedback_v1_listener feedback_listener = {
    feedback_handle_done,
    feedback_handle_format_table,
    feedback_handle_main_device,
    feedback_handle_tranche_done,
    feedback_handle_tranche_target_device,
    feedback_handle_tranche_formats,
    feedback_handle_tranche_flags,
};

static void
registry_handle_global(void *_data, struct wl_registry *registry,
                       uint32_t name, const char *interface, uint32_t version)
{
    struct wayland_display_target *data = _data;

    if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0 && version >= 4) {
        data->dmabuf = wl_registry_bind(data->registry, name,
                                        &zwp_linux_dmabuf_v1_interface,
                                        MIN(version, 4));
        data->feedback = zwp_linux_dmabuf_v1_get_default_feedback(data->dmabuf);
        zwp_linux_dmabuf_feedback_v1_add_listener(data->feedback,
                                                  &feedback_listener,
                                                  data);
        /* Need another roundtrip to collect the feedback events */
        data->roundtrips_needed++;
    }
}

static void
registry_handle_remove(void *_data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_remove
};


int
main(void)
{
    const char *expected_driver_name = NULL;
    struct wayland_display_target data;

    memset(&data, 0, sizeof(data));

    expected_driver_name = getenv("PIGLIT_WAYLAND_EXPECTED_DRIVER");
    if (!expected_driver_name) {
        printf("$PIGLIT_WAYLAND_EXPECTED_DRIVER must be set to run this test\n");
        piglit_report_result(PIGLIT_SKIP);
    }

    /* Connect to $WAYLAND_DISPLAY or $WAYLAND_SOCKET */
    data.dpy = wl_display_connect(NULL);
    if (!data.dpy) {
        printf("Could not connect to Wayland display\n");
        piglit_report_result(PIGLIT_SKIP);
    }

    /* The registry advertises the available interfaces */
    data.registry = wl_display_get_registry(data.dpy);
    assert(data.registry);
    wl_registry_add_listener(data.registry, &registry_listener, &data);

    /* Listen for the wl_registry advertisements to get supported interfaces */
    wl_display_roundtrip(data.dpy);

    if (!data.dmabuf) {
        printf("zwp_linux_dmabuf_v1 is not available\n");
        piglit_report_result(PIGLIT_SKIP);
    }

    /* Wait until we receive the zwp_linux_dmabuf_feedback_v1.done event */
    while (!data.feedback_done)
        wl_display_roundtrip(data.dpy);

    if (!data.primary_driver_name ||
        strcmp(data.primary_driver_name, expected_driver_name) != 0) {
        printf("Got driver name %s, wanted %s\n", data.primary_driver_name,
               expected_driver_name);
        piglit_report_result(PIGLIT_FAIL);
    }

    free(data.primary_driver_name);
    zwp_linux_dmabuf_feedback_v1_destroy(data.feedback);
    zwp_linux_dmabuf_v1_destroy(data.dmabuf);
    wl_registry_destroy(data.registry);
    wl_display_disconnect(data.dpy);

    piglit_report_result(PIGLIT_PASS);
}
