// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "CodeGenerator.hh"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif
using namespace Core;


void CodeGenerator::grow(const size_t n) {
    n_ += n;
    code_ = (u8*)realloc(code_, n_);
}

bool CodeGenerator::append(const u8 *new_code, const size_t n) {
    if (finalized_) return false;
    int o = n_;
    grow(n);
    for (size_t i = 0; i < n; ++i) (code_ + o)[i] = new_code[i];
    return true;
}

bool CodeGenerator::append(const std::vector<u8> &new_code) {
    return append(&new_code[0], new_code.size());
}

bool CodeGenerator::appendDWordOffset(unsigned int offset) {
    /* split offset in bytes, reverse order */
    bool ok = true;
    for (u8 i = 0; i <= 24; i+= 8) {
	u8 c = (offset >> i) & 0xff;
	append(&c, 1) && ok;
    }
    return ok;
}


void CodeGenerator::finalize() {
    Message msg(log());
    msg << "generated " << u32(n_) << " bytes of code." << "\n";

    size_t pagesize = getpagesize();
    size_t pagesNeeded = (n_ + pagesize - 1) / pagesize;
    size_t bytesNeeded = pagesNeeded * pagesize;

    void *page = mmap(
	0, bytesNeeded,
	PROT_EXEC | PROT_READ | PROT_WRITE,
	MAP_PRIVATE | MAP_ANONYMOUS,
	-1, 0);
    hope(page != MAP_FAILED);
    memcpy(page, code_, n_);
    free(code_);
    code_ = (u8*) page;
    n_ = bytesNeeded;
    finalized_ = true;

    msg << "aligned code to address " << (void*) code_
	<< " (page size is " << u32(pagesize) << ").";
}

CodeGenerator::CodeGenerator(const Configuration &c) :
    Component(c),
    finalized_(false), n_(0), code_(0)
{}

CodeGenerator::~CodeGenerator() {
    if (code_) {
	if (finalized_)
	    munmap(code_, n_);
	else
	    free(code_);
    }
}
