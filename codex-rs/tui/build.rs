fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("haiku") {
        cc::Build::new()
            .cpp(true)
            .file("src/clipboard_paste/haiku.cpp")
            .compile("codex_haiku_clipboard");
        println!("cargo:rustc-link-lib=be");
        println!("cargo:rustc-link-lib=translation");
        println!("cargo:rerun-if-changed=src/clipboard_paste/haiku.cpp");
    }
}
