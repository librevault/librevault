use std::env;
use std::path::PathBuf;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Tonic
    let descriptor_path =
        PathBuf::from(env::var("OUT_DIR").unwrap()).join("librevault.controller.v1.bin");
    tonic_build::configure()
        .file_descriptor_set_path(descriptor_path)
        .format(true)
        .compile(&["src/proto/controller.proto"], &["src/proto/"])?;

    // Prost
    prost_build::compile_protos(
        &["src/proto/protocol.proto", "src/proto/meta.proto"],
        &["src/proto/"],
    )
    .unwrap();
    Ok(())
}
