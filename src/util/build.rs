fn main() {
    prost_build::compile_protos(
        &["../common/Meta_s.proto", "../common/V1Protocol.proto"],
        &["../common/"],
    )
    .unwrap();
    built::write_built_file().expect("Failed to acquire build-time information");

    let _build = cxx_build::bridges([
        "src/lib.rs",
        "src/logger.rs",
        "src/nodekey.rs",
        "src/path_normalize.rs",
        "src/aescbc.rs",
    ]);
}
