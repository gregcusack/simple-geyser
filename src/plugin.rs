use {
    log::*,
    agave_geyser_plugin_interface::geyser_plugin_interface::{
        GeyserPlugin, ReplicaAccountInfoVersions, Result as PluginResult,
        ReplicaTransactionInfoVersions, FfiNode, FfiPubkey,
    },
    solana_sdk::pubkey::Pubkey,
    solana_program::slot_history::Slot,
    std::{
        sync::{Mutex, atomic::{AtomicU64, Ordering}},
        time::{Duration, Instant},
    },
    
};

pub const RUST_LOG_FILTER: &str = "info";

#[derive(Debug)]
pub struct SimplePlugin {
    event_count: AtomicU64,
    // byte_count: AtomicU64,
    last_measurement: Mutex<Instant>,
}

impl Default for SimplePlugin {
    fn default() -> Self {
        SimplePlugin {
            event_count: AtomicU64::new(0),
            // byte_count: AtomicU64::new(0),
            last_measurement: Mutex::new(Instant::now()),
        }
    }
}

impl SimplePlugin {
    fn log_and_reset_metrics(&self) {
        let mut last_measurement = self.last_measurement.lock().unwrap(); // Lock for exclusive access
        let elapsed = last_measurement.elapsed();
        
        if elapsed >= Duration::from_secs(1) {
            // Swap and reset the counters
            let events = self.event_count.swap(0, Ordering::Relaxed);
            // let bytes = self.byte_count.swap(0, Ordering::Relaxed);

            let rate = events as f64 / elapsed.as_secs_f64();
            // let bandwidth = bytes as f64 / elapsed.as_secs_f64();

            info!(
                "greg: event rate: {:.2} events/s",
                rate
            );

            // Update `last_measurement` to the current time
            *last_measurement = Instant::now();
        }
    }
}

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

    fn notify_node_update(&self, ffi_node: &FfiNode) -> PluginResult<()> {
        info!("greg: ffi_node -> pk: {}, wc: {}, sv: {}", Pubkey::from(ffi_node.pubkey.pubkey), ffi_node.wallclock, ffi_node.shred_version);

        // let serialized_value = bincode::serialize(&ffi_node).expect("Failed to serialize value");
        // let byte_len = serialized_value.len() as u64;

        // Increment counters
        self.event_count.fetch_add(1, Ordering::Relaxed);
        // self.byte_count.fetch_add(byte_len, Ordering::Relaxed);
        
        // Check if we need to log and reset the rate/bandwidth metrics
        self.log_and_reset_metrics();

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

