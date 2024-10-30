use { 
    agave_geyser_plugin_interface::geyser_plugin_interface::{
        GeyserPlugin, ReplicaAccountInfoVersions, Result as PluginResult,
        ReplicaTransactionInfoVersions,
    },
    solana_program::{
        slot_history::Slot,
        pubkey::Pubkey,
    },
    solana_gossip::crds::VersionedCrdsValue,
    
};

#[derive(Debug, Default)]
pub struct SimplePlugin {}

impl GeyserPlugin for SimplePlugin {
    fn name(&self) -> &'static str {
        "simple-geyser"
    }

    fn on_load(&mut self, _config_file: &str, _is_reload: bool) -> PluginResult<()> {
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

        println!(
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
                println!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
            ReplicaTransactionInfoVersions::V0_0_2(transaction_info) => {
                println!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
        }
        Ok(())
    }

    fn insert_crds_value(&self, value: VersionedCrdsValue) -> PluginResult<()> {
        println!("greg: rx origin pk: {}", value.value.pubkey());
        Ok(())
    }

    fn gossip_messages_notifications_enabled(&self) -> bool {
        true
    }

}

