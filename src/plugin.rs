use { 
    log::*,
    agave_geyser_plugin_interface::geyser_plugin_interface::{
        GeyserPlugin, ReplicaAccountInfoVersions, Result as PluginResult,
        ReplicaTransactionInfoVersions, ContactInfoVersions, GeyserPluginError,
        FfiContactInfo,
    },
    solana_program::{
        slot_history::Slot,
        pubkey::Pubkey,
    },    
    std::{
        fs::OpenOptions,
        path::Path,
        sync::atomic::{AtomicU64, Ordering},
        mem::size_of,
    },
    memmap2::{MmapMut, MmapOptions},
};



pub const RUST_LOG_FILTER: &str = "info";
const SHM_PATH: &str = "/tmp/ffi_contact_info_shm";
const BUFFER_CAPACITY: usize = 10_000;
static mut MMAP: Option<MmapMut> = None;
static mut HEAD: *mut AtomicU64 = std::ptr::null_mut();
static mut TAIL: *mut AtomicU64 = std::ptr::null_mut();
static ENTRY_SIZE: usize = size_of::<FfiContactInfo>();

#[derive(Debug, Default)]
pub struct SimplePlugin {}

impl SimplePlugin {
    fn init_shared_memory(&self) -> PluginResult<()> {
        let shm_size = size_of::<u64>() * 2 + BUFFER_CAPACITY * ENTRY_SIZE;

        // Create or open the shared memory file
        let shm_path = Path::new(SHM_PATH);
        let file = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .open(&shm_path)
            .map_err(|e| GeyserPluginError::Custom(Box::new(e)))?;

        // Set the size of the shared memory file
        file.set_len(shm_size as u64)
            .map_err(|e| GeyserPluginError::Custom(Box::new(e)))?;

        // Memory-map the file
        let mut mmap = unsafe {
            MmapOptions::new()
                .len(shm_size)
                .map_mut(&file)
                .map_err(|e| GeyserPluginError::Custom(Box::new(e)))?
        };

        // Initialize head and tail to 0
        unsafe {
            let head_ptr = mmap[..size_of::<u64>()].as_mut_ptr() as *mut AtomicU64;
            let tail_ptr = mmap[size_of::<u64>()..size_of::<u64>() * 2]
                .as_mut_ptr() as *mut AtomicU64;

            head_ptr.write(AtomicU64::new(0));
            tail_ptr.write(AtomicU64::new(0));

            MMAP = Some(mmap);
            HEAD = head_ptr;
            TAIL = tail_ptr;
        }

        Ok(())
    }
}

impl GeyserPlugin for SimplePlugin {
    fn name(&self) -> &'static str {
        "simple-geyser"
    }

    fn on_load(&mut self, _config_file: &str, _is_reload: bool) -> PluginResult<()> {
        solana_logger::setup_with_default(RUST_LOG_FILTER); // Ensure logging is initialized
        // Initialize shared memory
        self.init_shared_memory()?;
        Ok(())
    }

    fn on_unload(&mut self) {
        unsafe {
            // Clean up shared memory
            MMAP = None;
            HEAD = std::ptr::null_mut();
            TAIL = std::ptr::null_mut();
        }
    }

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

        error!(
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
                error!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
            ReplicaTransactionInfoVersions::V0_0_2(transaction_info) => {
                error!("transaction {:#?} at slot {}!", transaction_info, slot);
            }
        }
        Ok(())
    }

    fn insert_crds_value(&self, ci: ContactInfoVersions) -> PluginResult<()> {

        let ContactInfoVersions::V0_0_1(ffi_ci) = ci;
        error!("ContactInfoVersions::V0_0_1 pk: {}, wc: {}, sv: {}", Pubkey::from(ffi_ci.pubkey), ffi_ci.wallclock, ffi_ci.shred_version);
        
        // Ensure shared memory is initialized
        if unsafe { MMAP.is_none() } {
            return Err(GeyserPluginError::Custom(Box::new(std::io::Error::new(
                std::io::ErrorKind::Other,
                "Shared memory not initialized",
            ))));
        }

        // Write to shared memory
        unsafe {
            let mmap = MMAP.as_mut().expect("Shared memory not initialized");
            let head = &*HEAD;
            let tail = &*TAIL;

            let current_head = head.load(Ordering::SeqCst);
            let index = current_head % BUFFER_CAPACITY as u64;
            let offset = size_of::<u64>() * 2 + index as usize * ENTRY_SIZE;

            // Copy ffi_contact_info into shared memory at the calculated offset
            let ptr = mmap[offset..offset + ENTRY_SIZE].as_mut_ptr() as *mut FfiContactInfo;
            ptr.write(ffi_ci);

            // Update head
            head.fetch_add(1, Ordering::SeqCst);

            // Handle overwriting old data if buffer is full
            if head.load(Ordering::SeqCst) - tail.load(Ordering::SeqCst) > BUFFER_CAPACITY as u64 {
                // Buffer is full; advance tail
                tail.fetch_add(1, Ordering::SeqCst);
            }
        }        
        Ok(())
    }

    fn gossip_messages_notifications_enabled(&self) -> bool {
        true
    }

}

