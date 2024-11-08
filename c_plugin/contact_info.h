#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lib.h"
#include "base_58.h"

#define BASE58_ENCODED_LENGTH 57  // Max length for Base58-encoded 32-byte key + null

struct ContactInfo_C {
    char base58_pubkey[BASE58_ENCODED_LENGTH];
    uint64_t wallclock;
    uint64_t shred_version;
    struct FfiVersion version;

    char *gossip_addr;
    char *rpc_addr;
    char *rpc_pubsub_addr;
    char *serve_repair_addr;
    char *tpu_udp_addr;
    char *tpu_quic_addr;
    char *tpu_forwards_udp_addr;
    char *tpu_forwards_quic_addr;
    char *tpu_vote_udp_addr;
    char *tpu_vote_quic_addr;
    char *tvu_udp_addr;
    char *tvu_quic_addr;
};

struct ContactInfo_C* get_contact_info(const struct FfiContactInfoInterface* info) {
    if (info == NULL 
        || info->get_pubkey_fn == NULL 
        || info->get_wallclock_fn == NULL
        || info->get_shred_version_fn == NULL
        || info->get_version_fn == NULL
        || info->get_gossip_fn == NULL
        || info->get_rpc_fn == NULL
        || info->get_rpc_pubsub_fn == NULL
        || info->get_serve_repair_fn == NULL
        || info->get_tpu_fn == NULL
        || info->get_tpu_forwards_fn == NULL
        || info->get_tpu_vote_fn == NULL
        || info->get_tvu_fn == NULL
    ) {
        fprintf(stderr, "Invalid interface or function pointers are NULL\n");
        return NULL;
    }

    //initialize ContactInfo_C
    struct ContactInfo_C* contact_info_c = (struct ContactInfo_C*)malloc(sizeof(struct ContactInfo_C));
    if (contact_info_c == NULL) {
        fprintf(stderr, "Memory allocation failed for ContactInfo_C\n");
        return NULL;
    }

    // Get Pubkey
    Key pubkey = info->get_pubkey_fn(info->contact_info_ptr);
    if (pubkey == NULL) {
        fprintf(stderr, "pubkey is NULL\n");
        free(contact_info_c);
        return NULL;
    }

    // Convert bytes to base58 encoded pubkey
    if (encode_base58(pubkey, 32, contact_info_c->base58_pubkey, sizeof(contact_info_c->base58_pubkey))) {
        fprintf(stderr, "Base58 encoding failed\n");
        free(contact_info_c);
        return NULL;
    }

    // Get wallclock and shred version
    contact_info_c->wallclock = info->get_wallclock_fn(info->contact_info_ptr);
    contact_info_c->shred_version = info->get_shred_version_fn(info->contact_info_ptr);

    // Get version
    if (!info->get_version_fn(info->contact_info_ptr, &contact_info_c->version)) {
        fprintf(stderr, "Failed to get version information\n");
        free(contact_info_c);
        return NULL;
    }

    // Get socket addresses
    contact_info_c->gossip_addr = NULL;
    contact_info_c->rpc_addr = NULL;
    contact_info_c->rpc_pubsub_addr = NULL;
    contact_info_c->serve_repair_addr = NULL;
    contact_info_c->tpu_udp_addr = NULL;
    contact_info_c->tpu_quic_addr = NULL;
    contact_info_c->tpu_forwards_udp_addr = NULL;
    contact_info_c->tpu_forwards_quic_addr = NULL;
    contact_info_c->tpu_vote_udp_addr = NULL;
    contact_info_c->tpu_vote_quic_addr = NULL;
    contact_info_c->tvu_udp_addr = NULL;
    contact_info_c->tvu_quic_addr = NULL;

    struct FfiSocketAddr socket;

    // GET_ADDRESS helper macro
    #define GET_ADDRESS(func, protocol, addr_field) \
        if (info->func(info->contact_info_ptr, protocol, &socket)) { \
            contact_info_c->addr_field = format_socket_addr(&socket); \
        }

    if (info->get_gossip_fn(info->contact_info_ptr, &socket)) {
        contact_info_c->gossip_addr = format_socket_addr(&socket);
    }

    if (info->get_rpc_fn(info->contact_info_ptr, &socket)) {
        contact_info_c->rpc_addr = format_socket_addr(&socket);
    }

    if (info->get_rpc_pubsub_fn(info->contact_info_ptr, &socket)) {
        contact_info_c->rpc_pubsub_addr = format_socket_addr(&socket);
    }

    GET_ADDRESS(get_serve_repair_fn, UDP, serve_repair_addr)

    GET_ADDRESS(get_tpu_fn, UDP, tpu_udp_addr)
    GET_ADDRESS(get_tpu_fn, QUIC, tpu_quic_addr)

    GET_ADDRESS(get_tpu_forwards_fn, UDP, tpu_forwards_udp_addr)
    GET_ADDRESS(get_tpu_forwards_fn, QUIC, tpu_forwards_quic_addr)

    GET_ADDRESS(get_tpu_vote_fn, UDP, tpu_vote_udp_addr)
    GET_ADDRESS(get_tpu_vote_fn, QUIC, tpu_vote_quic_addr)

    GET_ADDRESS(get_tvu_fn, UDP, tvu_udp_addr)
    GET_ADDRESS(get_tvu_fn, QUIC, tvu_quic_addr)

    #undef GET_ADDRESS

    return contact_info_c;
}

void print_contact_info(const struct ContactInfo_C *contact_info_c) {
    fprintf(stderr,
        "plugin: pk: %s, wc: %" PRIu64 ", sv: %" PRIu64 ", gs: %s, rs: %s, rpas: %s, "
        "srs: %s, tus: %s, tqs: %s, tfus: %s, tfqs: %s, tpu_vote_udp_s: %s, "
        "tpvqs: %s, tvu_udp_s: %s, tvu_quic_s: %s, version: v%u.%u.%u\n",
        contact_info_c->base58_pubkey, 
        contact_info_c->wallclock, 
        contact_info_c->shred_version, 
        contact_info_c->gossip_addr ? contact_info_c->gossip_addr : "None",
        contact_info_c->rpc_addr ? contact_info_c->rpc_addr : "None",
        contact_info_c->rpc_pubsub_addr ? contact_info_c->rpc_pubsub_addr : "None",
        contact_info_c->serve_repair_addr ? contact_info_c->serve_repair_addr : "None",
        contact_info_c->tpu_udp_addr ? contact_info_c->tpu_udp_addr : "None",
        contact_info_c->tpu_quic_addr ? contact_info_c->tpu_quic_addr : "None",
        contact_info_c->tpu_forwards_udp_addr ? contact_info_c->tpu_forwards_udp_addr : "None",
        contact_info_c->tpu_forwards_quic_addr ? contact_info_c->tpu_forwards_quic_addr : "None",
        contact_info_c->tpu_vote_udp_addr ? contact_info_c->tpu_vote_udp_addr : "None",
        contact_info_c->tpu_vote_quic_addr ? contact_info_c->tpu_vote_quic_addr : "None",
        contact_info_c->tvu_udp_addr ? contact_info_c->tvu_udp_addr : "None",
        contact_info_c->tvu_quic_addr ? contact_info_c->tvu_quic_addr : "None",
        contact_info_c->version.major, 
        contact_info_c->version.minor, 
        contact_info_c->version.patch);
    fflush(stderr); 
}

void free_contact_info(struct ContactInfo_C* contact_info_c) {
    if (contact_info_c == NULL)
        return;

    free(contact_info_c->gossip_addr);
    free(contact_info_c->rpc_addr);
    free(contact_info_c->rpc_pubsub_addr);
    free(contact_info_c->serve_repair_addr);
    free(contact_info_c->tpu_udp_addr);
    free(contact_info_c->tpu_quic_addr);
    free(contact_info_c->tpu_forwards_udp_addr);
    free(contact_info_c->tpu_forwards_quic_addr);
    free(contact_info_c->tpu_vote_udp_addr);
    free(contact_info_c->tpu_vote_quic_addr);
    free(contact_info_c->tvu_udp_addr);
    free(contact_info_c->tvu_quic_addr);

    free(contact_info_c);
}
