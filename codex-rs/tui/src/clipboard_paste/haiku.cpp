#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <cstdlib>
#include <cstring>
#include <memory>

static int translate_png(BPositionIO* input, void** bytes, size_t* length)
{
    BMallocIO output;
    BTranslatorRoster* roster = BTranslatorRoster::Default();
    if (roster == nullptr || roster->Translate(input, nullptr, nullptr, &output,
            B_PNG_FORMAT) != B_OK)
        return 2;
    size_t size = output.BufferLength();
    void* copy = std::malloc(size);
    if (copy == nullptr)
        return 2;
    std::memcpy(copy, output.Buffer(), size);
    *bytes = copy;
    *length = size;
    return 0;
}

// The caller owns the returned buffer and must release it with free().
extern "C" int codex_haiku_bitmap_png(const BMessage* data, void** bytes, size_t* length)
{
    *bytes = nullptr;
    *length = 0;
    // Tracker stores copied files as entry references. Prefer the original file
    // to any bitmap preview, matching the other desktop clipboard backends.
    char* name;
    type_code type;
    int32 count;
    for (int32 field = 0; data->GetInfo(B_REF_TYPE, field, &name, &type, &count) == B_OK; ++field) {
        for (int32 index = 0; index < count; ++index) {
            entry_ref ref;
            if (data->FindRef(name, index, &ref) != B_OK)
                continue;
            BEntry entry(&ref, true);
            if (entry.InitCheck() != B_OK || !entry.IsFile())
                continue;
            BFile file(&entry, B_READ_ONLY);
            if (file.InitCheck() == B_OK && translate_png(&file, bytes, length) == 0)
                return 0;
        }
    }

    BMessage archive;
    if (data->FindMessage("image/bitmap", &archive) != B_OK)
        return 1;
    BBitmap* bitmap = new BBitmap(&archive);
    if (bitmap->InitCheck() != B_OK) {
        delete bitmap;
        return 2;
    }
    BBitmapStream input(bitmap); // Takes ownership of bitmap.
    return translate_png(&input, bytes, length);
}

extern "C" int codex_haiku_clipboard_png(void** bytes, size_t* length)
{
    *bytes = nullptr;
    *length = 0;
    std::unique_ptr<BApplication> app;
    if (be_app == nullptr) {
        app.reset(new BApplication("application/x-vnd.openai-codex-clipboard"));
        if (app->InitCheck() != B_OK)
            return 2;
    }
    if (be_clipboard == nullptr || !be_clipboard->Lock())
        return 2;
    const BMessage* clipboardData = be_clipboard->Data();
    if (clipboardData == nullptr) {
        be_clipboard->Unlock();
        return 2;
    }
    BMessage data(*clipboardData);
    be_clipboard->Unlock();
    return codex_haiku_bitmap_png(&data, bytes, length);
}
