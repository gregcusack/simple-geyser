#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "base_58.h"

#define BASE58_ENCODED_LENGTH 57 // 56 characters + null terminator

struct FfiVersion {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t commit;
    uint32_t feature_set;
    uint16_t client;
};

// Define function pointer types for the Geyser plugin interface
typedef const char* (*NameFn)(void);
typedef int32_t (*OnLoadFn)(const char* config_file, int is_reload);
typedef void (*OnUnloadFn)(void);
typedef int32_t (*NodeUpdateNotificationsEnabledFn)(void);
typedef int32_t (*NotifyNodeUpdateFn)(const void* interface);

// Pubkey
typedef const uint8_t* Key;

// ContactInfoPtr as a pointer to void
typedef const void* ContactInfoPtr;

// Define the function pointer type for get_pubkey_fn
typedef Key (*ContactInfoGetKey)(ContactInfoPtr contact_info_ptr);
typedef uint64_t (*ContactInfoGetWallclockFn)(ContactInfoPtr contact_info_ptr);
typedef uint64_t (*ContactInfoGetShredVersion)(ContactInfoPtr contact_info_ptr);
typedef bool (*ContactInfoGetVersionFn)(ContactInfoPtr contact_info_ptr, struct FfiVersion* ffi_version);

// Define the FfiContactInfoInterface struct
struct FfiContactInfoInterface {
    ContactInfoPtr contact_info_ptr;
    ContactInfoGetKey get_pubkey_fn;
    ContactInfoGetWallclockFn get_wallclock_fn;
    ContactInfoGetShredVersion get_shred_version_fn;
    ContactInfoGetVersionFn get_version_fn;
    // // Socket address getter functions
    // pub get_gossip_fn: ContactInfoGetGossipFn,
    // pub get_rpc_fn: ContactInfoGetRpcFn,
    // pub get_rpc_pubsub_fn: ContactInfoGetRpcPubsubpFn,
    // pub get_serve_repair_fn: ContactInfoGetServeRepairFn,
    // pub get_tpu_fn: ContactInfoGetTpuFn,
    // pub get_tpu_forwards_fn: ContactInfoGetTpuForwardsFn,
    // pub get_tpu_vote_fn: ContactInfoGetTpuVoteFn,
    // pub get_tvu_fn: ContactInfoGetTvuFn,
    // ... other function pointers ...
};

// FfiGeyserPlugin struct w/ function pointers
struct FfiGeyserPlugin {
    NameFn name;
    OnLoadFn on_load;
    OnUnloadFn on_unload;
    NodeUpdateNotificationsEnabledFn node_update_notifications_enabled;
    NotifyNodeUpdateFn notify_node_update;
};

const char* name(void) {
    return "simple_geyser_in_c";
}

int32_t on_load(const char* config_file, int is_reload) {
    printf("greg: Loading plugin with config file: %s\n", config_file);
    return 0; // Return 0 for success
}

void on_unload(void) {
    printf("greg: Unloading plugin.\n");
}

// Enable node update notifications
int node_update_notifications_enabled(void) {
    return 1; // Return non-zero to indicate enabled
}

int notify_node_update(const void* interface) {
    const struct FfiContactInfoInterface* info = (const struct FfiContactInfoInterface*)interface;
    
    if (info == NULL 
        || info->get_pubkey_fn == NULL 
        || info->get_wallclock_fn == NULL
        || info->get_shred_version_fn == NULL
        || info->get_version_fn == NULL
    ) {
        fprintf(stderr, "greg: Invalid interface or function pointers are NULL\n");
        return -1; // Return error code
    }

    // Use get_pubkey_fn to get the pubkey
    Key pubkey = info->get_pubkey_fn(info->contact_info_ptr);
    if (pubkey == NULL) {
        fprintf(stderr, "greg: pubkey is NULL\n");
        return -1; // Return error code
    }

    uint64_t wallclock = info->get_wallclock_fn(info->contact_info_ptr);
    uint64_t shred_version = info->get_shred_version_fn(info->contact_info_ptr);

    // Use get_version_fn to get the version information
    struct FfiVersion version;
    bool success = info->get_version_fn(info->contact_info_ptr, &version);
    if (!success) {
        fprintf(stderr, "greg: Failed to get version information\n");
        return -1;
    }

    char base58_pubkey[BASE58_ENCODED_LENGTH];
    if (encode_base58(pubkey, 32, base58_pubkey, sizeof(base58_pubkey))) {
        fprintf(stderr, "greg: Base58 encoding failed\n");
        return -1;
    }

    // Print the Base58-encoded pubkey
    fprintf(stderr, "greg: pk: %s, wc: %" PRIu64 ", sv: %" PRIu64 ", version: v%u.%u.%u\n", base58_pubkey, wallclock, shred_version, version.major, version.minor, version.patch);
    fflush(stderr); 

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