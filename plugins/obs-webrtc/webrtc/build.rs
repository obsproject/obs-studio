extern crate cbindgen;

use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    let config = cbindgen::Config {
        language: cbindgen::Language::C,
        ..Default::default()
    };

    match cbindgen::generate_with_config(&crate_dir, config) {
        Ok(bindings) => bindings.write_to_file("./bindings.h"),
        Err(cbindgen::Error::ParseSyntaxError { .. }) => return, // ignore in favor of cargo's syntax check
        Err(err) => panic!("{:?}", err)
    };
}