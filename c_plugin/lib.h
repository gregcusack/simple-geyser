#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <arpa/inet.h>

typedef enum {
    UDP = 0,
    QUIC = 1
} FfiProtocol;

struct FfiVersion {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t commit;
    uint32_t feature_set;
    uint16_t client;
};

struct FfiIpAddr {
    uint8_t is_v4;     // 1 if IPv4, 0 if IPv6
    uint8_t addr[16]; // IP address bytes
};

struct FfiSocketAddr {
    struct FfiIpAddr ip;
    uint16_t port;
};


// Function pointer types for the Geyser plugin interface
typedef const char* (*NameFn)(void);
typedef int32_t (*OnLoadFn)(const char* config_file, int is_reload);
typedef void (*OnUnloadFn)(void);
typedef int32_t (*NodeUpdateNotificationsEnabledFn)(void);
typedef int32_t (*NotifyNodeUpdateFn)(const void* interface);

// Pubkey
typedef const uint8_t* Key;

// ContactInfoPtr
typedef const void* ContactInfoPtr;

// Function pointer type for each function
typedef Key (*ContactInfoGetKey)(ContactInfoPtr contact_info_ptr);
typedef uint64_t (*ContactInfoGetWallclockFn)(ContactInfoPtr contact_info_ptr);
typedef uint64_t (*ContactInfoGetShredVersion)(ContactInfoPtr contact_info_ptr);
typedef bool (*ContactInfoGetVersionFn)(ContactInfoPtr contact_info_ptr, struct FfiVersion* ffi_version);

typedef bool (*ContactInfoGetGossipFn)(ContactInfoPtr contact_info_ptr, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetRpcFn)(ContactInfoPtr contact_info_ptr, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetRpcPubsubpFn)(ContactInfoPtr contact_info_ptr, struct FfiSocketAddr* socket);

typedef bool (*ContactInfoGetServeRepairFn)(ContactInfoPtr contact_info_ptr, FfiProtocol protocol, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetTpuFn)(ContactInfoPtr contact_info_ptr, FfiProtocol protocol, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetTpuForwardsFn)(ContactInfoPtr contact_info_ptr, FfiProtocol protocol, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetTpuVoteFn)(ContactInfoPtr contact_info_ptr, FfiProtocol protocol, struct FfiSocketAddr* socket);
typedef bool (*ContactInfoGetTvuFn)(ContactInfoPtr contact_info_ptr, FfiProtocol protocol, struct FfiSocketAddr* socket);

struct FfiContactInfoInterface {
    ContactInfoPtr contact_info_ptr;
    ContactInfoGetKey get_pubkey_fn;
    ContactInfoGetWallclockFn get_wallclock_fn;
    ContactInfoGetShredVersion get_shred_version_fn;
    ContactInfoGetVersionFn get_version_fn;
    // Socket address getter functions
    ContactInfoGetGossipFn get_gossip_fn;
    ContactInfoGetRpcFn get_rpc_fn;
    ContactInfoGetRpcPubsubpFn get_rpc_pubsub_fn;
    ContactInfoGetServeRepairFn get_serve_repair_fn;
    ContactInfoGetTpuFn get_tpu_fn;
    ContactInfoGetTpuForwardsFn get_tpu_forwards_fn;
    ContactInfoGetTpuVoteFn get_tpu_vote_fn;
    ContactInfoGetTvuFn get_tvu_fn;
};

// FfiGeyserPlugin struct w/ function pointers
struct FfiGeyserPlugin {
    NameFn name;
    OnLoadFn on_load;
    OnUnloadFn on_unload;
    NodeUpdateNotificationsEnabledFn node_update_notifications_enabled;
    NotifyNodeUpdateFn notify_node_update;
};

// User must free the returned memory
char* format_socket_addr(const struct FfiSocketAddr *addr) {
    char ip_str[INET6_ADDRSTRLEN];
    char *result = (char *)malloc(INET6_ADDRSTRLEN + 8);
    
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }

    if (addr->ip.is_v4) {
        inet_ntop(AF_INET, addr->ip.addr, ip_str, sizeof(ip_str));
        snprintf(result, INET6_ADDRSTRLEN + 8, "%s:%d", ip_str, addr->port);
    } else {
        inet_ntop(AF_INET6, addr->ip.addr, ip_str, sizeof(ip_str));
        snprintf(result, INET6_ADDRSTRLEN + 8, "[%s]:%d", ip_str, addr->port);
    }

    return result; // Caller must free
}