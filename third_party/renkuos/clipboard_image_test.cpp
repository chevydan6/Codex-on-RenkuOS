// Run on RenkuOS after linking haiku.cpp with libbe and libtranslation.
#include <Application.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <DataIO.h>
#include <Entry.h>
#include <Message.h>
#include <TranslationUtils.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

extern "C" int codex_haiku_bitmap_png(const BMessage*, void**, size_t*);
extern "C" int codex_haiku_clipboard_png(void**, size_t*);

int main()
{
    BApplication app("application/x-vnd.codex-clipboard-test");
    BMessage empty;
    void* bytes = nullptr;
    size_t length = 0;
    assert(codex_haiku_bitmap_png(&empty, &bytes, &length) != 0);
    assert(bytes == nullptr && length == 0);
    BMessage invalidArchive;
    BMessage invalidClipboard;
    assert(invalidClipboard.AddMessage("image/bitmap", &invalidArchive) == B_OK);
    assert(codex_haiku_bitmap_png(&invalidClipboard, &bytes, &length) != 0);
    assert(bytes == nullptr && length == 0);

    BBitmap bitmap(BRect(0, 0, 1, 2), B_RGBA32);
    assert(bitmap.InitCheck() == B_OK);
    std::memset(bitmap.Bits(), 0xff, bitmap.BitsLength());
    BMessage archived;
    assert(bitmap.Archive(&archived, true) == B_OK);
    BMessage clipboard;
    assert(clipboard.AddMessage("image/bitmap", &archived) == B_OK);
    assert(codex_haiku_bitmap_png(&clipboard, &bytes, &length) == 0);
    const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    assert(length > sizeof(signature));
    assert(std::memcmp(bytes, signature, sizeof(signature)) == 0);
    BMemoryIO png(bytes, length);
    BBitmap* decoded = BTranslationUtils::GetBitmap(&png);
    assert(decoded != nullptr);
    assert(decoded->Bounds().IntegerWidth() == 1);
    assert(decoded->Bounds().IntegerHeight() == 2);
    delete decoded;

    // Tracker puts copied files on the clipboard as B_REF_TYPE entries.
    char path[] = "/tmp/codex-clipboard-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(write(fd, bytes, length) == static_cast<ssize_t>(length));
    assert(close(fd) == 0);
    entry_ref ref;
    assert(get_ref_for_path(path, &ref) == B_OK);
    BBitmap preview(BRect(0, 0, 0, 0), B_RGBA32);
    assert(preview.InitCheck() == B_OK);
    BMessage previewArchive;
    assert(preview.Archive(&previewArchive, true) == B_OK);
    BMessage fileClipboard;
    entry_ref directoryRef;
    assert(get_ref_for_path("/tmp", &directoryRef) == B_OK);
    assert(fileClipboard.AddRef("r3_directory", &directoryRef) == B_OK);
    assert(fileClipboard.AddRef("r3_test", &ref) == B_OK);
    assert(fileClipboard.AddMessage("image/bitmap", &previewArchive) == B_OK);
    void* fileBytes = nullptr;
    size_t fileLength = 0;
    assert(codex_haiku_bitmap_png(&fileClipboard, &fileBytes, &fileLength) == 0);
    BMemoryIO filePng(fileBytes, fileLength);
    BBitmap* fileImage = BTranslationUtils::GetBitmap(&filePng);
    assert(fileImage != nullptr);
    assert(fileImage->Bounds().IntegerWidth() == 1);
    assert(fileImage->Bounds().IntegerHeight() == 2);
    delete fileImage;
    std::free(fileBytes);
    assert(unlink(path) == 0);
    fileBytes = nullptr;
    fileLength = 0;
    assert(codex_haiku_bitmap_png(&fileClipboard, &fileBytes, &fileLength) == 0);
    BMemoryIO fallbackPng(fileBytes, fileLength);
    BBitmap* fallbackImage = BTranslationUtils::GetBitmap(&fallbackPng);
    assert(fallbackImage != nullptr);
    assert(fallbackImage->Bounds().IntegerWidth() == 0);
    assert(fallbackImage->Bounds().IntegerHeight() == 0);
    delete fallbackImage;
    std::free(fileBytes);
    std::free(bytes);

    assert(be_clipboard->Lock());
    BMessage original(*be_clipboard->Data());
    assert(be_clipboard->Clear() == B_OK);
    *be_clipboard->Data() = clipboard;
    assert(be_clipboard->Commit() == B_OK);
    be_clipboard->Unlock();
    bytes = nullptr;
    length = 0;
    int result = codex_haiku_clipboard_png(&bytes, &length);
    assert(be_clipboard->Lock());
    assert(be_clipboard->Clear() == B_OK);
    *be_clipboard->Data() = original;
    assert(be_clipboard->Commit() == B_OK);
    be_clipboard->Unlock();
    assert(result == 0);
    assert(length > sizeof(signature));
    assert(std::memcmp(bytes, signature, sizeof(signature)) == 0);
    std::free(bytes);
    return 0;
}
