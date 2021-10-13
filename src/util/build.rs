use std::io::BufRead;

fn read_lines(path: &str) -> Vec<String> {
    let f = std::fs::File::open(path).expect("Failed to open cxx_bridges list");
    let buf = std::io::BufReader::new(f);
    buf.lines()
        .map(|l| l.expect("Could not parse line"))
        .collect()
}

fn main() {
    prost_build::compile_protos(
        &["../common/Meta_s.proto", "../common/V1Protocol.proto"],
        &["../common/"],
    )
    .unwrap();
    built::write_built_file().expect("Failed to acquire build-time information");

    let _build = cxx_build::bridges(read_lines("cxx_bridges.txt"));
}
