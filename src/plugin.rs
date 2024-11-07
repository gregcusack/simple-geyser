use {
    log::*,
    agave_geyser_plugin_interface::geyser_plugin_interface::{
        GeyserPlugin, ReplicaAccountInfoVersions, Result as PluginResult,
        ReplicaTransactionInfoVersions, FfiPubkey, GeyserPluginError,
    },
    solana_gossip::contact_info_ffi::{FfiSocketAddr, ContactInfoInterface, ffi_socket_addr_to_socket_addr},
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
        let pubkey_ptr = unsafe { (interface.get_pubkey_fn)(interface.contact_info_ptr) };
        let pubkey_bytes = unsafe { std::slice::from_raw_parts(pubkey_ptr, 32) };
        let pk = Pubkey::try_from(pubkey_bytes).unwrap();
        let wallclock = unsafe { (interface.get_wallclock_fn)(interface.contact_info_ptr) };
        let shred_version = unsafe { (interface.get_shred_version_fn)(interface.contact_info_ptr) };
        let mut ffi_gossip_socket = FfiSocketAddr::default();

        let success = unsafe {
            (interface.get_gossip_fn)(
                interface.contact_info_ptr,
                &mut ffi_gossip_socket as *mut FfiSocketAddr,
            )
        };
        if !success {
            error!("greg: failed gossip socket: {:?}", pk);
            return Err(GeyserPluginError::Custom(Box::new(std::io::Error::new(
                std::io::ErrorKind::Other,
                "greg: failed to get gossip socket",
            ))));
        }
        let gossip = ffi_socket_addr_to_socket_addr(&ffi_gossip_socket);
        info!("greg: pk: {}, wc: {}, sv: {}, gs: {}", pk, wallclock, shred_version, gossip);

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

