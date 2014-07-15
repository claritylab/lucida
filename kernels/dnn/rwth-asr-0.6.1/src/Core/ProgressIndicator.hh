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
// $Id: ProgressIndicator.hh 9235 2013-11-29 21:09:57Z golik $

#ifndef _CORE_PROGRESSINDICATOR_HH
#define _CORE_PROGRESSINDICATOR_HH

#include <ctime>
#include <unistd.h> // for ssize_t
#include <Core/Types.hh>

namespace Core {

    /** Provide assurance to impatient users.
     *
     * A progress indicators provides a sign of activity during
     * lengthy operations.  The only purpose is to reassure the user
     * that the machine is still working and will eventually complete.
     *
     * This class has been designed for minimal performance penalty.
     * Unlike simple-minded printf("%d\r", i) approaches, you may
     * safely use ProgressIndicator::notify() from the inner-most loop
     * of a highly compute intensive algorithm.
     *
     * Usage:
     *
     * Usually you will have some way to measure the amount of work
     * done. E.g. "bytes transferred", "time frames processed", "lines
     * read".  Ideally you know the total number of these "work
     * units" in advance, or have an upper bound which will be lowered
     * later on.
     * -# Create an instance of ProgressIndicator.
     * -# Call start().  If known, provide the total number units to
     * process.
     * -# Call notify() with the current number of units processed
     * each time you come around. If necessary you can also update the
     * total.  If you have no idea how much work has been done, just
     * call notify without arguments.
     * -# When your task is complete, call finish().  This will hide
     * the progress indicator.
     *
     * Limitations:
     * - Currently there can be at most one progress indicator active
     *   at any time.
     * - There are no provisions for thread safety.
     *
     * TODO:
     * - Should detect if cout is connected to a TTY, and suppress
     *   messages otherwise.
     * - see Limitations
     */

    class ProgressIndicator {
    public:
	enum Alignment { Left, Right };

    private:
	static const int defaultLength;
	static const int fd = 2;
	Alignment align_;
	std::string task_, unit_;
	u32 tick_, done_, total_, doneAtLastUpdate_, totalAtLastUpdate_;
	time_t startTime_;
	bool isVisible_;
	class DrawBuffer;
	DrawBuffer *draw;
	ssize_t write_return_val_;

	void hide();
	void print();
	void update();
	void updateSize();

	static ProgressIndicator *activeInstance;
	static void sigAlarmHandler(int);
	static void sigStopContinueHandler(int);
	static void sigWinchHandler(int);
	void activate();
	void deactivate();
	static const float updateInterval; // seconds
	bool isEnabled() const;
	bool isInForeground() const;
	int windowSize();
    public:
	/**
	 * @param task a terse description of the operation, which is
	 * displayed to the left of the progress indicator.
	 *
	 * @param unit a optional name of the unit progress is
	 * measured in (e.g. bytes, lines, states, nodes...)
	 **/
	ProgressIndicator(const std::string &task, const std::string &unit = "");
	~ProgressIndicator();

	/**
	 * Indicate start of the operation.
	 * @param total (estimated) amount of work to do.
	 */
	void start(u32 total = 0);

	/**
	 * Change task description.
	 * @param task a terse description of the operation, which is
	 * @param align indicate whether description should be left or right aligned
	 * displayed to the left of the progress indicator.
	 */
	void setTask(const std::string&, Alignment align = Left);

	/**
	 * Update estimate of total work.
	 */
	void setTotal(u32 total) {
	    if (total > total_) total_ = total;
	}

	/**
	 * Indicate the current state of operation.
	 * You may call notify() very frequently, e.g. from your inner
	 * loop. The run-time penalty of notify() is very small, since
	 * actual updates of the indicator (involving output) are done
	 * only a few times per second.
	 */
	void notify(u32 done) {
	    if (done > done_) done_ = done;
	}

	/**
	 * Indicate the current state of operation and update estimate
	 * of total work.  This is similar to notify(done).
	 */
	void notify(u32 done, u32 total) {
	    setTotal(total);
	    notify(done);
	}

	/**
	 * Indicate that the operation is going on.
	 */
	void notify() {
	    notify(done_ + 1);
	}

	/**
	 * Indicate completion of the operation.
	 * @param hide, if true (default) the progress indicator is
	 * erased, otherwise it remains visible.
	 */
	void finish(bool hide = true);
    };

    class WithProgressIndicator {
    private:
	mutable ProgressIndicator progress_;
    public:
	WithProgressIndicator(const std::string &task = "", const std::string &unit = "") : progress_(task, unit) {}
	void setTask(const std::string &task, ProgressIndicator::Alignment align) const { progress_.setTask(task, align); }
	void start(u32 total = 0) const { progress_.start(total); }
	void setTotal(u32 total) const { progress_.setTotal(total); }
	void notify(u32 done) const { progress_.notify(done); }
	void notify(u32 done, u32 total) const { progress_.notify(done, total); }
	void notify() const { progress_.notify(); }
	void finish(bool hide = true) const { progress_.finish(hide); }
    };

    class WithoutProgressIndicator {
    public:
	WithoutProgressIndicator(const std::string &task = "", const std::string &unit = "") {}
	void setTask(const std::string &task, ProgressIndicator::Alignment align) const {}
	void start(u32 total = 0) const {}
	void setTotal(u32 total) const {}
	void notify(u32 done) const {}
	void notify(u32 done, u32 total) const {}
	void notify() const {}
	void finish(bool hide = true) const {}
    };

    //    const float ProgressIndicator::updateInterval = 0.2;


} // namespace Core

#endif //_CORE_PROGRESSINDICATOR_HH
