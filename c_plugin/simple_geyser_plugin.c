/* 
* Geyser Plugin Written in C
* Streams ContactInfo updates
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "contact_info.h"

const char* name(void) {
    return "simple_geyser_in_c";
}

int32_t on_load(const char* config_file, int is_reload) {
    printf("Loading plugin with config file: %s\n", config_file);
    return 0; // Return 0 for success
}

void on_unload(void) {
    printf("Unloading plugin.\n");
}

// Enable node update notifications
int node_update_notifications_enabled(void) {
    return 1; // Return non-zero to indicate enabled
}

int notify_node_update(const void* interface) {
    const FfiContactInfoInterface* info = (const FfiContactInfoInterface*)interface;
    struct ContactInfo_C* contact_info_c = get_contact_info(info);
    if (!contact_info_c) {
        return -1;
    }

    print_contact_info(contact_info_c);
    free_contact_info(contact_info_c);

    return 0; 
}

// Implement _create_geyser_plugin_c function
__attribute__((visibility("default"))) struct FfiGeyserPlugin* _create_geyser_plugin_c(void) {
    struct FfiGeyserPlugin* plugin = (struct FfiGeyserPlugin*)malloc(sizeof(struct FfiGeyserPlugin));
    plugin->name = name;
    plugin->on_load = on_load;
    plugin->on_unload = on_unload;
    plugin->node_update_notifications_enabled = node_update_notifications_enabled;
    plugin->notify_node_update = notify_node_update;
    return plugin;
}