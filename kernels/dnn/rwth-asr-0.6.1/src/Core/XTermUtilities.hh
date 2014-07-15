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
#ifndef _CORE_XTERM_UTILITIES_HH_
#define _CORE_XTERM_UTILITIES_HH_

#include <iostream>
#include <Core/Types.hh>
/*
  It is probably better to use
  the funcionality defined in e.g.
  curses.h instead of escape
  sequences...
*/


namespace xterm {
    /**
       format
    */
    const char * const normal      = "\033[0m";
    const char * const bold        = "\033[1m";
    const char * const understrike = "\033[4m";
    const char * const blink       = "\033[5m";
    const char * const invert      = "\033[7m";

    /**
       colour
    */
    const char * const black       = "\033[0;30m";
    const char * const red         = "\033[0;31m";
    const char * const green       = "\033[0;32m";
    const char * const brown       = "\033[0;33m";
    const char * const blue        = "\033[0;34m";
    const char * const purple      = "\033[0;35m";
    const char * const cyan        = "\033[0;36m";
    const char * const lightgray   = "\033[0;37m";
    const char * const darkgray    = "\033[1;30m";
    const char * const lightred    = "\033[1;31m";
    const char * const lightgreen  = "\033[1;32m";
    const char * const yellow      = "\033[1;33m";
    const char * const lightblue   = "\033[1;34m";
    const char * const lightpurple = "\033[1;35m";
    const char * const lightcyan   = "\033[1;36m";
    const char * const white       = "\033[1;37m";

    struct Command {
	virtual ~Command() {}
	virtual void write(std::ostream & out) const = 0;
    };

    /**
       cursor movement
       The upper left corner has index (1, 1)
    */
    struct move : public Command {
	u16 row, col;
	move(u16 row = 0, u16 col = 0) : row(row), col(col) {}
	inline void write(std::ostream & out) const { out << "\033[" << row << ";" << col << "H"; }
    };

    struct up : public Command {
	u16 d;
	up(u16 d = 1) : d(d) {}
	inline void write(std::ostream & out) const { out << "\033[" << d << "A"; }
    };
    struct down : public Command {
	u16 d;
	down(u16 d = 1) : d(d) {}
	inline void write(std::ostream & out) const { out << "\033[" << d << "B"; }
    };
    struct right : public Command {
	u16 d;
	right(u16 d = 1) : d(d) {}
	inline void write(std::ostream & out) const { out << "\033[" << d << "C"; }
    };
    struct left : public Command {
	u16 d;
	left(u16 d = 1) : d(d) {}
	inline void write(std::ostream & out) const { out << "\033[" << d << "D"; }
    };

    const char * const save    = "\033[s";
    const char * const restore = "\033[u";

    /**
       other
    */
    const char * const clear = "\033[2J\033[0;0H";
    const char * const kill  = "\033[K";

    struct title : public Command {
	const char * s;
	title(const char * s = "") : s(s) {}
	inline void write(std::ostream & out) const { out << "\033]0;" << s << "\007"; }
    };

}

inline std::ostream & operator<<(std::ostream & out, const xterm::Command & cmd) {
    cmd.write(out);
    return out;
}

#endif // _CORE_XTERM_UTILITIES_HH_
