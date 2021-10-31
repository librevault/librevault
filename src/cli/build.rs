use std::env;
use std::path::PathBuf;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::configure().format(true).compile(
        &["../daemon-rs/src/proto/controller.proto"],
        &["../daemon-rs/src/proto/"],
    )?;
    Ok(())
}
