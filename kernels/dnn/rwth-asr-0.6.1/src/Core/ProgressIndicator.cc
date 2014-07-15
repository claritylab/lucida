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
// $Id: ProgressIndicator.cc 9621 2014-05-13 17:35:55Z golik $

#include "ProgressIndicator.hh"

#include <cmath>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <Core/Assertions.hh>
#include <Core/Channel.hh>
#include <Core/Debug.hh>

using namespace Core;

/*
Examples from other programs:

ssh:
train-full.corpus     17% |****                         |  2692 KB    28:55 ETA
train-full.corpus    100% |*****************************| 15670 KB    33:51



0    0    1    1    2    2    3    3    4    4    5    5    6    6    7    7    8
0....5....0....5....0....5....0....5....0....5....0....5....0....5....0....5....0



old (static) Sprint format
Bar display (with total)

building state tree       90% [=========================== ]   1234567 states   ~  5min
building look-ahead       33% [====================        ]    123456 nodes    ~   53s
------------24---------- ----  --------------28------------ ----10---- ---7---  ----9----
Task                     pcnt  Bar                          Done       Unit     ETA

Spinner display (no total)

loading counts                [/]          6789 events
------------24----------           ----10------ ---7---
Task                               Done         unit

0                        C    A                            B D         E       K      L
building state tree       90% [=========================== ]   1234567 states  ~  5min
building look-ahead       33% [====================        ]    123456 nodes   ~   53s
------------24---------- ----  --------------28------------  ----9---- ---7--- ---7---
Task                     pcnt  Bar                           Done       Unit    ETA

*/

const float ProgressIndicator::updateInterval = 0.2; // seconds
const int ProgressIndicator::defaultLength = 90;
ProgressIndicator *ProgressIndicator::activeInstance = 0;

// ===========================================================================
class ProgressIndicator::DrawBuffer {
public:
    static const int maximumLength = 250;
private:
    static const char spinner[];
    int length; /**< number of visible characters */
    char line[maximumLength+2];
    char info[maximumLength+2];

    void clear() {
	memset(line, ' ', length);
	line[length  ] = '\r';
	line[length+1] = '\0';
    }

    void setLength(int l) {
	require(l <= maximumLength);
	length = l;
	clear();
    }

    void putPercentage(int pos, f32 percentage) {
	int z = snprintf(info, length, "%3.0f%%", percentage);
	strncpy(line + pos, info, std::min(5, std::max(0, z)));
    }

    void putBar(int start, int end, f32 percentage) {
	line[start] = '['; line[end] = ']';
	int pos = start + 1 + int(roundf(f32(end-start-1) / 100.0 * percentage));
	for (int i = start + 1; i < end; ++i)
	    line[i] = (i < pos) ? '=' : ' ';
    }

    void putItems(int pos, u32 done, const std::string &unit) {
	int z = snprintf(info, length, "%10d %s", done, unit.c_str());
	strncpy(line + pos, info, std::min(length - pos , std::max(0, z)));
    }

public:
    DrawBuffer(int l) {
	setLength(l);
    }

    void resize(int l) {
	setLength(l);
    }

    void drawBar(const std::string &task, ProgressIndicator::Alignment align,
		 const std::string &unit, u32 done, f32 percentage, f32 eta);
    void drawSpinner(const std::string &task, const std::string &unit, u32 done, u32 tick);
    ssize_t write(int fd) const {
	return ::write(fd, line, length+2);
    }
};

void ProgressIndicator::DrawBuffer::drawBar(
    const std::string &task, ProgressIndicator::Alignment align, const std::string &unit,
    u32 done, f32 percentage, f32 eta)
{
    int etaPos   = length - 7;
    int unitPos  = etaPos - 1 - std::min(int(unit.length()), length / 10);
    int itemsPos = unitPos - 1 - 10;
    int barEnd   = itemsPos - 2;
    int percentagePos = std::min(std::max(int(task.length()), length / 5), length / 3) + 1;
    int barStart = percentagePos + 5;

    clear();

    switch (align) {
    case Left:
	strncpy(line, task.data(), std::min(int(task.length()), percentagePos - 1));
	break;
    case Right:
	strncpy(line, task.data() + std::max(0, int(task.length()) - percentagePos - 1),
		std::min(size_t(percentagePos - 1), task.length()));
	break;
    default: defect();
    }
    putPercentage(percentagePos, percentage);
    putBar(barStart, barEnd, percentage);
    putItems(itemsPos, done, unit);

    if (eta) {
	int z;
	if (eta < 60.0) {
	    z = snprintf(info, length, "~ %4.0fs  ", eta);
	} else if (eta < 3600.0) {
	    z = snprintf(info, length, "~ %2.0fmin  ", eta / 60.0);
	} else if (eta < 36000) {
	    z = snprintf(info, length, "~ %2.1fh  ", eta / 3600.0);
	} else {
	    z = snprintf(info, length, "~ %4.0fh  ", eta / 3600.0);
	}
	strncpy(line + etaPos, info, std::min(length - etaPos, std::max(0, z)));
    }
}

const char ProgressIndicator::DrawBuffer::spinner[] = "-\\|/";

void ProgressIndicator::DrawBuffer::drawSpinner(
    const std::string &task, const std::string &unit, u32 done, u32 tick)
{
    int spinnerPos = 2 * length / 3;
    clear();
    strncpy(line, task.data(), std::min(int(task.length()), spinnerPos - 1));
    line[spinnerPos]   = '[';
    line[spinnerPos+1] = spinner[tick % (sizeof(spinner)-1)];
    line[spinnerPos+2] = ']';
    putItems(spinnerPos + 4, done, unit);
}


// ===========================================================================
void ProgressIndicator::sigAlarmHandler(int) {
    if (activeInstance) activeInstance->update();
}

/* It would be better not to start the timer if we continue in
 * background, but unfortunatelly the shell won't signal us when we
 * get back to foreground.
 *
 * It would also be nice, if the time we are suspended is not taken
 * into account for estimating the remaining time.
 */

void ProgressIndicator::sigStopContinueHandler(int sig) {
    if (!activeInstance) return;
    switch (sig) {
    case SIGTSTP:
	activeInstance->deactivate();
	activeInstance->hide();
	signal(SIGCONT, sigStopContinueHandler);
	raise(sig);
	break;
    case SIGCONT:
	signal(SIGCONT, SIG_DFL);
	activeInstance->activate();
	break;
    }
}

void ProgressIndicator::sigWinchHandler(int sig) {
    if (activeInstance) activeInstance->updateSize();
}

ProgressIndicator::ProgressIndicator(
    const std::string &task,
    const std::string &unit) :
    align_(Left), task_(task), unit_(unit),
    isVisible_(false), draw(0), write_return_val_(0)
{}

ProgressIndicator::~ProgressIndicator() {
    if (activeInstance == this)
	finish();
    delete draw;
}

void ProgressIndicator::setTask(const std::string &t, Alignment align) {
    align_ = align;
    task_ = t;
}

bool ProgressIndicator::isEnabled() const {
    return (isatty(fd) == 1);
}

bool ProgressIndicator::isInForeground() const {
    return (tcgetpgrp(fd) == getpgrp());
}

int ProgressIndicator::windowSize() {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws))
	return defaultLength;
    if (ws.ws_col == 0)
	return defaultLength;
    if (ws.ws_col - 1 >= DrawBuffer::maximumLength)
	return DrawBuffer::maximumLength;
    return ws.ws_col - 1;
}

void ProgressIndicator::start(u32 total) {
    require(activeInstance != this);
    tick_ = done_ = doneAtLastUpdate_ = 0;
    total_ = total;
    startTime_ = time(0);

    if (!isEnabled() || activeInstance) return;

    activeInstance = this;
    Core::Channel::Manager::us()->blockTty();
    if (!draw) draw = new DrawBuffer(windowSize());

    // Note: An active progress bar
    // prevents GDB from stepping through the program code. GDB stops correctly at the breakpoints,
    // but if you use the "step" or "next" command afterwards, the timer signal lets GDB jump into
    // the sigAlarmHandler method instead of the desired program line. For that reason, the progress
    // indicator is deactivated in debug mode.
#ifndef DEBUG
    if(!AmIBeingDebugged())
    activate();
#endif
}

void ProgressIndicator::activate() {
    signal(SIGTSTP, sigStopContinueHandler);
    signal(SIGALRM, sigAlarmHandler);
    signal(SIGWINCH, sigWinchHandler);
    struct itimerval tv;
    tv.it_interval.tv_sec  = (long int)(floor(updateInterval));
    tv.it_interval.tv_usec = (long int)(1000000.0 * (updateInterval - floor(updateInterval)));
    tv.it_value = tv.it_interval;
    setitimer(ITIMER_REAL, &tv, 0);
}

void ProgressIndicator::deactivate() {
    struct itimerval tv;
    tv.it_interval.tv_sec = tv.it_interval.tv_usec = 0;
    tv.it_value = tv.it_interval;
    setitimer(ITIMER_REAL, &tv, 0);
    signal(SIGALRM, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGWINCH, SIG_DFL);
}

void ProgressIndicator::update() {
    if (!isVisible_ || done_ != doneAtLastUpdate_ || total_ != totalAtLastUpdate_) {
	if (!isInForeground()) return;
	if (done_ != doneAtLastUpdate_) ++tick_;
	doneAtLastUpdate_ = done_;
	totalAtLastUpdate_ = total_;
	print();
    }
}

void ProgressIndicator::updateSize() {
    draw->resize(windowSize());
}

void ProgressIndicator::finish(bool shouldHide) {
    if (activeInstance == this) {
	deactivate();
	if (shouldHide) {
	    hide();
	} else if (isInForeground()) {
	    if (total_) total_ = done_;
	    print();
	    write_return_val_ = write(fd, "\n", 1);
	    isVisible_ = false;
	}
	Core::Channel::Manager::us()->unblockTty();
	activeInstance = 0;
    }
}

void ProgressIndicator::hide() {
    if (isVisible_) {
	write_return_val_ = write(fd, "\033[0K", 4);
	isVisible_ = false;
    }
}

void ProgressIndicator::print() {
    require(activeInstance == this);
    if (total_) {
	f32 percentage = 100.0 * f32(done_) / f32(total_);
	f32 eta = 0.0;
	if (done_ && done_ < total_)
	    eta = f32(time(0) - startTime_) / f32(done_) * f32(total_ - done_);
	draw->drawBar(task_, align_, unit_, done_, percentage, eta);
    } else {
	draw->drawSpinner(task_, unit_, done_, tick_);
    }
    draw->write(fd);
    isVisible_ = true;
}
