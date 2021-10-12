fn main() {
    prost_build::compile_protos(
        &["../common/Meta_s.proto", "../common/V1Protocol.proto"],
        &["../common/"],
    )
    .unwrap();
    built::write_built_file().expect("Failed to acquire build-time information");

    cxx_build::bridge("src/lib.rs");
    println!("cargo:rerun-if-changed=src/lib.rs");
}
