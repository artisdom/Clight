#include <systemd/sd-bus.h>
#include <libudev.h>

#include "../inc/camera.h"

static int method_setbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int method_getbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int method_getmaxbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int method_getactualbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static void get_first_matching_device(struct udev_device **dev, const char *subsystem);
#ifndef DISABLE_FRAME_CAPTURES
static int method_captureframes(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
#endif

static const char object_path[] = "/org/clight/backlight";
static const char bus_interface[] = "org.clight.backlight";
static struct udev *udev;
/**
 * Bus spec: https://dbus.freedesktop.org/doc/dbus-specification.html
 */
static const sd_bus_vtable calculator_vtable[] = {
    SD_BUS_VTABLE_START(0),
    // takes: backlight kernel interface, eg: "intel_backlight" and val to be written. Returns new val.
    SD_BUS_METHOD("setbrightness", "si", "i", method_setbrightness, SD_BUS_VTABLE_UNPRIVILEGED),
    // takes: backlight kernel interface, eg: "intel_backlight". Returns brightness val
    SD_BUS_METHOD("getbrightness", "s", "i", method_getbrightness, SD_BUS_VTABLE_UNPRIVILEGED),
    // takes: backlight kernel interface, eg: "intel_backlight". Returns max brightness val
    SD_BUS_METHOD("getmaxbrightness", "s", "i", method_getmaxbrightness, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("getactualbrightness", "s", "i", method_getactualbrightness, SD_BUS_VTABLE_UNPRIVILEGED),
#ifndef DISABLE_FRAME_CAPTURES
    // takes: video interface, eg: "/dev/video0" and number of frames to be taken. Returns frames average brightness.
    SD_BUS_METHOD("captureframes", "si", "d", method_captureframes, SD_BUS_VTABLE_UNPRIVILEGED),
#endif
    SD_BUS_VTABLE_END
};

/**
 * Brightness setter method
 */
static int method_setbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int value, r, max;
    struct udev_device *dev;
    const char *backlight_interface;
    
    /* Read the parameters */
    r = sd_bus_message_read(m, "si", &backlight_interface, &value);
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }
    
    /* Return an error if value is < 0 */
    if (value < 0) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_INVALID_ARGS, "Value must be greater or equal to 0.");
        return -EINVAL;
    }
    
    // if no backlight_interface is specified, try to get first matching device
    if (!strlen(backlight_interface)) {
        get_first_matching_device(&dev, "backlight");
    } else {
        dev = udev_device_new_from_subsystem_sysname(udev, "backlight", backlight_interface);
    }
    if (!dev) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_FILE_NOT_FOUND, "Device does not exist.");
        return -1;
    }
    
    /**
     * Check if value is <= max_brightness value
     */
    max = atoi(udev_device_get_sysattr_value(dev, "max_brightness"));
    if (value > max) {
        sd_bus_error_setf(ret_error, SD_BUS_ERROR_INVALID_ARGS, "Value must be smaller than %d.", max);
        return -EINVAL;
    }
    
    char val[10];
    sprintf(val, "%d", value);
    r = udev_device_set_sysattr_value(dev, "brightness", val);
    if (r < 0) {
        udev_device_unref(dev);
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_ACCESS_DENIED, "Not authorized.");
        return r;
    }
    
    printf("New brightness value for %s: %d\n", udev_device_get_sysname(dev), value);
    
    udev_device_unref(dev);
    /* Reply with the response */
    return sd_bus_reply_method_return(m, "i", value);
}

/**
 * Current brightness getter method
 */
static int method_getbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int x, r;
    struct udev_device *dev;
    const char *backlight_interface;
    
    /* Read the parameters */
    r = sd_bus_message_read(m, "s", &backlight_interface);
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }
    
    // if no backlight_interface is specified, try to get first matching device
    if (!strlen(backlight_interface)) {
        get_first_matching_device(&dev, "backlight");
    } else {
        dev = udev_device_new_from_subsystem_sysname(udev, "backlight", backlight_interface);
    }
    if (!dev) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_FILE_NOT_FOUND, "Device does not exist.");
        return -1;
    }
    
    x = atoi(udev_device_get_sysattr_value(dev, "brightness"));
    printf("Current brightness value for %s: %d\n", udev_device_get_sysname(dev), x);
    
    udev_device_unref(dev);
    
    /* Reply with the response */
    return sd_bus_reply_method_return(m, "i", x);
}

/**
 * Max brightness value getter method
 */
static int method_getmaxbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int x, r;
    struct udev_device *dev;
    const char *backlight_interface;
    
    /* Read the parameters */
    r = sd_bus_message_read(m, "s", &backlight_interface);
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }
    
    // if no backlight_interface is specified, try to get first matching device
    if (!strlen(backlight_interface)) {
        get_first_matching_device(&dev, "backlight");
    } else {
        dev = udev_device_new_from_subsystem_sysname(udev, "backlight", backlight_interface);
    }
    if (!dev) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_FILE_NOT_FOUND, "Device does not exist.");
        return -1;
    }
    
    x = atoi(udev_device_get_sysattr_value(dev, "max_brightness"));
    printf("Max brightness value for %s: %d\n", udev_device_get_sysname(dev), x);
    
    udev_device_unref(dev);
    
    /* Reply with the response */
    return sd_bus_reply_method_return(m, "i", x);
}

/**
 * Actual brightness value getter method
 */
static int method_getactualbrightness(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int x, r;
    struct udev_device *dev;
    const char *backlight_interface;
    
    /* Read the parameters */
    r = sd_bus_message_read(m, "s", &backlight_interface);
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }
    
    // if no backlight_interface is specified, try to get first matching device
    if (!strlen(backlight_interface)) {
        get_first_matching_device(&dev, "backlight");
    } else {
        dev = udev_device_new_from_subsystem_sysname(udev, "backlight", backlight_interface);
    }
    if (!dev) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_FILE_NOT_FOUND, "Device does not exist.");
        return -1;
    }
    
    x = atoi(udev_device_get_sysattr_value(dev, "actual_brightness"));
    printf("Actual brightness value for %s: %d\n", udev_device_get_sysname(dev), x);
    
    udev_device_unref(dev);
    
    /* Reply with the response */
    return sd_bus_reply_method_return(m, "i", x);
}

/**
 * Set dev to first device in subsystem
 */
static void get_first_matching_device(struct udev_device **dev, const char *subsystem) {
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, subsystem);
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        *dev = udev_device_new_from_syspath(udev, path);
        break;
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);
}

#ifndef DISABLE_FRAME_CAPTURES
/**
 * Frames capturing method
 */
static int method_captureframes(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int num_frames, r;
    struct udev_device *dev;
    const char *video_interface;
    double val;
    
    /* Read the parameters */
    r = sd_bus_message_read(m, "si", &video_interface, &num_frames);
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }
    
    // if no video device is specified, try to get first matching device
    if (!strlen(video_interface)) {
        get_first_matching_device(&dev, "video4linux");
    } else {
        dev = udev_device_new_from_subsystem_sysname(udev, "video4linux", video_interface);
    }
    if (!dev) {
        sd_bus_error_set_const(ret_error, SD_BUS_ERROR_FILE_NOT_FOUND, "Device does not exist.");
        return -1;
    }
    
    val = capture_frames(udev_device_get_devnode(dev), num_frames);
    
    printf("Frames captured by %s average brightness value: %lf\n", udev_device_get_sysname(dev), val);
    
    udev_device_unref(dev);
    
    /* Reply with the response */
    return sd_bus_reply_method_return(m, "d", val);
}
#endif

int main(int argc, char *argv[]) {
    sd_bus_slot *slot = NULL;
    sd_bus *bus = NULL;
    int r;
    
    udev = udev_new();
    
    /* Connect to the system bus */
    r = sd_bus_default_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        goto finish;
    }
    
    /* Install the object */
    r = sd_bus_add_object_vtable(bus,
                                 &slot,
                                 object_path,
                                 bus_interface,
                                 calculator_vtable,
                                 NULL);
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
        goto finish;
    }
    
    /* Take a well-known service name so that clients can find us */
    r = sd_bus_request_name(bus, bus_interface, 0);
    if (r < 0) {
        fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
        goto finish;
    }
    
    for (;;) {
        /* Process requests */
        r = sd_bus_process(bus, NULL);
        if (r < 0) {
            fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
            goto finish;
        }
        
        /* we processed a request, try to process another one, right-away */
        if (r > 0) { 
            continue;
        }
        
        /* Wait for the next request to process */
        r = sd_bus_wait(bus, (uint64_t) -1);
        if (r < 0) {
            fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
            goto finish;
        }
    }
    
finish:
    if (slot) {
        sd_bus_slot_unref(slot);
    }
    if (bus) {
        sd_bus_unref(bus);
    }
    udev_unref(udev);
    
    return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}