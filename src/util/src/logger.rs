use crate::convert_str;
use crate::ffi::FfiString;
use log::log;

#[repr(C)]
#[allow(dead_code)]
pub enum Level {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Trace = 4,
}

#[no_mangle]
pub extern "C" fn log_message(level: Level, msg: FfiString, target: FfiString) {
    let log_level = match level {
        Level::Error => log::Level::Error,
        Level::Warn => log::Level::Warn,
        Level::Info => log::Level::Info,
        Level::Debug => log::Level::Debug,
        Level::Trace => log::Level::Trace,
    };
    log!(target: target.to_str(), log_level, "{}", msg.to_str());
}

#[no_mangle]
pub extern "C" fn log_init() {
    simple_logger::SimpleLogger::new().env().init().unwrap();
}
