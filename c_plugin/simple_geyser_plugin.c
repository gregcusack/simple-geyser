#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Define function pointer types for the Geyser plugin interface
typedef const char* (*NameFn)(void);
typedef int32_t (*OnLoadFn)(const char* config_file, int is_reload);
typedef void (*OnUnloadFn)(void);
typedef int32_t (*NodeUpdateNotificationsEnabledFn)(void);
typedef int32_t (*NotifyNodeUpdateFn)(const void* interface);

// Define the FfiGeyserPlugin struct with all necessary function pointers
struct FfiGeyserPlugin {
    NameFn name;
    OnLoadFn on_load;
    OnUnloadFn on_unload;
    NodeUpdateNotificationsEnabledFn node_update_notifications_enabled;
    NotifyNodeUpdateFn notify_node_update;
};

// Return the plugin's name
const char* name(void) {
    return "simple_geyser_in_c";
}

// Load the plugin with the specified configuration file
int32_t on_load(const char* config_file, int is_reload) {
    printf("greg: Loading plugin with config file: %s\n", config_file);
    return 0; // Return 0 for success
}

// Unload the plugin
void on_unload(void) {
    printf("greg: Unloading plugin.\n");
}

// Enable node update notifications
int node_update_notifications_enabled(void) {
    return 1; // Return non-zero to indicate enabled
}

// Notify of a node update
int notify_node_update(const void* interface) {
    // Implement logic to handle the node update
    printf("greg: Node update notified.\n");
    const struct FfiContactInfoInterface* info = (const struct FfiContactInfoInterface*)interface;

    return 0; // Return 0 for success
}

// Implement _create_plugin function
__attribute__((visibility("default"))) struct FfiGeyserPlugin* _create_geyser_plugin_c(void) {
    struct FfiGeyserPlugin* plugin = (struct FfiGeyserPlugin*)malloc(sizeof(struct FfiGeyserPlugin));
    plugin->name = name;
    plugin->on_load = on_load;
    plugin->on_unload = on_unload;
    plugin->node_update_notifications_enabled = node_update_notifications_enabled;
    plugin->notify_node_update = notify_node_update;
    return plugin;
}