use notify::{DebouncedEvent, RecommendedWatcher, Watcher};
use std::time::Duration;

pub struct WatcherWrapper {
    pub watcher: RecommendedWatcher,
    _handle: std::thread::JoinHandle<()>,
}

impl WatcherWrapper {
    pub fn new<F: Fn(DebouncedEvent) + Send + 'static>(f: F) -> Self {
        let (tx, rx) = std::sync::mpsc::channel();
        let watcher = RecommendedWatcher::new(tx, Duration::from_millis(500)).unwrap();

        let handle = std::thread::spawn(move || {
            while let Ok(ev) = rx.recv() {
                f(ev)
            }
        });

        Self {
            watcher,
            _handle: handle,
        }
    }
}
