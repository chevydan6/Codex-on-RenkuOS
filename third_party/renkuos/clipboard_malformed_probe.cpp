#include <Application.h>
#include <Message.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
extern "C" int codex_haiku_bitmap_png(const BMessage*,void**,size_t*);
int main() {
 BApplication app("application/x-vnd.codex-malformed-clipboard");
 const char* keys[]={"_frame","_cspace","_bmflags","_rowbytes","_data","class","_area","_server_token"};
 uint32_t x=0x12345678;
 for(int n=0;n<1000;n++) {
  BMessage archive,clipboard;
  for(int j=0;j<8;j++) {x=x*1664525u+1013904223u;switch(x%4){case 0:archive.AddInt32(keys[j],(int32)x);break;case 1:archive.AddString(keys[j],"malformed");break;case 2:archive.AddBool(keys[j],(x&1)!=0);break;case 3:break;}}
  clipboard.AddMessage("image/bitmap",&archive);
  void* data=nullptr;size_t len=0;int result=codex_haiku_bitmap_png(&clipboard,&data,&len);
  if(result==0 && (!data || !len)){std::fprintf(stderr,"invalid success at %d\n",n);return 2;}
  std::free(data);
 }
 std::puts("1000 malformed bitmap archives handled");return 0;
}
