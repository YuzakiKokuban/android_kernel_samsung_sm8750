/*
 * Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

mod panic_hook;
mod argument;

use std::env;
use std::default::Default;
use std::path::{Path, PathBuf};
use std::string::String;
use std::fs::{File, OpenOptions};
use std::os::unix::io::{RawFd, FromRawFd, AsRawFd};
use std::thread;
use std::io;
use std::fmt::{self, Display};
use std::str::FromStr;
use std::thread::JoinHandle;
use std::process;

extern crate simplelog;
use simplelog::*;

extern crate android_logger;
use libc::{self, c_uint, c_int, c_char, open, O_RDWR, O_WRONLY};

use devices::virtio::{self, BlockAsync, base_features};
use hypervisor::{ProtectionType};
use devices::VirtioMmioDevice;
use devices::BusDevice;
use base::{pagesize, AsRawDescriptor, AsRawDescriptors};
use vm_memory::{GuestAddress, GuestMemory, GuestMemoryError, MemoryRegion};
use std::sync::Arc;
use std::convert::TryInto;

use devices::virtio::vhost::vsock::Vsock;
use devices::virtio::vsock::VsockConfig;
use devices::virtio::block::DiskOption;
use devices::BusAccessInfo;
use crate::{
    argument::{set_arguments, Argument},
};
use base::{FlockOperation, flock};
use base::{ioctl_with_val, ioctl_io_nr, ioctl_with_ref, ioctl_with_mut_ref, ioctl_iow_nr, ioctl_ior_nr, ioctl_iowr_nr, SafeDescriptor, FromRawDescriptor};
use base::sys::linux::validate_raw_fd;
use base::{SharedMemory, MemoryMappingBuilder, MappedRegion, MemoryMapping};
// Logging
#[macro_use]
extern crate log;

use log::{Level, LevelFilter};
use android_logger::{Config};

// Minijail
use minijail::Minijail;

static GH_PATH: &str = "/dev/gunyah";
static QGH_PATH: &str = "/dev/qgunyah";
static VIRTIO_BE_PATH: &str = "/dev/gh_virtio_backend_";
static TRACE_MARKER: &str = "/sys/kernel/tracing/trace_marker";
static VHOST_VSOCK_PATH: &str = "/dev/vhost-vsock";
// Todo: Use UAPI header files
const ASSIGN_EVENTFD: u32 = 1;
const GH_IOCTL_TYPE_V2: u32 = 0xB2;
const GH_IOCTL_TYPE_V1: u32 = 0xBC;

const VBE_ASSIGN_IRQFD: u32 = 1;

const EVENT_RESET_RQST: u32 = 2;
const EVENT_INTERRUPT_ACK: u32 = 4;
const EVENT_DRIVER_OK: u32 = 8;
const EVENT_APP_EXIT: u32 = 0x100;

const VIRTIO_MMIO_DEVICE_FEATURES: u64 = 0x10;
const VIRTIO_MMIO_DEVICE_FEATURES_SEL: u64 = 0x14;
const VIRTIO_MMIO_DRIVER_FEATURES: u64 = 0x20;
const VIRTIO_MMIO_DRIVER_FEATURES_SEL: u64 = 0x24;
const VIRTIO_MMIO_QUEUE_SEL: u64 = 0x30;
const VIRTIO_MMIO_QUEUE_NUM_MAX: u64 = 0x34;
const VIRTIO_MMIO_QUEUE_NUM: u64 = 0x38;
const VIRTIO_MMIO_QUEUE_READY: u64 = 0x44;
const VIRTIO_MMIO_QUEUE_DESC_LOW: u64 = 0x80;
const VIRTIO_MMIO_QUEUE_DESC_HIGH: u64 = 0x84;
const VIRTIO_MMIO_QUEUE_AVAIL_LOW: u64 = 0x90;
const VIRTIO_MMIO_QUEUE_AVAIL_HIGH: u64 = 0x94;
const VIRTIO_MMIO_QUEUE_USED_LOW: u64 = 0xa0;
const VIRTIO_MMIO_QUEUE_USED_HIGH: u64 = 0xa4;
const VIRTIO_MMIO_STATUS: u64 = 0x70;
const VIRTIO_MMIO_STATUS_IDX: u64 = 28;

const GH_VCPU_MAX: u16 = 512;

const CROSVM_MINIJAIL_POLICY: &str = "/system_ext/etc/seccomp_policy/qcrosvm.policy";
const LOG_TAG: &str = "qcrosvm";

const MB: u64 = 1 << 20;
const VHOST_VSOCK_HOST_CID: u64 = 2;

#[derive(Debug)]

enum BackendError {
    StrError(String),
    StrNumError{err: String, val: io::Error},
}

impl Display for BackendError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use self::BackendError::*;

        match self {
            StrError(s) => write!(f, "{}", format!("Error: {}", s)),
            StrNumError{err, val} => write!(f, "{}", format!("Error: {} ({})", err, val)),
        }
    }
}

struct VirtioDisk {
    disk: DiskOption,
    label: u32,
    mmio: Option<VirtioMmioDevice>,
    config_space: Option<Vec<u32>>,
}

struct Vcpu {
	id: u8,
	raw_fd: i32,
	thread_handle: Option<JoinHandle<()>>,
}

struct AdditionalMem {
    mem_size : u64,
    shm: Option<SharedMemory>,
    mem_region: Option<Box<MemoryMapping>>,
}

struct VsockDevice {
    enable: bool,
    config: VsockConfig,
    label: u32,
    mmio: Option<VirtioMmioDevice>,
    config_space: Option<Vec<u32>>,
}

/// Aggregate of all configurable options for a block device
struct BackendConfig {
    sfd: Option<SafeDescriptor>,
    vm_sfd: Option<SafeDescriptor>,
    vm: Option<String>,
    mem: Option<GuestMemory>,
    vdisks: Vec<VirtioDisk>,
    vsock: VsockDevice,
    vcpus: Vec<Vcpu>,
    vcpu_count: u16,
    driver_variant: u8,
    sandbox: bool,
    log_level: LevelFilter,
    log_type: Option<String>,
    additional_mem: AdditionalMem,
}

impl Default for BackendConfig {
    fn default() -> BackendConfig {
        BackendConfig {
            vdisks: Vec::new(),
            vsock: VsockDevice {
                enable: false,
                config: VsockConfig {
                    cid: 0,
                    vhost_device: PathBuf::from(VHOST_VSOCK_PATH),
                },
                label: 0,
                mmio: None,
                config_space: None,
            },
            vm: None,
            mem: None,
            sfd: None,
            vm_sfd: None,
            vcpus: Vec::new(),
            vcpu_count: 1,
            driver_variant: 2,
            sandbox: false,
            log_level: log::LevelFilter::Info,
            log_type: Some("ftrace".to_string()),
            additional_mem: AdditionalMem {
                mem_size : 0,
                shm: None,
                mem_region: None,
            },
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
struct fw_name {
	_name: [::std::os::raw::c_char; 16usize],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
struct user_memory_region {
	memory_size: u64,
	userspace_addr: u64,
	fw_name : fw_name,
}

#[repr(C)]
struct VirtioEventfd {
    _label: u32,
    _flags: u32,
    _queue_num: u32,
    _fd: RawFd,
}

#[repr(C)]
struct VirtioIrqfd {
    _label: u32,
    _flags: u32,
    _fd: RawFd,
    _reserved: u32,
}

#[repr(C)]
struct VirtioEvent {
    _label: u32,
    _event: u32,
    _event_data: u32,
    _reserved: u32,
}

#[repr(C)]
struct VirtioDevFeatures {
        _label: u32,
        _reserved: u32,
        _features_sel: u32,
        _features: u32,
}

#[repr(C)]
struct VirtioQueueMax {
        _label: u32,
        _reserved: u32,
        _queue_sel: u32,
        _queue_num_max: u32,
}

#[repr(C)]
struct VirtioConfigData {
        _label: u32,
        _config_size: u32,
        _config_data: *mut libc::c_char,
}

#[repr(C)]
struct VirtioQueueInfo {
        _label: u32,
        _queue_sel: u32,
        _queue_num: u32,
        _queue_ready: u32,
        _queue_desc: u64,
        _queue_driver: u64,
        _queue_device: u64,
}

#[repr(C)]
struct VirtioDriverFeatures {
        _label: u32,
        _reserved: u32,
        _features_sel: u32,
        _features: u32,
}

#[repr(C)]
struct VirtioAckReset {
        _label: u32,
        _reserved: u32,
}

/* system ioctls */
ioctl_io_nr!(GH_CREATE_VM,			GH_IOCTL_TYPE_V2, 0x01);

/* vm ioctls */
ioctl_io_nr!(GH_CREATE_VCPU,           	GH_IOCTL_TYPE_V2, 0x40);
ioctl_iow_nr!(GH_VM_SET_FW_NAME,		GH_IOCTL_TYPE_V2, 0x41, fw_name);
ioctl_ior_nr!(GH_VM_GET_FW_NAME,		GH_IOCTL_TYPE_V2, 0x42, fw_name);
ioctl_io_nr!(GH_GET_VCPU_COUNT,        	GH_IOCTL_TYPE_V2, 0x43);
ioctl_iow_nr!(GH_VM_GET_RESV_MEMORY_SIZE,		GH_IOCTL_TYPE_V2, 0x44, fw_name);
ioctl_iow_nr!(GH_VM_SET_USER_MEM_REGION,		GH_IOCTL_TYPE_V2, 0x45, user_memory_region);

/* vm ioctls for virtio backend driver */
ioctl_ior_nr!(GET_SHARED_MEMORY_SIZE_V2,   	GH_IOCTL_TYPE_V2, 0x61, u64);
ioctl_iow_nr!(IOEVENTFD_V2,                	GH_IOCTL_TYPE_V2, 0x62, VirtioEventfd);
ioctl_iow_nr!(IRQFD_V2,                    	GH_IOCTL_TYPE_V2, 0x63, VirtioIrqfd);
ioctl_iowr_nr!(WAIT_FOR_EVENT_V2,          	GH_IOCTL_TYPE_V2, 0x64, VirtioEvent);
ioctl_iow_nr!(SET_DEVICE_FEATURES_V2,      	GH_IOCTL_TYPE_V2, 0x65, VirtioDevFeatures);
ioctl_iow_nr!(SET_QUEUE_NUM_MAX_V2,        	GH_IOCTL_TYPE_V2, 0x66, VirtioQueueMax);
ioctl_iow_nr!(SET_DEVICE_CONFIG_DATA_V2,   	GH_IOCTL_TYPE_V2, 0x67, VirtioConfigData);
ioctl_iowr_nr!(GET_DRIVER_CONFIG_DATA_V2,  	GH_IOCTL_TYPE_V2, 0x68, VirtioConfigData);
ioctl_iowr_nr!(GET_QUEUE_INFO_V2,          	GH_IOCTL_TYPE_V2, 0x69, VirtioQueueInfo);
ioctl_iowr_nr!(GET_DRIVER_FEATURES_V2,     	GH_IOCTL_TYPE_V2, 0x6a, VirtioDriverFeatures);
ioctl_iowr_nr!(ACK_DRIVER_OK_V2,           	GH_IOCTL_TYPE_V2, 0x6b, u32);
ioctl_io_nr!(SET_APP_READY_V2,             	GH_IOCTL_TYPE_V2, 0x6c);
ioctl_iow_nr!(ACK_RESET_V2,                	GH_IOCTL_TYPE_V2, 0x6d, VirtioAckReset);

/* virtio backend driver ioctls for backward compatibility */
ioctl_ior_nr!(GET_SHARED_MEMORY_SIZE_V1,   	GH_IOCTL_TYPE_V1, 1, u64);
ioctl_iow_nr!(IOEVENTFD_V1,                	GH_IOCTL_TYPE_V1, 2, VirtioEventfd);
ioctl_iow_nr!(IRQFD_V1,                    	GH_IOCTL_TYPE_V1, 3, VirtioIrqfd);
ioctl_iowr_nr!(WAIT_FOR_EVENT_V1,          	GH_IOCTL_TYPE_V1, 4, VirtioEvent);
ioctl_iow_nr!(SET_DEVICE_FEATURES_V1,      	GH_IOCTL_TYPE_V1, 5, VirtioDevFeatures);
ioctl_iow_nr!(SET_QUEUE_NUM_MAX_V1,        	GH_IOCTL_TYPE_V1, 6, VirtioQueueMax);
ioctl_iow_nr!(SET_DEVICE_CONFIG_DATA_V1,   	GH_IOCTL_TYPE_V1, 7, VirtioConfigData);
ioctl_iowr_nr!(GET_DRIVER_CONFIG_DATA_V1,  	GH_IOCTL_TYPE_V1, 8, VirtioConfigData);
ioctl_iowr_nr!(GET_QUEUE_INFO_V1,          	GH_IOCTL_TYPE_V1, 9, VirtioQueueInfo);
ioctl_iowr_nr!(GET_DRIVER_FEATURES_V1,     	GH_IOCTL_TYPE_V1, 10, VirtioDriverFeatures);
ioctl_iowr_nr!(ACK_DRIVER_OK_V1,           	GH_IOCTL_TYPE_V1, 11, u32);
ioctl_io_nr!(SET_APP_READY_V1,             	GH_IOCTL_TYPE_V1, 12);
ioctl_iow_nr!(ACK_RESET_V1,                	GH_IOCTL_TYPE_V1, 13, VirtioAckReset);

/* vcpu ioctls */
ioctl_io_nr!(GH_VCPU_RUN,			GH_IOCTL_TYPE_V2, 0x80);

enum VmIoctl {
	IoEventFd,
	IrqFd,
	WaitForEvent,
	SetDeviceFeatures,
	SetQueueNumMax,
	SetDeviceConfigData,
	GetDriverConfigData,
	GetQueueInfo,
	GetDriverFeatures,
	AckDriverOk,
	AckReset
}

fn to_cmd(ioc: VmIoctl, version: u8) -> std::result::Result<i32, BackendError> {
	match version {
		2 => match ioc {
			VmIoctl::IoEventFd => Ok(IOEVENTFD_V2()),
			VmIoctl::IrqFd => Ok(IRQFD_V2()),
			VmIoctl::WaitForEvent => Ok(WAIT_FOR_EVENT_V2()),
			VmIoctl::SetDeviceFeatures => Ok(SET_DEVICE_FEATURES_V2()),
			VmIoctl::SetQueueNumMax => Ok(SET_QUEUE_NUM_MAX_V2()),
			VmIoctl::SetDeviceConfigData => Ok(SET_DEVICE_CONFIG_DATA_V2()),
			VmIoctl::GetDriverConfigData => Ok(GET_DRIVER_CONFIG_DATA_V2()),
			VmIoctl::GetQueueInfo => Ok(GET_QUEUE_INFO_V2()),
			VmIoctl::GetDriverFeatures => Ok(GET_DRIVER_FEATURES_V2()),
			VmIoctl::AckDriverOk => Ok(ACK_DRIVER_OK_V2()),
			VmIoctl::AckReset => Ok(ACK_RESET_V2()),
		}
		1 => match ioc {
			VmIoctl::IoEventFd => Ok(IOEVENTFD_V1()),
			VmIoctl::IrqFd => Ok(IRQFD_V1()),
			VmIoctl::WaitForEvent => Ok(WAIT_FOR_EVENT_V1()),
			VmIoctl::SetDeviceFeatures => Ok(SET_DEVICE_FEATURES_V1()),
			VmIoctl::SetQueueNumMax => Ok(SET_QUEUE_NUM_MAX_V1()),
			VmIoctl::SetDeviceConfigData => Ok(SET_DEVICE_CONFIG_DATA_V1()),
			VmIoctl::GetDriverConfigData => Ok(GET_DRIVER_CONFIG_DATA_V1()),
			VmIoctl::GetQueueInfo => Ok(GET_QUEUE_INFO_V1()),
			VmIoctl::GetDriverFeatures => Ok(GET_DRIVER_FEATURES_V1()),
			VmIoctl::AckDriverOk => Ok(ACK_DRIVER_OK_V1()),
			VmIoctl::AckReset => Ok(ACK_RESET_V1()),
		}
		_ => Err(BackendError::StrError(String::from("Unsupported driver variant."))),
	}
}

fn print_usage() {
    println!("qcrosvm [-l] [-s] [--disk=IMAGE_FILE,label=LABEL[,rw=[true|false],sparse=[true|false],block_size=BYTES]] [--mem=MBYTES] [--vsock=label=LABEL,cid=GUEST_CID] --vm=VMNAME");
    println!("\n[-l] or [--log=[level=trace|debug|info|warn|error],[type=ftrace|logcat|term]]");
    println!("Default logger level: info");
    println!("Default logger type: ftrace");

}

fn new_from_rawfd(ranges: &[(GuestAddress, u64)], fd: &RawFd) -> std::result::Result<GuestMemory, GuestMemoryError> {
        // Compute the memory alignment
        let pg_size = pagesize();
        let mut regions = Vec::new();
        let mut offset = 0;

        for range in ranges {
            if range.1 % pg_size as u64 != 0 {
                return Err(GuestMemoryError::MemoryNotAligned);
            }
	    let file = Arc::new(unsafe { File::from_raw_fd(*fd) });
	    let region = MemoryRegion::new_from_file(range.1, range.0, offset, file)
	    .map_err(|e| {
            error!("{}", format!("failed to create mem region, addr:{}, size:{}. Err: {}", range.0, range.1, e));
	    ()}).expect(&format!("{}:{}", file!(), line!()));
	    regions.push(region);
	    offset += range.1 as u64;
        }

        GuestMemory::from_regions(regions)
}

fn raw_fd_from_path(path: &Path) -> std::result::Result<RawFd, ()> {
    if !path.is_file() {
        return Err(());
    }

    let raw_fd = path
        .file_name()
        .and_then(|fd_osstr| fd_osstr.to_str())
        .and_then(|fd_str| fd_str.parse::<c_int>().ok())
        .ok_or(())?;

    validate_raw_fd(raw_fd).map_err(|_e| {()})
}

fn create_bdev(disk: &DiskOption, q_size: Option<u16>) -> std::result::Result<Box<BlockAsync>, BackendError> {
	// Special case '/proc/self/fd/*' paths. The FD is already open, just use it.
	let raw_image: File = if disk.path.parent() == Some(Path::new("/proc/self/fd")) {

		// Safe because we will validate |raw_fd|.
		unsafe {File::from_raw_fd(raw_fd_from_path(&disk.path).map_err(|_| BackendError::StrError(String::from("raw_fd_from_path failed")))?)}
	} else {
		OpenOptions::new()
		.read(true)
		.write(!disk.read_only)
		.open(&disk.path).map_err(|_| BackendError::StrNumError {
				err: String::from("open of disk file failed"),
				val: io::Error::last_os_error(),
				})?
	};

	// Lock the disk image to prevent other crosvm instances from using it.
	let lock_op = if disk.read_only {
		FlockOperation::LockShared
	} else {
		FlockOperation::LockExclusive
	};

	flock(&raw_image, lock_op, true).map_err(|_| BackendError::StrNumError {
				err: String::from("flock on disk file failed"),
				val: io::Error::last_os_error(),
			})?;

	let disk_file = disk::create_disk_file(raw_image, false, disk::MAX_NESTING_DEPTH, Path::new(&disk.path)).map_err(|_| BackendError::StrNumError {
				err: String::from("create_disk_file failed"),
				val: io::Error::last_os_error(),
				})?;

	let dev = virtio::BlockAsync::new(
		base_features(ProtectionType::Protected),
		disk_file ,
		disk,
		None,
		q_size,
		Some(1), /* Only one virtqueue is supported for now */
	).map_err(|_| BackendError::StrError(String::from("virtio_block_new failed")))?;

    Ok(Box::new(dev))
}

fn create_block_devices(cfg: &mut BackendConfig, irq_num: &mut u32) -> std::result::Result<(), BackendError> {
    for vdisk in &mut cfg.vdisks {
        let mem = cfg.mem.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let sfd :&SafeDescriptor;
	let q_size :Option<u16>;
	match cfg.driver_variant {
		1 => {sfd = cfg.sfd.as_ref().expect(&format!("{}:{}", file!(), line!())); q_size = Some(256)}
		2 => {sfd = cfg.vm_sfd.as_ref().expect(&format!("{}:{}", file!(), line!())); q_size = Some(128)}
		_ => return Err(BackendError::StrError(String::from("Unsupported driver variant.")))
	};

	let bdev = create_bdev(&vdisk.disk, q_size)?;
	vdisk.mmio = Some(VirtioMmioDevice::new(mem.clone(), bdev, true).expect(&format!("{}:{}", file!(), line!())));
        let mut idx = 0;
        let mmio = vdisk.mmio.as_mut().expect(&format!("{}:{}", file!(), line!()));
        for (e, _addr, _datamatch) in mmio.ioevents() {
            let event_fd = VirtioEventfd {
                _label : vdisk.label,
                _flags : ASSIGN_EVENTFD,
                _queue_num : idx,
                _fd : e.as_raw_descriptor(),
            };

            idx = idx + 1;
            let ret = unsafe { ioctl_with_ref(sfd, to_cmd(VmIoctl::IoEventFd, cfg.driver_variant)
								.expect(&format!("{}:{}", file!(), line!())), &event_fd) };
            if ret < 0 {
                return Err(BackendError::StrNumError {
                   err: String::from("ioeventfd ioctl failed"),
                   val: io::Error::last_os_error(),
                });
            }
        }

        let irq_evt = devices::IrqEdgeEvent::new().expect(&format!("{}:{}", file!(), line!()));
        mmio.assign_irq(&irq_evt, *irq_num);
        *irq_num = *irq_num + 1;

        let irq_fd = VirtioIrqfd {
            _label: vdisk.label,
            _fd : irq_evt.as_raw_descriptors().clone().into_iter().nth(0)
								.expect(&format!("{}:{}", file!(), line!())),
            _flags: VBE_ASSIGN_IRQFD,
            _reserved: 0,
        };

        let ret = unsafe { ioctl_with_ref(sfd, to_cmd(VmIoctl::IrqFd, cfg.driver_variant)
		                   .expect(&format!("{}:{}", file!(), line!())), &irq_fd) };
        if ret < 0 {
            return Err(BackendError::StrNumError {
                err: String::from("irqfd ioctl failed"),
                val: io::Error::last_os_error(),
            });
        }
    }

    Ok(())
}

fn create_vhost_vsock_device(vsockcfg: &VsockConfig) -> std::result::Result<Box<Vsock>, BackendError> {
        let features :u64 = base_features(ProtectionType::Protected);
        let dev = virtio::vhost::vsock::Vsock::new(features, vsockcfg)
                        .map_err(|_| BackendError::StrError(String::from("vhost vsock new failed")))?;
        Ok(Box::new(dev))
}

fn create_vsock_device(cfg: &mut BackendConfig, irq_num: &mut u32) -> std::result::Result<(), BackendError> {
        let mem = cfg.mem.as_ref().expect(&format!("{}:{}", file!(), line!()));
        let _vsock = &mut cfg.vsock;
        let vsockdev = create_vhost_vsock_device(&_vsock.config)?;
        let sfd :&SafeDescriptor;
        match cfg.driver_variant {
                1 => {sfd = cfg.sfd.as_ref().expect(&format!("{}:{}", file!(), line!()));}
                2 => {sfd = cfg.vm_sfd.as_ref().expect(&format!("{}:{}", file!(), line!()));}
                _ => return Err(BackendError::StrError(String::from("Unsupported driver variant.")))
        };

        _vsock.mmio = Some(VirtioMmioDevice::new(mem.clone(), vsockdev, true).expect(&format!("{}:{}", file!(), line!())));
        let mut idx = 0;
        let mmio = _vsock.mmio.as_mut().expect(&format!("{}:{}", file!(), line!()));
        mmio.on_sandboxed();
        for (e, _addr, _datamatch) in mmio.ioevents() {
            let event_fd = VirtioEventfd {
                _label : _vsock.label,
                _flags : ASSIGN_EVENTFD,
                _queue_num : idx,
                _fd : e.as_raw_descriptor(),
            };

            idx = idx + 1;
            let ret = unsafe { ioctl_with_ref(sfd, to_cmd(VmIoctl::IoEventFd, cfg.driver_variant)
                                                       .expect(&format!("{}:{}", file!(), line!())), &event_fd) };
            if ret < 0 {
                    return Err(BackendError::StrNumError {
                    err: String::from("ioeventfd ioctl failed"),
                    val: io::Error::last_os_error(),
                });
            }
        }

        let irq_evt = devices::IrqEdgeEvent::new().expect(&format!("{}:{}", file!(), line!()));
        mmio.assign_irq(&irq_evt, *irq_num);
        *irq_num = *irq_num + 1;

        let irq_fd = VirtioIrqfd {
            _label: _vsock.label,
            _fd : irq_evt.as_raw_descriptors().clone().into_iter().nth(0)
                                                .expect(&format!("{}:{}", file!(), line!())),
            _flags: VBE_ASSIGN_IRQFD,
            _reserved: 0,
        };

        let ret = unsafe { ioctl_with_ref(sfd, to_cmd(VmIoctl::IrqFd, cfg.driver_variant)
                               .expect(&format!("{}:{}", file!(), line!())), &irq_fd) };
        if ret < 0 {
            return Err(BackendError::StrNumError {
                err: String::from("irqfd ioctl failed"),
                val: io::Error::last_os_error(),
            });
        }

        Ok(())
}

fn mmio_write(mmio: &mut VirtioMmioDevice, offset: u64, data: u32) {
    let bytes = data.to_le_bytes();
    let info = BusAccessInfo { address: 0, id: 0, offset};
    mmio.write(info, &bytes);
}

fn mmio_read(mmio: &mut VirtioMmioDevice, offset: u64, data: &mut [u8]) {
   let info = BusAccessInfo { address: 0, id: 0, offset};
   mmio.read(info, data);
}

fn handle_driver_ok(label: u32, sfd: &SafeDescriptor, mmio: &mut VirtioMmioDevice, cspace: &mut Vec<u32>, driver_variant: u8) {
    let mut cdata = VirtioConfigData {
        _label: label,
        _config_size: 4096,
        _config_data: cspace.as_mut_ptr() as *mut c_char,
    };

    let label_copy = label;

    let ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::GetDriverConfigData, driver_variant)
	                   .expect(&format!("{}:{}", file!(), line!())), &mut cdata)};
    assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

    let mut drv_feat = VirtioDriverFeatures {
        _label: label,
        _reserved: 0,
        _features_sel: 0,
        _features: 0,
    };

    let ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::GetDriverFeatures, driver_variant)
	                   .expect(&format!("{}:{}", file!(), line!())), &mut drv_feat)};
    assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

    mmio_write(mmio, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0x0u32);

    mmio_write(mmio, VIRTIO_MMIO_DRIVER_FEATURES, drv_feat._features);

    drv_feat._features_sel = 1;
    let ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::GetDriverFeatures, driver_variant)
	                   .expect(&format!("{}:{}", file!(), line!())), &mut drv_feat)};
    assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

    mmio_write(mmio, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0x1u32);

    mmio_write(mmio, VIRTIO_MMIO_DRIVER_FEATURES, drv_feat._features);

    let qsize = mmio.ioevents().len();
    for queue_index in 0..qsize as u32 {
        let mut qinfo = VirtioQueueInfo {
                        _label: label,
                        _queue_sel: queue_index as u32,
                        _queue_num: 0,
                        _queue_ready: 0,
                        _queue_desc: 0,
                        _queue_driver: 0,
                        _queue_device: 0,
        };

        let mut queue_addr: u32;

        let ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::GetQueueInfo, driver_variant)
                               .expect(&format!("{}:{}", file!(), line!())), &mut qinfo)};
        assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

        mmio_write(mmio, VIRTIO_MMIO_QUEUE_SEL, queue_index as u32);

        mmio_write(mmio, VIRTIO_MMIO_QUEUE_NUM, qinfo._queue_num);

        queue_addr = qinfo._queue_desc as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_DESC_LOW, queue_addr);

        queue_addr = (qinfo._queue_desc >> 32) as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_DESC_HIGH, queue_addr);

        queue_addr = qinfo._queue_driver as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_AVAIL_LOW, queue_addr);

        queue_addr = (qinfo._queue_driver >> 32) as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_AVAIL_HIGH, queue_addr);

        queue_addr = qinfo._queue_device as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_USED_LOW, queue_addr);

        queue_addr = (qinfo._queue_device >> 32) as u32;
        mmio_write(mmio, VIRTIO_MMIO_QUEUE_USED_HIGH, queue_addr);

        mmio_write(mmio, VIRTIO_MMIO_QUEUE_READY, qinfo._queue_ready);
    }

    mmio_write(mmio, VIRTIO_MMIO_STATUS, cspace[VIRTIO_MMIO_STATUS_IDX as usize]);

    let ret = unsafe { ioctl_with_val(sfd, to_cmd(VmIoctl::AckDriverOk, driver_variant)
	                   .expect(&format!("{}:{}", file!(), line!())), label_copy as u64)};
    assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());
}

fn handle_events(label: u32, sfd: SafeDescriptor, mmio: &mut VirtioMmioDevice, cspace: &mut Vec<u32>, driver_variant: u8) -> u32 {
	let mut first_time = 1;
	loop {
		let mut vevent  = VirtioEvent {
			_label: label,
			_event: 0,
			_event_data: 0,
			_reserved: 0,
		};

		let ret = unsafe { ioctl_with_mut_ref(&sfd, to_cmd(VmIoctl::WaitForEvent, driver_variant)
		                   .expect(&format!("{}:{}", file!(), line!())), &mut vevent)};
		assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

		match vevent._event {
			EVENT_DRIVER_OK => handle_driver_ok(label, &sfd, mmio, cspace, driver_variant),
			EVENT_INTERRUPT_ACK =>  { }
			EVENT_RESET_RQST =>  {
				let mut ackrst = VirtioAckReset {
					_label: label,
					_reserved: 0,
				};
				if first_time == 1 {
					let ret = unsafe { ioctl_with_mut_ref(&sfd, to_cmd(VmIoctl::AckReset, driver_variant)
					                   .expect(&format!("{}:{}", file!(), line!())), &mut ackrst)};
					assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());
					first_time = 0;
				} else {
					return 0;
				}
			}
			EVENT_APP_EXIT => return 0,
			_ => error!("{}", format!("Unexpected event {} received", vevent._event)),
		}
	}
}

fn read_banked_reg(mmio: &mut VirtioMmioDevice, sel: u32, offset_write: u64, offset_read: u64) -> u32 {

	let mut val: [u8; 4] = [0; 4];

	val[0] = sel as u8;
	mmio_write(mmio, offset_write as u64, sel);
	mmio_read(mmio, offset_read as u64, &mut val);

	u32::from_le_bytes(val)
}

fn init_config_space(config_space: &mut Vec<u32>, label: u32, mmio: &mut VirtioMmioDevice, sfd: &mut SafeDescriptor, driver_variant: u8) {
	let mut val: [u8; 4] = [0; 4];
	let mut reg: u32;
	let mut offset: u32 = 0;
	let mut ret;

	while offset < 4096 {
		mmio_read(mmio, offset as u64, &mut val);
		reg = u32::from_le_bytes(val);
		config_space.push(reg);
		offset += 4;
	}

	let mut cdata = VirtioConfigData {
		_label: label,
		_config_size: 4096,
		_config_data: config_space.as_mut_ptr() as *mut c_char,
	};

	ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::SetDeviceConfigData, driver_variant)
	               .expect(&format!("{}:{}", file!(), line!())), &mut cdata) };
	assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

	let mut feat = VirtioDevFeatures {
		_label: label,
		_reserved: 0,
		_features_sel: 0,
		_features: 0,
	};

	feat._features = read_banked_reg(mmio, feat._features_sel, VIRTIO_MMIO_DEVICE_FEATURES_SEL, VIRTIO_MMIO_DEVICE_FEATURES);
	ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::SetDeviceFeatures, driver_variant)
	               .expect(&format!("{}:{}", file!(), line!())), &mut feat) };
	assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

	feat._features_sel = 1;
	feat._features = read_banked_reg(mmio, feat._features_sel, VIRTIO_MMIO_DEVICE_FEATURES_SEL, VIRTIO_MMIO_DEVICE_FEATURES);
	ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::SetDeviceFeatures, driver_variant)
	               .expect(&format!("{}:{}", file!(), line!())), &mut feat) };
	assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

	let qsize = mmio.ioevents().len();
	for queue_index in 0..qsize as u32 {
		let mut queue_max = VirtioQueueMax {
			_label: label,
			_reserved: 0,
			_queue_sel: queue_index as u32,
			_queue_num_max: 0,
		};

		queue_max._queue_num_max = read_banked_reg(mmio, queue_max._queue_sel, VIRTIO_MMIO_QUEUE_SEL, VIRTIO_MMIO_QUEUE_NUM_MAX);
		ret = unsafe { ioctl_with_mut_ref(sfd, to_cmd(VmIoctl::SetQueueNumMax, driver_variant)
			.expect(&format!("{}:{}", file!(), line!())), &mut queue_max) };
		assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());
	}
}

fn set_minijail(policy: &str) -> Result<(), ()> {
    let mut jail = Minijail::new().map_err(|_| ())?;
    jail.no_new_privs();
    jail.parse_seccomp_filters(Path::new(policy)).map_err(|_| ())?;
    jail.use_seccomp_filter();

    // Jail the current process.
    jail.enter();

    Ok(())
}

fn create_vcpus(cfg: &mut BackendConfig) -> std::result::Result<(), BackendError> {
	let vm_sfd = cfg.vm_sfd.as_ref().expect(&format!("{}:{}", file!(), line!()));
	for vcpu_id in 0..cfg.vcpu_count{
		let vcpu_fd = unsafe { libc::ioctl(vm_sfd.as_raw_descriptor(), GH_CREATE_VCPU(), vcpu_id as c_uint) };
		if vcpu_fd < 0 {
			return Err(BackendError::StrNumError {
				err: String::from("create vcpu ioctl failed"),
				val: io::Error::last_os_error(),});
		}
		cfg.vcpus.push(Vcpu {id: vcpu_id as u8, raw_fd: vcpu_fd, thread_handle: None});
	}
	Ok(())

}

fn run_a_vcpu(vcpu_rawfd: i32, cpu_id: u8, vm_name: &str) -> std::result::Result<JoinHandle<()>, BackendError>{
    let builder = thread::Builder::new()
        .name(format!("{}_vcpu{}", vm_name, cpu_id));
    let vm = vm_name.to_string();
    builder.spawn(move || {
        let ret = unsafe { libc::ioctl(vcpu_rawfd, GH_VCPU_RUN()) };
        if ret == 0 {
            error!("{}", format!("{}_vcpu{} returned 0", vm, cpu_id));
            std::process::exit(0);
        }
        else {
            error!("{}", format!("{}_vcpu{} exited with reason {}", vm, cpu_id, ret));
            panic!("{}", format!("{}_vcpu{} exited with reason {}", vm, cpu_id, ret));
        }
    }).map_err(|_| BackendError::StrNumError {
        err: format!("{}_vcpu{} thread create failed", vm_name, cpu_id),
        val: io::Error::last_os_error(),
    })
}

fn run_vcpus(cfg: &mut BackendConfig) ->  std::result::Result<(), BackendError> {
	for vcpu in &mut cfg.vcpus {
		let vcpu_rawfd = vcpu.raw_fd;
		let vm_name = cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!()));
		let handle = run_a_vcpu(vcpu_rawfd, vcpu.id, vm_name);
		if let Err(_handle) = handle {
			return Err(_handle);
		}
		vcpu.thread_handle = Some(handle.expect(&format!("{}:{}", file!(), line!())));
	}
	Ok(())

}

fn set_user_memory_region(cfg: &mut BackendConfig, vm_name: String, fw_name: fw_name) -> std::result::Result<(), BackendError> {
	let vm_sfd = cfg.vm_sfd.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let reserved_mem_size = unsafe { ioctl_with_ref(vm_sfd, GH_VM_GET_RESV_MEMORY_SIZE(), &fw_name) };
	if reserved_mem_size < 0 {
		return Err(BackendError::StrNumError {
			err: String::from("Get reserved mem size failed"),
			val: io::Error::last_os_error(),});
	}

	let reserved_mem_size = reserved_mem_size as u64;
	let total_mem_size = cfg.additional_mem.mem_size * MB;
	if reserved_mem_size > total_mem_size {
		error!("{}", format!("Error: memory size should larger than reserved memory size {:?} ", reserved_mem_size));
		panic!("{}", format!("Error: memory size should larger than reserved memory size {:?} ", reserved_mem_size));
	}

	let user_mem_size = total_mem_size - reserved_mem_size;
	let shm = SharedMemory::new(vm_name, user_mem_size).expect("Failed to create shared memory");
	let mmap = MemoryMappingBuilder::new(shm.size() as usize)
				.from_shared_memory(&shm)
				.build()
				.expect("Failed to map shared memory");

	cfg.additional_mem.shm = Some(shm);
	cfg.additional_mem.mem_region = Some(Box::new(mmap));

	let lim = libc::rlimit { rlim_cur: libc::RLIM_INFINITY, rlim_max: libc::RLIM_INFINITY };
	let ret = unsafe { libc::prlimit(process::id().try_into().unwrap(), libc::RLIMIT_MEMLOCK, &lim, std::ptr::null_mut()) };
	if ret < 0 {
		return Err(BackendError::StrNumError {
			err: String::from("Remove memlock rlimit fail"),
			val: io::Error::last_os_error(),});
	} else if ret > 0 {
		return Err(BackendError::StrError(String::from("Unexpected return value from prlimit(): {n}")));
	}

	let mem_region = cfg.additional_mem.mem_region.as_mut().expect(&format!("{}:{}", file!(), line!())).as_ptr() as u64;
	let guest_mem_desc = user_memory_region {
		memory_size: user_mem_size,
		userspace_addr: mem_region,
		fw_name: fw_name
	};

	let ret = unsafe { ioctl_with_ref(vm_sfd, GH_VM_SET_USER_MEM_REGION(), &guest_mem_desc) };
	if ret != 0 {
		return Err(BackendError::StrNumError {
			err: String::from("set user mem region failed"),
			val: io::Error::last_os_error(),});
	}
	Ok(())
}

fn run_backend_v2(cfg: &mut BackendConfig, file_name: String) -> std::result::Result<(), ()>
{
	let file = OpenOptions::new()
				.read(true)
				.write(true)
				.open(file_name).unwrap();
	let fd: i32 = file.as_raw_fd();
	if fd < 0 {
		error!("{}", format!("Error: device node open failed {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: device node open failed {:?}", io::Error::last_os_error()));
	}
	cfg.sfd = Some(unsafe { SafeDescriptor::from_raw_descriptor(fd) });
	cfg.driver_variant = 2;
	let sfd = cfg.sfd.as_mut().expect(&format!("{}:{}", file!(), line!())).try_clone()
	                          .expect(&format!("{}:{}", file!(), line!()));

	let vm_fd = unsafe { libc::ioctl(sfd.as_raw_descriptor(), GH_CREATE_VM()) };
	if vm_fd < 0 {
		error!("{}", format!("Error: create vm ioctl failed with error {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: create vm ioctl failed with error {:?}", io::Error::last_os_error()));
	}

	cfg.vm_sfd = Some(unsafe { SafeDescriptor::from_raw_descriptor(vm_fd) });

	let vm_name = cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let mut fw_name = fw_name {_name: [0; 16],};
	fw_name._name[..vm_name.len()].copy_from_slice(vm_name.as_bytes());

	if cfg.additional_mem.mem_size > 0 {
		let e = set_user_memory_region(cfg, vm_name.to_string(), fw_name);
		if let Err(_e) = e {
			error!("{}", _e);
			panic!("{}", _e);
		}
	}

	let vm_sfd = cfg.vm_sfd.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let ret = unsafe { ioctl_with_ref(vm_sfd, GH_VM_SET_FW_NAME(), &fw_name) };
	if ret != 0 {
		error!("{}", format!("Error: set fw name ioctl failed with error {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: set fw name ioctl failed with error {:?}", io::Error::last_os_error()));
	}

	let vcpu_count = unsafe { libc::ioctl(vm_fd, GH_GET_VCPU_COUNT()) };
	if vcpu_count < 0 || vcpu_count > (GH_VCPU_MAX).try_into().expect(&format!("{}:{}", file!(), line!())) {
		error!("{}", format!("Error: get vcpu count ioctl failed {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: get vcpu count ioctl failed {:?}", io::Error::last_os_error()));
	}
	cfg.vcpu_count = vcpu_count as u16;
	info!("{}", format!("vcpu_count {}", cfg.vcpu_count));

	if !cfg.vdisks.is_empty() || cfg.vsock.enable {
		let mut shmem_size: u64 = 0;
		let ret = unsafe { ioctl_with_mut_ref(vm_sfd, GET_SHARED_MEMORY_SIZE_V2(), &mut shmem_size) };
		if ret != 0 || shmem_size == 0 {
			error!("{}", format!("Error: get vm shared memory size ioctl failed {:?}", io::Error::last_os_error()));
			panic!("{}", format!("Error: get vm shared memory size ioctl failed {:?}", io::Error::last_os_error()));
		}

		info!("{}", format!("shmem_size {}", shmem_size));

		cfg.mem = Some(self::new_from_rawfd(&[(GuestAddress(0), shmem_size)], &vm_fd)
		               .expect(&format!("{}:{}", file!(), line!())));
	}

	let mut irq_num = 0;
	let mut blk_thread_handles  = Vec::new();
	if !cfg.vdisks.is_empty() {
		let e = create_block_devices(cfg, &mut irq_num);
		if let Err(_e) = e {
			error!("{}", _e);
			panic!("{}", _e);
		}

		for vdisk in &mut cfg.vdisks {
			let label = vdisk.label;
			let mut sfd = cfg.vm_sfd.as_mut().expect(&format!("{}:{}", file!(), line!())).try_clone()
			              .expect(&format!("{}:{}", file!(), line!()));
			let mut mmio = vdisk.mmio.take().expect(&format!("{}:{}", file!(), line!()));
			let mut cspace = vdisk.config_space.take().expect(&format!("{}:{}", file!(), line!()));
			let driver_variant = cfg.driver_variant;
			init_config_space(&mut cspace, label, &mut mmio, &mut sfd, driver_variant);

			debug!("Blk thread being created");
			let handle = thread::spawn(move || {
					handle_events(label, sfd, &mut mmio, &mut cspace, driver_variant);
					});
			blk_thread_handles.push(handle);
		}
	}

	let mut vsock_thread_handles = Vec::new();
	if cfg.vsock.enable {
		let vsock_err = create_vsock_device(cfg, &mut irq_num);

		if let Err(_vsock_err) = vsock_err {
			error!("{}", _vsock_err);
			return Err(());
		}

		let label = cfg.vsock.label;
		let mut sfd = cfg.vm_sfd.as_mut().expect(&format!("{}:{}", file!(), line!())).try_clone()
			      .expect(&format!("{}:{}", file!(), line!()));
		let mut mmio = cfg.vsock.mmio.take().unwrap();
		let mut cspace = cfg.vsock.config_space.take().unwrap();
		let driver_variant = cfg.driver_variant;
		init_config_space(&mut cspace, label, &mut mmio, &mut sfd, driver_variant);

		debug!("Vsock Thread being created");
		let handle = thread::spawn(move || {
				handle_events(label, sfd, &mut mmio, &mut cspace, driver_variant);
				});
		vsock_thread_handles.push(handle);
	}

	let e = create_vcpus(cfg);
	if let Err(_e) = e {
		error!("{}", _e);
		panic!("{}", _e);
	}
	let e = run_vcpus(cfg);
	if let Err(_e) = e {
		error!("{}", _e);
		panic!("{}", _e);
	}

	for vcpu in &mut cfg.vcpus {
		let _ret = vcpu.thread_handle.take().expect(&format!("{}:{}", file!(), line!())).join();
	}
	if !cfg.vdisks.is_empty() {
		for handle in blk_thread_handles {
			let _ret = handle.join();
		}
	}
	if cfg.vsock.enable {
		for handle in vsock_thread_handles {
			let _ret = handle.join();
		}
	}

	Ok(())
}

fn run_backend_v1(cfg: &mut BackendConfig) -> std::result::Result<(), ()>
{
	if cfg.vdisks.is_empty() {
		error!("Error: missing disks argument");
		print_usage();
		panic!("Error: missing disks argument");
	}

	let vm_name = cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let file_name = format!("{}{}", VIRTIO_BE_PATH, vm_name);
	let fd: i32 = unsafe { open(file_name.as_ptr() as *const c_char, O_RDWR) };
	if fd < 0 {
		error!("{}", format!("Error: device node open failed {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: device node open failed {:?}", io::Error::last_os_error()));
	}
	cfg.sfd = Some(unsafe { SafeDescriptor::from_raw_descriptor(fd) });
	cfg.driver_variant = 1;

	let sfd = cfg.sfd.as_mut().expect(&format!("{}:{}", file!(), line!())).try_clone()
	          .expect(&format!("{}:{}", file!(), line!()));
	let mut shmem_size: u64 = 0;
	let ret = unsafe { ioctl_with_mut_ref(&sfd, GET_SHARED_MEMORY_SIZE_V1(), &mut shmem_size) };
	if ret != 0 || shmem_size == 0 {
		error!("{}", format!("Error: GET_SHARED_MEMORY_SIZE ioctl failed {:?}", io::Error::last_os_error()));
		panic!("{}", format!("Error: GET_SHARED_MEMORY_SIZE ioctl failed {:?}", io::Error::last_os_error()));
	}

	info!("{}", format!("shmem_size {}", shmem_size));

	cfg.mem = Some(self::new_from_rawfd(&[(GuestAddress(0), shmem_size)], &sfd.as_raw_descriptor())
	               .expect(&format!("{}:{}", file!(), line!())));

	let mut irq_num = 0;
	let e = create_block_devices(cfg, &mut irq_num);
	if let Err(_e) = e {
		error!("{}", _e);
		panic!("{}", _e);
	}

	let mut blk_thread_handles  = Vec::new();

	for vdisk in &mut cfg.vdisks {
		let label = vdisk.label;
		let mut sfd = cfg.sfd.as_mut().expect(&format!("{}:{}", file!(), line!())).try_clone()
		              .expect(&format!("{}:{}", file!(), line!()));
		let mut mmio = vdisk.mmio.take().expect(&format!("{}:{}", file!(), line!()));
		let mut cspace = vdisk.config_space.take().expect(&format!("{}:{}", file!(), line!()));
		let driver_variant = cfg.driver_variant;
		init_config_space(&mut cspace, label, &mut mmio, &mut sfd, driver_variant);

		debug!("Thread being created");
		let handle = thread::spawn(move || {
				handle_events(label, sfd, &mut mmio, &mut cspace, driver_variant);
				});
		blk_thread_handles.push(handle);
	}

	let ret = unsafe { libc::ioctl(sfd.as_raw_descriptor(), SET_APP_READY_V1(), 0) };
	assert!(ret == 0, "{}:{}:ret={}, {}", file!(), line!(), ret, io::Error::last_os_error());

	let vm_name = cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!()));
	if Err(()) == boot_vm_v1(vm_name) { return Err(()) };

	for handle in blk_thread_handles {
		let _ret = handle.join();
	}

	Ok(())
}

fn boot_vm_v1(vm_name: &str) -> std::result::Result<(), ()>
{
	use std::io::Write;

	let boot_vm_path = format!("/sys/kernel/load_guestvm_{}/boot_guestvm", vm_name);

	if !Path::new(&boot_vm_path).exists() {
		error!("{}", format!("{} path does not exist", boot_vm_path));
		panic!("{}", format!("{} path does not exist", boot_vm_path));
	}

	let fd: i32 = unsafe { open(boot_vm_path.as_ptr() as *const c_char, O_WRONLY) };
	if fd < 0 {
		error!("{}", format!("Error: {} open failed {:?}", boot_vm_path, io::Error::last_os_error()));
		panic!("{}", format!("Error: {} open failed {:?}", boot_vm_path, io::Error::last_os_error()));
	}
	let file = unsafe { File::from_raw_fd(fd) };
	let ret = write!(&file, "1");
	match ret {
		Ok(()) => {
			info!("{}", format!("{} booted successfully", vm_name));
			return Ok(());
		},
		Err(e) => {
			error!("{}", format!("{} boot failed {:?}", vm_name, e));
			panic!("{}", format!("{} boot failed {:?}", vm_name, e));
		},
	};

}

fn run_backend(cfg: &mut BackendConfig) -> std::result::Result<(), ()>
{
	if cfg.vm.is_none() {
		error!("Error: missing vm argument");
		print_usage();
		panic!("Error: missing vm argument");
	}

        // Enforce the current process to be jailed.
        if cfg.sandbox {
            match set_minijail(CROSVM_MINIJAIL_POLICY){
                 Ok(_) => {
                     debug!("Sandboxing using minijail is enabled!!");
                 }
                 Err(_) => {
                     error!("Minijail enforcement failed!!");
                     panic!("Minijail enforcement failed!!");
                 }
            }
        }

	let vm_name = cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!()));
	let virtio_backend_dev_path = format!("{}{}", VIRTIO_BE_PATH, vm_name);
	let gh_path = format!("{}", GH_PATH);
	let qgh_path = format!("{}", QGH_PATH);

	if Path::new(&qgh_path).exists() {
		return run_backend_v2(cfg, qgh_path)
	}
	else if Path::new(&gh_path).exists() {
		return run_backend_v2(cfg, gh_path)
	}
	//Fallback to old driver - VM with virtio disks
	else if Path::new(&virtio_backend_dev_path).exists() {
		return run_backend_v1(cfg)
	}
	//Fallback to old driver - VM without virtio disks.
	else {
		return boot_vm_v1(vm_name)
	}

}

fn set_logger(cfg: &mut BackendConfig) -> std::result::Result<(), ()>
{
	let mut log_tag = String::from(LOG_TAG);

	if !cfg.vm.is_none() {
		log_tag.push('_');
		log_tag.push_str(cfg.vm.as_ref().expect(&format!("{}:{}", file!(), line!())));
	}

	match cfg.log_type.as_ref().expect(&format!("{}:{}", file!(), line!())).as_str() {
		"logcat" => {
			android_logger::init_once(
					Config::default()
					.with_max_level(LevelFilter::Trace)
					.with_tag(log_tag.as_str()));
			log::set_max_level(cfg.log_level);
		}
		"term" => {
			let config = ConfigBuilder::new()
				.set_time_level(LevelFilter::Off)
				.set_max_level(LevelFilter::Off)
				.set_location_level(LevelFilter::Off)
				.set_thread_level(LevelFilter::Off)
				.set_target_level(LevelFilter::Off)
				.with_tag(log_tag.as_str())
				.build();
			let _init = SimpleLogger::init(cfg.log_level, config);
		}
		//Default logger
		"ftrace" => {
			let config = ConfigBuilder::new()
				.set_time_level(LevelFilter::Off)
				.set_max_level(LevelFilter::Off)
				.set_location_level(LevelFilter::Off)
				.set_thread_level(LevelFilter::Off)
				.set_target_level(LevelFilter::Off)
				.with_tag(log_tag.as_str())
				.without_new_line()
				.build();
			let _init = WriteLogger::init(cfg.log_level, config, File::create(TRACE_MARKER)
					.expect(&format!("{}:{}", file!(), line!())));

		}

		_ => {}
	}

	return Ok(())
}

fn set_argument(cfg: &mut BackendConfig, name: &str, value: Option<&str>) -> argument::Result<()> {
	match name {
	"disk" => {
		let param = value.expect(&format!("{}:{}", file!(), line!()));
		let mut components = param.split(',');
		let read_only = true;
		let disk_path =
			PathBuf::from(
			components
			.next()
			.ok_or_else(|| argument::Error::InvalidValue {
				value: param.to_owned(),
				expected: String::from("missing disk path"),
			})?
		);

		if !disk_path.exists() {
			return Err(argument::Error::InvalidValue {
				value: param.to_owned(),
				expected: String::from("an existing file"),
			});
		}

		let mut vdisk = VirtioDisk {
			disk: DiskOption {
				path: disk_path,
				read_only,
				root: false,
				sparse: true,
				direct: false,
				block_size: 512,
				id: None,
				multiple_workers: false,
				async_executor: None,
				packed_queue: false,
				bootindex: None,
				pci_address: None,
			},
			label: 0,
			mmio: None,
			config_space: Some(Vec::new()),
		};

		for opt in components {
			let mut o = opt.splitn(2, '=');
			let kind = o.next().ok_or_else(|| argument::Error::InvalidValue {
				value: opt.to_owned(),
				expected: String::from("disk options must not be empty"),
			})?;

			let value = o.next().ok_or_else(|| argument::Error::InvalidValue {
					value: opt.to_owned(),
					expected: String::from("disk options must be of the form `kind=value`"),
				})?;

			match kind {
			"label" => {
				let label: u32 = u32::from_str_radix(value, 16)
					.map_err(|_| argument::Error::InvalidValue {
						value: value.to_owned(),
						expected: String::from("`label` must be an unsigned integer"),
				})?;
				if label == 0 {
					return Err(argument::Error::InvalidValue {
						value: value.to_owned(),
						expected: String::from("`label` must be a non zero integer"),
					});

				}
				vdisk.label = label;
			}

			"sparse" => {
				let sparse = value.parse().map_err(|_| argument::Error::InvalidValue {
					value: value.to_owned(),
					expected: String::from("`sparse` must be a boolean"),
				})?;
				vdisk.disk.sparse = sparse;
			}

			"block_size" => {
				let block_size =
					value.parse().map_err(|_| argument::Error::InvalidValue {
						value: value.to_owned(),
						expected: String::from("`block_size` must be an integer"),
					})?;
                                match block_size {
                                    512 | 1024 => vdisk.disk.block_size = block_size,
                                    _ => {
                                        return Err(argument::Error::InvalidValue {
                                            value: value.to_owned(),
                                            expected: String::from("`block_size` must be 512 or 1024"),
                                        });
                                    }
                                }
			}

			"rw" => {
				let rwrite: bool = value.parse().map_err(|_| argument::Error::InvalidValue {
					value: value.to_owned(),
					expected: String::from("`rw` must be a boolean"),
				})?;
				vdisk.disk.read_only = !rwrite;
			}

			_ => {
				return Err(argument::Error::InvalidValue {
					value: kind.to_owned(),
					expected: String::from("supported disk options only"),
				});
			}
			}
		}

		cfg.vdisks.push(vdisk);
	}

	"vsock" => {
		let param = value.expect(&format!("{}:{}", file!(), line!()));
		let components = param.split(',');
		let mut vsock_label: u32 = 0;
		let mut vsock_cid: u64 = 0;
		let vsock_path = PathBuf::from(VHOST_VSOCK_PATH);

		for opt in components {
			let mut o = opt.splitn(2, '=');
			let kind = o.next().ok_or_else(|| argument::Error::InvalidValue {
				value: opt.to_owned(),
				expected: String::from("vsock options must not be empty"),
			})?;

			let value = o.next().ok_or_else(|| argument::Error::InvalidValue {
					value: opt.to_owned(),
					expected: String::from("vsock options must be of the form `kind=value`"),
				})?;

			match kind {
				"label" => {
					let label: u32 = u32::from_str_radix(value, 16)
						.map_err(|_| argument::Error::InvalidValue {
							value: value.to_owned(),
							expected: String::from("`label` must be an unsigned integer"),
					})?;
					if label == 0 {
						return Err(argument::Error::InvalidValue {
							value: value.to_owned(),
							expected: String::from("`label` must be a non zero integer"),
						});
					}
					vsock_label = label;
				}

				"cid" => {
					let cid = value.parse().map_err(|_| argument::Error::InvalidValue {
						value: value.to_owned(),
						expected: String::from("cid must be an unsigned integer"),
					})?;

					if cid ==  VHOST_VSOCK_HOST_CID {
						return Err(argument::Error::InvalidValue {
							value: value.to_owned(),
							expected: String::from("guest cid cannot equal host cid"),
						});
					}
					vsock_cid = cid;
				}

				_ => {
					return Err(argument::Error::InvalidValue {
						value: kind.to_owned(),
						expected: String::from("supported vsock options only"),
					});
				}
			}
		}
		cfg.vsock = VsockDevice {
			enable: true,
			config: VsockConfig {
				cid: vsock_cid,
				vhost_device: vsock_path,
			},
			label: vsock_label,
			mmio: None,
			config_space: Some(Vec::new()),
		};
	}
	"vm" => {
		cfg.vm = Some(value.expect(&format!("{}:{}", file!(), line!())).to_owned());
		//PID would be required for log analysis of all log levels. Hence error!().
		error!("{}", format!("qcrosvm PID for {}: {}", cfg.vm.as_ref()
		      .expect(&format!("{}:{}", file!(), line!())), process::id()));
	}

	"sandbox" => {
                cfg.sandbox = true;
	}

	"mem" => {
		let mem_str = value.expect(&format!("{}:{}", file!(), line!()));
		let mem_size: u64 = u64::from_str_radix(&mem_str, 10)
					.map_err(|_| argument::Error::InvalidValue {
						value: mem_str.to_owned(),
						expected: String::from("`mem` must be an unsigned integer"),
				})?;

		if mem_size > 1000 {
			return Err(argument::Error::InvalidValue {
				value: mem_str.to_owned(),
				expected: String::from("`mem` must be no greater than 1000"),
			});
		}

		cfg.additional_mem = AdditionalMem {
			mem_size: mem_size,
			shm: None,
			mem_region: None
		};
	}

        "log" => {
		let param = value.expect(&format!("{}:{}", file!(), line!()));
		let components = param.split(',');

		for opt in components {
			let mut o = opt.splitn(2, '=');
			let kind = o.next().ok_or_else(|| argument::Error::InvalidValue {
				value: opt.to_owned(),
				expected: String::from("log options must not be empty"),
			})?;

			let value = o.next().ok_or_else(|| argument::Error::InvalidValue {
					value: opt.to_owned(),
					expected: String::from("log options must be of the form `kind=value`"),
				})?;

			match kind {
				"level" => {
					let level = value.to_owned();
					match Level::from_str(&level)
					{
						Ok(temp_log_level) => {
							// Reset the logging level
							cfg.log_level = temp_log_level.to_level_filter();
						}
						Err(_) =>  {
							return Err(argument::Error::InvalidValue {
								value: level,
								expected: String::from("trace | debug | info | warn | error"),
								});
						}
					}
				}

				"type" => {
					let logger_type = value.to_owned();
					match logger_type.as_str() {
						"logcat"|"term"|"ftrace" => {
							cfg.log_type = Some(logger_type);
						}
						_ => {
							return Err(argument::Error::InvalidValue {
								value: value.to_owned(),
								expected: String::from
								("supported logger options. 'type=logcat|term|ftrace"),
								});
						}
					}
				}

				_ => {
					return Err(argument::Error::InvalidValue {
						value: kind.to_owned(),
						expected: String::from("supported logger options. 'type=logcat | term | ftrace'"),
					});
				}
			}
		}
        }

	_ => unreachable!(),

	}

	Ok(())
}

fn parse_and_run(args: std::env::Args) -> std::result::Result<(), ()> {
	let arguments =
			&[
			Argument::short_value('d', "disk", "PATH,label=LABEL[,key=value[,key=value[,...]]", "Path to a disk image followed by comma-separated options.
			Valid keys:
			label=LABEL - Indicates the label associated with the virtual (disk)
			sparse=BOOL - Indicates whether the disk should support the discard operation (default: true)
			block_size=BYTES - Set the reported block size of the disk (default: 512)
			rw - Sets the disk as read-writeable"),

			Argument::short_value('l', "log",
			"[level=trace|debug|info|warn|error],[type=ftrace|logcat|term]",
			"Logging Configurations. Default level: info, Default type: ftrace"),
			Argument::short_value('v', "vm", "VMNAME", "Virtual Machine Name"),
			Argument::short_value('m', "mem", "MEMORY_SIZE", "Total virtual machine memory size in MB include additional memory from user space. VM must be debuggable."),
			Argument::short_flag('s', "sandbox", "Sandbox using minijail (default: disabled."),
			Argument::value("vsock", "label=LABEL,cid=GUEST_CID",
			"label=LABEL - Indicates the label associated with vsock
			cid=GUEST_CID - Indicates the cid of guest VM"),
		];

	let mut cfg = BackendConfig::default();
	let match_res = set_arguments(args, &arguments[..], |name, value| {
			set_argument(&mut cfg, name, value)
	});

	_ = set_logger(&mut cfg);

	match match_res {

	Ok(()) => match run_backend(&mut cfg) {
		Ok(_) => {
			info!("backend exited normally");
			Ok(())
		}

		Err(_) => {
			Err(())
		}
	},

	Err(e) => {
		error!("{}", format!("Error parsing arguments {:?}", e));
		Err(())
	}
	}
}

fn backend_main() -> std::result::Result<(), ()> {

    match env::var("KBDEV") {
	    Ok(_) => panic_hook::set_panic_hook(),
	    Err(_) => {},
    }

    let mut args = std::env::args();

    if args.next().is_none() {
        print_usage();
        return Err(());
    }

    return parse_and_run(args);
}

fn main() {
    std::process::exit(if backend_main().is_ok() { 0 } else { 1 });
}
