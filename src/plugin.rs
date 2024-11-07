use {
    agave_geyser_plugin_interface::geyser_plugin_interface::{
        FfiPubkey, GeyserPlugin, GeyserPluginError, ReplicaAccountInfoVersions, ReplicaTransactionInfoVersions, Result as PluginResult
    }, 
    log::*, 
    solana_gossip::contact_info_ffi::{
        ffi_socket_addr_to_socket_addr, 
        FfiContactInfoInterface,
        FfiProtocol,
    }, 
    solana_program::slot_history::Slot, 
    solana_sdk::pubkey::Pubkey,
};

pub const RUST_LOG_FILTER: &str = "info";

#[derive(Debug, Default)]
pub struct SimplePlugin {}

impl GeyserPlugin for SimplePlugin {
    fn name(&self) -> &'static str {
        "simple-geyser"
    }

    fn on_load(&mut self, _config_file: &str, _is_reload: bool) -> PluginResult<()> {
        solana_logger::setup_with_default(RUST_LOG_FILTER); // Ensure logging is initialized
        Ok(())
    }

    fn on_unload(&mut self) {}

    fn update_account(
        &self,
        account: ReplicaAccountInfoVersions,
        slot: u64,
        _is_startup: bool,
    ) -> PluginResult<()> {

        let pubkey_bytes = match account {
            ReplicaAccountInfoVersions::V0_0_1(account_info) => account_info.pubkey,
            ReplicaAccountInfoVersions::V0_0_2(account_info) => account_info.pubkey,
            ReplicaAccountInfoVersions::V0_0_3(account_info) => account_info.pubkey,
        };

        info!(
            "account {:?} updated at slot {}!",
            Pubkey::try_from(pubkey_bytes).unwrap(),
            slot
        );

        Ok(())
    }

    fn notify_end_of_startup(&self) -> PluginResult<()> {
        Ok(())
    }

    fn account_data_notifications_enabled(&self) -> bool {
        false // process account changes
    }

    fn transaction_notifications_enabled(&self) -> bool {
        false // dont process new txs
    }

    fn notify_transaction(
        &self,
        transaction: ReplicaTransactionInfoVersions,
        slot: Slot,
    ) -> PluginResult<()> {
        match transaction {
            ReplicaTransactionInfoVersions::V0_0_1(transaction_info) => {
                info!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
            ReplicaTransactionInfoVersions::V0_0_2(transaction_info) => {
                info!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
        }
        Ok(())
    }

    fn notify_node_update(&self, interface: &FfiContactInfoInterface) -> PluginResult<()> {
        let pk = interface.pubkey().map_err(|e| {
            error!("greg: failed pubkey: {:?}", e);
            GeyserPluginError::NodeUpdateError {
                msg: e.to_string(),
            }
        })?;
        let wallclock = interface.wallclock();
        let shred_version = interface.shred_version();

        let gossip = interface.gossip().map(|ffi_socket| {
            ffi_socket_addr_to_socket_addr(&ffi_socket)
        });

        let rpc = interface.rpc().map(|ffi_socket| {
            ffi_socket_addr_to_socket_addr(&ffi_socket)
        });

        let tpu_quic = interface.tpu(FfiProtocol::QUIC).map(|ffi_socket| {
            ffi_socket_addr_to_socket_addr(&ffi_socket)
        });

        let tpu_udp = interface.tpu(FfiProtocol::UDP).map(|ffi_socket| {
            ffi_socket_addr_to_socket_addr(&ffi_socket)
        });

        info!("greg: pk: {}, wc: {}, sv: {}, gs: {:?}, rs: {:?}, tsq: {:?}, tsu: {:?}", pk, wallclock, shred_version, gossip, rpc, tpu_quic, tpu_udp);

        Ok(())
    }

    fn notify_node_removal(&self, pubkey: &FfiPubkey) -> PluginResult<()> {
        info!("greg: pk removed pk: {}", Pubkey::from(pubkey.pubkey));
        Ok(())
    }

    fn node_update_notifications_enabled(&self) -> bool {
        true
    }

}

