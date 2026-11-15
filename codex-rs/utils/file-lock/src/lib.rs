use std::fs::File;
use std::fs::TryLockError;

pub fn try_lock(file: &File) -> Result<(), TryLockError> {
    #[cfg(not(target_os = "haiku"))]
    {
        file.try_lock()
    }
    #[cfg(target_os = "haiku")]
    {
        try_flock(file, 0x02)
    }
}

pub fn try_lock_shared(file: &File) -> Result<(), TryLockError> {
    #[cfg(not(target_os = "haiku"))]
    {
        file.try_lock_shared()
    }
    #[cfg(target_os = "haiku")]
    {
        try_flock(file, 0x01)
    }
}

pub fn lock(file: &File) -> std::io::Result<()> {
    #[cfg(not(target_os = "haiku"))]
    {
        file.lock()
    }
    #[cfg(target_os = "haiku")]
    {
        flock(file, 0x02)
    }
}

pub fn lock_shared(file: &File) -> std::io::Result<()> {
    #[cfg(not(target_os = "haiku"))]
    {
        file.lock_shared()
    }
    #[cfg(target_os = "haiku")]
    {
        flock(file, 0x01)
    }
}

pub fn unlock(file: &File) -> std::io::Result<()> {
    #[cfg(not(target_os = "haiku"))]
    {
        file.unlock()
    }
    #[cfg(target_os = "haiku")]
    {
        flock(file, 0x08)
    }
}

// Haiku's <sys/file.h> defines LOCK_SH=1, LOCK_EX=2, LOCK_NB=4, LOCK_UN=8.
#[cfg(target_os = "haiku")]
fn flock(file: &File, operation: i32) -> std::io::Result<()> {
    use std::os::fd::AsRawFd;
    unsafe extern "C" {
        fn flock(fd: i32, operation: i32) -> i32;
    }
    if unsafe { flock(file.as_raw_fd(), operation) } == 0 {
        Ok(())
    } else {
        Err(std::io::Error::last_os_error())
    }
}

#[cfg(target_os = "haiku")]
fn try_flock(file: &File, operation: i32) -> Result<(), TryLockError> {
    flock(file, operation | 0x04).map_err(|error| {
        if error.kind() == std::io::ErrorKind::WouldBlock {
            TryLockError::WouldBlock
        } else {
            TryLockError::Error(error)
        }
    })
}

#[cfg(test)]
mod tests {
    use super::try_lock;
    use super::try_lock_shared;
    use std::fs::OpenOptions;
    use std::fs::TryLockError;

    #[test]
    fn lock_blocks_an_independent_open_and_releases_on_close() {
        let path =
            std::env::temp_dir().join(format!("codex-arg0-lock-test-{}", std::process::id()));
        let open = || {
            OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .truncate(false)
                .open(&path)
                .unwrap()
        };
        let first = open();
        let second = open();
        try_lock(&first).unwrap();
        assert!(matches!(try_lock(&second), Err(TryLockError::WouldBlock)));
        drop(first);
        try_lock(&second).unwrap();
        drop(second);
        std::fs::remove_file(path).unwrap();
    }

    #[test]
    fn shared_locks_coexist_and_exclude_a_writer() {
        let path =
            std::env::temp_dir().join(format!("codex-shared-lock-test-{}", std::process::id()));
        let open = || {
            OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .truncate(false)
                .open(&path)
                .unwrap()
        };
        let first = open();
        let second = open();
        let writer = open();
        try_lock_shared(&first).unwrap();
        try_lock_shared(&second).unwrap();
        assert!(matches!(try_lock(&writer), Err(TryLockError::WouldBlock)));
        drop(first);
        drop(second);
        try_lock(&writer).unwrap();
        drop(writer);
        std::fs::remove_file(path).unwrap();
    }

    #[test]
    fn explicit_unlock_allows_another_writer() {
        let path = std::env::temp_dir().join(format!("codex-unlock-test-{}", std::process::id()));
        let open = || {
            OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .truncate(false)
                .open(&path)
                .unwrap()
        };
        let first = open();
        let second = open();
        super::lock(&first).unwrap();
        assert!(matches!(try_lock(&second), Err(TryLockError::WouldBlock)));
        super::unlock(&first).unwrap();
        try_lock(&second).unwrap();
        drop(first);
        drop(second);
        std::fs::remove_file(path).unwrap();
    }
}
