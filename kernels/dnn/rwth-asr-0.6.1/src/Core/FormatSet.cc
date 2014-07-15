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
#include "Directory.hh"
#include "FormatSet.hh"

using namespace Core;

const std::string FormatSet::defaultQualifier("_default_");
const std::string FormatSet::qualifierDelimiter(":");

bool FormatSet::checkFile(const std::string &filename, AccessMode mode) const
{
    if (filename.empty()) {
	error("Failed to open file because filename is empty.");
	return false;
    } else if (mode == modeRead) {
	if (!isValidPath(filename)) {
	    error("File '%s' does not exist.", filename.c_str());
	    return false;
	}
    }
    return true;
}

std::string FormatSet::getQualifier(const std::string &filename)
{
    std::string::size_type delimiterPos = filename.find(qualifierDelimiter);
    if (delimiterPos == std::string::npos) return "";
    return filename.substr(0, delimiterPos);
}

std::string FormatSet::stripQualifier(const std::string &filename)
{
    std::string::size_type delimiterPos = filename.find(qualifierDelimiter);
    if (delimiterPos == std::string::npos) return filename;
    return filename.substr(delimiterPos + qualifierDelimiter.size());
}
