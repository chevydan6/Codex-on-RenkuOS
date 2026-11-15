#include <unistd.h>

int main()
{
    const pid_t child = fork();
    if (child < 0)
        return 1;
    if (child > 0)
        return 0;
    execl("/boot/system/apps/Terminal", "Terminal", "-t", "Codex on RenkuOS",
        "/bin/bash", "-c",
        "cd /boot/home && /CodexBuild/renkuos-codex-google-login/"
        "renkuos-native-daemon/bin/codex; "
        "printf '\\nPress Enter to close this window.\\n'; read -r",
        static_cast<char*>(nullptr));
    return 1;
}
