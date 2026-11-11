// SPDX-License-Identifier: GPL-3.0-or-later
#include <mpq.hpp>
#include <future>
#include <cstdio>
#include <array>
void check(bool ok) { if (!ok) throw std::runtime_error("shared MPQ lifetime/concurrency check failed"); }
int main(int argc,char** argv) try {
    check(argc==2); const std::filesystem::path path(argv[1]),second(path.string()+".second");std::filesystem::remove(path);std::filesystem::remove(second);
    std::vector<std::byte> bytes(65539);for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(i%251);
    HANDLE archive{},file{};
    check(SFileCreateArchive(path.c_str(),MPQ_CREATE_ARCHIVE_V1,16,&archive));
    for(const char* name:{"alpha.bin","beta.bin"}) {
        check(SFileCreateFile(archive,name,0,DWORD(bytes.size()),0,MPQ_FILE_COMPRESS,&file));
        check(SFileWriteFile(file,bytes.data(),DWORD(bytes.size()),MPQ_COMPRESSION_ZLIB));
        check(SFileFinishFile(file));
    }
    check(SFileCloseArchive(archive));
    check(SFileCreateArchive(second.c_str(),MPQ_CREATE_ARCHIVE_V1,16,&archive));
    check(SFileCreateFile(archive,"gamma.bin",0,DWORD(bytes.size()),0,MPQ_FILE_COMPRESS,&file));
    check(SFileWriteFile(file,bytes.data(),DWORD(bytes.size()),MPQ_COMPRESSION_ZLIB));
    check(SFileFinishFile(file));check(SFileCloseArchive(archive));
    std::optional<d2d::mpq::File> retained,isolated;
    {
        d2d::mpq::Stack original;original.push(path);original.push(second);
        auto independently_opened=original.reopen();check(independently_opened.read("alpha.bin")==bytes);
        std::array<std::future<void>,4> jobs;
        for(auto& job:jobs)job=std::async(std::launch::async,[view=original.share(),&bytes]()mutable {
            for(int n=0;n<40;++n) {
                check(view.read(n%2?"gamma.bin":"beta.bin")==bytes);
                auto f=view.open(n%2?"gamma.bin":"beta.bin");check(f.has_value()&&f->size()==bytes.size());
                check(f->seek(2051)==2051);std::array<std::byte,2093> sample{};
                check(f->read(sample.data(),sample.size())==sample.size());
                check(std::equal(sample.begin(),sample.end(),bytes.begin()+2051));
            }
        });
        for(auto& job:jobs)job.get();
        auto view=original.share();retained=view.open("alpha.bin");check(retained.has_value());
        isolated=view.open("gamma.bin");check(isolated.has_value());
        check(!view.open("missing.bin"));
    }
    // The file outlives both the original Stack and every shared reader.
    check(retained->size()==bytes.size());std::vector<std::byte> actual(bytes.size());
    check(retained->read(actual.data(),actual.size())==actual.size()&&actual==bytes);
    d2d::mpq::File moved=std::move(*retained);retained.reset();
    check(moved.seek(7)==7);std::array<std::byte,17> part{};check(moved.read(part.data(),part.size())==part.size());
    check(std::equal(part.begin(),part.end(),bytes.begin()+7));
    check(isolated->seek(0)==0);std::vector<std::byte> independent_bytes(bytes.size());
    check(isolated->read(independent_bytes.data(),independent_bytes.size())==bytes.size()&&independent_bytes==bytes);
    isolated.reset();moved=d2d::mpq::File{};std::filesystem::remove(path);std::filesystem::remove(second);
    std::puts("PASS: shared/independent MPQ stacks and archive-locked streams, four concurrent readers, streaming seek/read, retained and moved file lifetime");return 0;
} catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
