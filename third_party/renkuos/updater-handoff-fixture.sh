#!/bin/sh
if [ "$4" = "--help" ]; then
    exit 0
fi
printf '%s\n' "$0" "$1" "$2" "$3" "$$" > "$CODEX_TEST_HANDOFF_MARKER"
exit 0
