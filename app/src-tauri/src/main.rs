// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    if std::env::args()
        .skip(1)
        .any(|a| a == "--version" || a == "-v")
    {
        println!("{}", env!("CARGO_PKG_VERSION"));
        return;
    }
    biopass_lib::run()
}
