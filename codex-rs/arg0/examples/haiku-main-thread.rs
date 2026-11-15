#[cfg(target_os = "haiku")]
fn main() -> anyhow::Result<()> {
    unsafe extern "C" {
        fn find_thread(name: *const std::ffi::c_char) -> i32;
        fn getpid() -> i32;
    }
    codex_arg0::arg0_dispatch_or_else(|_| async {
        use std::os::unix::process::CommandExt;

        let thread = unsafe { find_thread(std::ptr::null()) };
        let team = unsafe { getpid() };
        anyhow::ensure!(
            thread == team,
            "dispatcher ran on worker thread {thread} instead of main thread {team}"
        );
        let error = std::process::Command::new("/bin/sh")
            .args(["-c", "exit 0"])
            .exec();
        Err(error.into())
    })
}

#[cfg(not(target_os = "haiku"))]
fn main() {}
