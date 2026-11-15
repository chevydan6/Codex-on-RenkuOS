use std::io::{self, BufRead, Write};

pub(crate) fn confirm(input: &mut impl BufRead, output: &mut impl Write) -> io::Result<bool> {
    writeln!(output, "WARNING: This RenkuOS version has no sandbox.")?;
    writeln!(output, "Codex commands will run with your user account's permissions and can access or change your files.")?;
    write!(output, "Type yes to continue without a sandbox, or press Enter to exit: ")?;
    output.flush()?;
    let mut answer = String::new();
    input.read_line(&mut answer)?;
    Ok(answer.trim().eq_ignore_ascii_case("yes"))
}

#[cfg(test)]
use std::io::Cursor;

#[cfg(test)]
#[test]
fn requires_explicit_acceptance() {
    let mut output = Vec::new();
    assert!(confirm(&mut Cursor::new(b"yes\n"), &mut output).unwrap());
    let message = String::from_utf8(output).unwrap();
    assert!(message.contains("no sandbox"));
    assert!(message.contains("permissions"));
}

#[cfg(test)]
#[test]
fn rejects_decline_empty_input_and_other_responses() {
    for input in ["no\n", "\n", "", "y\n", "yes please\n"] {
        assert!(!confirm(&mut Cursor::new(input), &mut Vec::new()).unwrap());
    }
}
