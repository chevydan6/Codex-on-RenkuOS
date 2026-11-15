#include <OS.h>
#include <image.h>
#include <unistd.h>
#include <string>
int main() {
 image_info info;int32 cookie=0;std::string cli;
 while(get_next_image_info(B_CURRENT_TEAM,&cookie,&info)==B_OK)
  if(info.type==B_APP_IMAGE){std::string path=info.name;cli=path.substr(0,path.find_last_of('/'))+"/bin/codex";break;}
 if(cli.empty()||access(cli.c_str(),X_OK)!=0)return 1;
 pid_t child=fork();if(child<0)return 1;if(child>0)return 0;
 execl("/boot/system/apps/Terminal","Terminal","-t","Codex on RenkuOS","/bin/bash","-c",
  "cd \"$HOME\" && \"$1\"; printf '\\nPress Enter to close this window.\\n'; read -r",
  "codex-launcher",cli.c_str(),static_cast<char*>(nullptr));return 1;
}
