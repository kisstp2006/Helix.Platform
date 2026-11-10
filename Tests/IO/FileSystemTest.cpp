#include <Helix/Platform/System/DynamicLibrary.h>
#include <Helix/Platform/IO/FileSystem.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Time/Time.h>
#include <Helix/Platform/Window/Window.h>
#include <Helix/Core/Types/Guid.h>
#include <Helix/Core/Strings/Format.h>
#include <Helix/Testing/Test.h>

#include <atomic>
#include <cstdlib>

#define HX_CHECK_MSG_NO_DISPLAY() HX_CHECK(std::getenv("DISPLAY") == nullptr)

using namespace helix;
using namespace helix::platform;

HX_TEST(FileSystem, WriteReadListRemove)
{
    String dir = JoinPath(TempDirectory(), Format("helix_test_{}", Guid::Generate().ToString()));
    String nested = JoinPath(dir, "a/b");
    HX_REQUIRE(CreateDirectories(nested));
    HX_CHECK(DirectoryExists(nested));
    String file = JoinPath(nested, "data.bin");
    u8 data[300];
    for (int i = 0; i < 300; i++)
        data[i] = u8(i);
    HX_REQUIRE(WriteFile(file, Span<const u8>(data, 300)));
    HX_CHECK(FileExists(file));
    Array<u8> read;
    HX_REQUIRE(ReadFile(file, read));
    HX_CHECK_EQ(read.Size(), 300u);
    HX_CHECK_EQ(read[299], 43);
    {
        File f;
        HX_REQUIRE(f.Open(file, FileMode::Append));
        HX_CHECK_EQ(f.Write("xyz", 3), 3u);
    }
    {
        File f;
        HX_REQUIRE(f.Open(file, FileMode::Read));
        HX_CHECK_EQ(f.Size(), 303);
        HX_CHECK(f.Seek(300));
        char tail[3];
        HX_CHECK_EQ(f.Read(tail, 3), 3u);
        HX_CHECK(StringView(tail, 3) == "xyz");
    }
    int count = 0;
    HX_CHECK(ListDirectory(nested, [](const DirectoryEntry& e, void* user) {
        if (e.name == "data.bin" && !e.isDirectory && e.size == 303)
            ++*static_cast<int*>(user);
    }, &count));
    HX_CHECK_EQ(count, 1);
    String moved = JoinPath(nested, "moved.bin");
    HX_CHECK(RenameFile(file, moved));
    HX_CHECK(!FileExists(file) && FileExists(moved));
    HX_CHECK(RemoveFile(moved));
    HX_CHECK(RemoveEmptyDirectory(nested));
    HX_CHECK(RemoveEmptyDirectory(JoinPath(dir, "a")));
    HX_CHECK(RemoveEmptyDirectory(dir));
    HX_CHECK(!DirectoryExists(dir));
    HX_CHECK(!ExecutablePath().Empty());
}

HX_TEST(FileSystem, Paths)
{
    HX_CHECK(JoinPath("a/", "/b") == "a/b");
    HX_CHECK(JoinPath("a", "b") == "a/b");
    HX_CHECK(GetFileName("x/y/file.tar.gz") == "file.tar.gz");
    HX_CHECK(GetExtension("x/y/file.tar.gz") == ".gz");
    HX_CHECK(GetExtension("x/.hidden").Empty());
    HX_CHECK(GetDirectoryName("x/y/file") == "x/y");
    HX_CHECK(GetDirectoryName("file").Empty());
}
