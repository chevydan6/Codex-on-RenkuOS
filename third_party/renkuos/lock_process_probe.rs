#[path="/boot/home/src/codex-renkuos/codex-rs/utils/file-lock/src/lib.rs"]
mod locking;
use std::{fs::OpenOptions,io::{BufRead,BufReader,Write},process::{Command,Stdio}};
fn main(){
 let args:Vec<String>=std::env::args().collect();
 let path=args.get(2).cloned().unwrap_or_else(||format!("/CodexBuild/tmp/codex-lock-process-{}",std::process::id()));
 let file=OpenOptions::new().create(true).truncate(false).read(true).write(true).open(&path).unwrap();
 if args.get(1).map(String::as_str)==Some("hold") {
  locking::lock(&file).unwrap();println!("LOCKED");std::io::stdout().flush().unwrap();
  loop{std::thread::sleep(std::time::Duration::from_secs(60));}
 }
 for _ in 0..20 {
  let mut child=Command::new(std::env::current_exe().unwrap()).arg("hold").arg(&path).stdout(Stdio::piped()).spawn().unwrap();
  let mut line=String::new();BufReader::new(child.stdout.take().unwrap()).read_line(&mut line).unwrap();assert_eq!(line.trim(),"LOCKED");
  assert!(matches!(locking::try_lock(&file),Err(std::fs::TryLockError::WouldBlock)));
  child.kill().unwrap();child.wait().unwrap();
  locking::try_lock(&file).unwrap();locking::unlock(&file).unwrap();
 }
 drop(file);std::fs::remove_file(path).unwrap();println!("20 child-kill lock recovery rounds passed");
}
