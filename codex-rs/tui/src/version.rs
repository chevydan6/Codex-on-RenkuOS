/// The current Codex CLI version as embedded at compile time.
#[cfg(target_os = "haiku")]
pub const CODEX_CLI_VERSION: &str = "0.171.0-alpha.5";
#[cfg(not(target_os = "haiku"))]
pub const CODEX_CLI_VERSION: &str = env!("CARGO_PKG_VERSION");
