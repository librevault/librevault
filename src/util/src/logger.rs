use log::log;

fn log_message(level: &str, msg: &str, target: &str) {
    let log_level = match level {
        "error" => log::Level::Error,
        "warn" => log::Level::Warn,
        "info" => log::Level::Info,
        "debug" => log::Level::Debug,
        "trace" => log::Level::Trace,
        _ => unimplemented!(),
    };
    log!(target: target, log_level, "{}", msg);
}

fn log_init() {
    simple_logger::SimpleLogger::new().env().init().unwrap();
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn log_init();
        fn log_message(level: &str, msg: &str, target: &str);
    }
}
