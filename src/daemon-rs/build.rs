use std::env;
use std::path::{Path, PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let descriptor_path =
        PathBuf::from(env::var("OUT_DIR").unwrap()).join("librevault.controller.v1.bin");
    tonic_build::configure()
        .file_descriptor_set_path(descriptor_path)
        .format(true)
        .compile(&["src/proto/controller.proto"], &["src/proto/"])?;
    Ok(())
}
