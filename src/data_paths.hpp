// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <mpq.hpp>
#include <fstream>
namespace d2mini {
inline d2d::mpq::Stack open_data(const std::filesystem::path& dir, const std::filesystem::path& patch) {
    d2d::mpq::Stack stack; stack.push_installer(patch);
    for (const char* name : {"d2exp.mpq","d2data.mpq","d2char.mpq"}) {
        const auto direct=dir/name;
        if(std::filesystem::is_regular_file(direct)){stack.push(direct);continue;}
        std::ifstream manifest(dir/"d2-discs.tsv");std::string line;bool found=false;
        while(std::getline(manifest,line)){
            const auto tab=line.find('\t');
            if(line.substr(0,tab)==name){stack.push(line.substr(tab+1));found=true;break;}
        }
        if(!found)throw std::runtime_error(std::string("Missing ")+name);
    }
    return stack;
}
}
