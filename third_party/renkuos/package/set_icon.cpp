#include <Application.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <NodeInfo.h>
#include <TranslationUtils.h>
#include <View.h>
#include <cstdio>
#include <initializer_list>
int main(int argc,char**argv){
 if(argc<3)return 2;BApplication app("application/x-vnd.renkuos-codex-icon-tool");
 BBitmap* source=BTranslationUtils::GetBitmap(argv[1]);if(!source)return 3;
 for(int a=2;a<argc;a++){
  BFile file(argv[a],B_READ_WRITE);BNodeInfo node(&file);if(file.InitCheck()!=B_OK)return 4;
  for(int size: {16,32}){
   BBitmap bitmap(BRect(0,0,size-1,size-1),B_BITMAP_ACCEPTS_VIEWS,B_CMAP8);
   if(bitmap.InitCheck()!=B_OK||!bitmap.Lock())return 5;
   BView* view=new BView(bitmap.Bounds(),"icon",B_FOLLOW_NONE,0);bitmap.AddChild(view);
   view->SetHighColor(B_TRANSPARENT_COLOR);view->FillRect(bitmap.Bounds());
   view->SetDrawingMode(B_OP_COPY);view->DrawBitmap(source,source->Bounds(),bitmap.Bounds());view->Sync();bitmap.Unlock();
   if(node.SetIcon(&bitmap,size==16?B_MINI_ICON:B_LARGE_ICON)!=B_OK)return 6;
  }
 }
 delete source;return 0;
}
