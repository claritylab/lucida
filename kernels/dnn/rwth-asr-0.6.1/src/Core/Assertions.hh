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
// $Id: Assertions.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _ASSERTIONS_H
#define _ASSERTIONS_H

#include <sys/cdefs.h>
#include <iostream>
#ifdef __SUNPRO_CC
#define __PRETTY_FUNCTION__ "UNKNOWN"
#endif

/**
 * @page DesignByContract Design by Contract
 * @see Assertions.hh
 *
 * @subsection faq Two FAQs for the extremely impacient:
 *
 * - Q: I get a precondition violation in somebody else's code. Why?
 *   A: A precondition violation is caused by a require() statement.
 *   The meaning of such a statement line require(foo) is: The
 *   function must not be called unless the expression "foo" is
 *   true.  The caller is obliged to satisfy "foo", the callee may
 *   rely on this.  So a precondition violation means, there is a
 *   bug in your (or some intermediate) code.  Find it and make sure
 *   "foo" holds.
 *
 * - Q: I get a postcondition or assertion violation in somebody
 *   else's code. Why?
 *   A: The kind of violations are caused by
 *   ensure(), verify() and defect() statements.  They all state
 *   some proposition by the author about the correct functioning of
 *   his code.  If any of them fails, the author has made an error
 *   and it's time to fix this.
 *
 * @subsection dbc Design by Contract
 * Now to some more thorough explanation:
 * (Most of the following material is based on chapter 11 of "Object
 * Oriented Software Construction, 2nd ed." by Bertrand Meyer, which
 * is quite rewarding to read.)
 *
 * @subsubsection bugs What is a bug?
 *
 * - An @em error is a wrong decision made by the developer or user.  An
 *   error during development leads to a defect (see below).  An error
 *   by the user leads to a failure.
 *
 * - A @em defect (often called bug) is a property of the program code,
 *   causing the executable to deviate from its intended behaviour
 *   (so-called failure).
 *
 * - A @em failure (aka runtime error) is an event of the program
 *   deviating from its intended behaviour.  Failures may be caused by
 *   defects, user errors or exceptional events (e.g. disk full).
 *
 * - An @em assertions is a proposition about the correct behaviour of
 *   the programme.  If an assertions does not hold this indicates the
 *   presence of a defect.  It is wrong to use assertions to state
 *   anything which is not knowable at compile time.
 *
 * @subsubsection prepost Preconditions and Postconditions
 *
 * The basic idea is to view the relation between classes as a
 * client-supplier relationship.  The interface of a function or
 * class is a contract between these parties, which states their
 * mutual obligations.  The obligations of the client, i.e. the
 * caller, are called preconditions; they must be satisfied before the
 * function is called.  The supplier, i.e. the called function,
 * benefits from its preconditions, because it may rely on them.  The
 * obligations of the callee are called postconditions; they must be
 * fulfilled when the function returns.
 *
 * Preconditions and postconditions are two types of assertions.  If
 * under any circumstances an assertion does not hold, this indicates
 * a defect.  Therefore it is very useful to test all assertions
 * during program execution.  To declare a precondition, use the
 * require() macro; to declare a postcondition, use ensure().  Other
 * assertions can be declared using verify().  Conceptually pre- and
 * postconditions are part of the functions interface, but in C++ we
 * can only embed them in the function code.  Function argument types
 * and return types can also be considered pre- and postconditions
 * respectively, but they are checked statically by the compiler.
 * require(), ensure() and verify() all take a boolean expression as
 * argument and abort execution immediatelly with an error message,
 * when it evaluates to false.
 *
 * @warning Remember that assertions must NEVER have SIDE-EFFECTS.
 *
 * The use of assertions resembles a formal proof of correctness:
 * Preconditions (require()) correspond to the presumptions, verify()
 * assertions denote the intermediate steps of the proof, and
 * postconditions (ensure()) state the proposition to be proved.
 *
 * @subsection performance Performance Issues
 *
 * In an ideal world where computers are infinitely fast, we would
 * check all assertions all the time.  However in the real world we
 * cannot afford to do that.  But not checking them would defeat their
 * purpose and their benefits (especially quick detection and spotting
 * of errors).  As D. Knuth (supposedly) put it: "Switching off checks
 * in a production version, is like wearing a life jacket on the
 * shore, and leaving it behind when going to the sea."  In consequence
 * Sprint adopts a three way build system:
 * - the release verion checks no assertions at all.  Maximum speed,
 * maximum risk
 * - the standard version checks most assertions, except for those incurring
 * a significant performance penalty.  High speed, moderate risk.
 * - the debug version checks all assertions all the time, but may be
 * horribly slow.  No risk, no fun.
 *
 * Normally you should use the normal require(), ensure() and verify()
 * macros.  If you find, that they slow down you program significantly
 * (by more than 1% as rule of thumb), use the underscore versions
 * require_(), ensure_() and verify_(), which are only checked in the
 * debug version.
 **/

/**
 * @file Assertions.hh
 * Design by Contract support functions.
 * @see @ref DesignByContract
 */

namespace AssertionsPrivate {

    void stackTrace(std::ostream &os, int cutoff = 0);

    /**
     * prints an error message an aborts the program.
     * similar to assertionFailed() but allows to add verbose information
     * (not limited to strings).
     */
    struct FailedAssertion
    {
	FailedAssertion(const char *type,
			const char *expr,
			const char *function,
			const char *filename,
			unsigned int line);
	~FailedAssertion() __attribute__((noreturn));
	std::ostream& stream() const { return std::cerr; }
    };

    /*
     * print an error message for a violated binary relation assertion.
     * Calls to FailedAssertion are wrapped in a (non-inline) function such that
     * the instructions for printing the operands (x, y) are not inserted
     * everywhere verify_op is used.
     */
    template<class S, class T>
    void assertionFailedVerbose(const S &x, const T &y, const char *op,
				const char *expr, const char *function,
				const char *filename, unsigned int line)
	 __attribute__ ((noreturn)) __attribute__((noinline));
    template<class S, class T>
    void assertionFailedVerbose(const S &x, const T &y, const char *op,
				const char *expr, const char *function,
				const char *filename, unsigned int line)
    {
	FailedAssertion("assertion", expr, function, filename, line).stream()
		    << x << " " << op << " " << y;
    }


    /**
     * print assertion error message.
     * @todo consider using FailedAssertion instead in order to remove
     * duplicated code.
     */
    void assertionFailed(const char *type,
			 const char *expr,
			 const char *function,
			 const char *filename,
			 unsigned int line)
	__attribute__ ((noreturn));

    void hopeDisappointed(const char *expr,
			  const char *function,
			  const char *filename,
			  unsigned int line)
	__attribute__ ((noreturn));

}

#if !defined(RELEASE)
/**
 * Check precondition.   Abort if @c expr is false.
 *
 * Use require() to state requirements you impose on the value of the
 * functions argument or on the state of objects when the function is
 * called.  The caller is the obliged take care of this and fulfill
 * all requirements.  Your function should not test for any these
 * preconditions anymore.
 * @warning @c expr must not have side-effects.
 * If your function is very short, inlined and called frequently,
 * you may consider to use require_() instead.
 * @see @ref DesignByContract
 */
#define require(expr)							\
    ((expr) ? ((void) 0) : AssertionsPrivate::assertionFailed		\
     ("precondition", __STRING(expr), __PRETTY_FUNCTION__, __FILE__, __LINE__))

// Internal macro used by require_{eq,ne,lt,gt,le,ge,null,notnull}
#define _require_rel(x, y, r, n) \
	(((x) r (y)) ? ((void) 0) : AssertionsPrivate::assertionFailedVerbose( \
		x, y, __STRING(r), __STRING(x r y), \
		__PRETTY_FUNCTION__, __FILE__, __LINE__))

#else
#define require(expr) ((void) 0)
#define _require_rel(x, y, r, n) ((void) 0)
#endif

// assertions printing the actual values if not fulfilled.
#define require_eq(x, y) _require_rel(x, y, ==, eq)
#define require_ne(x, y) _require_rel(x, y, !=, ne)
#define require_lt(x, y) _require_rel(x, y, <, lt)
#define require_gt(x, y) _require_rel(x, y, >, gt)
#define require_le(x, y) _require_rel(x, y, <=, le)
#define require_ge(x, y) _require_rel(x, y, >=, ge)
#define require_null(x) _require_rel(x, 0, ==, eq)
#define require_notnull(x) _require_rel(x, 0, !=, ne)



#if !defined(RELEASE)
/**
 * verbose check precodition.   Abort if @c expr is false and print @c comment.
 * @see @ref require(expr)
 */
#define requirec(expr,comment)							\
    ((expr) ? ((void) 0) : AssertionsPrivate::assertionFailed		        \
     ("precondition", __STRING(expr)__STRING(->)__STRING(comment), __PRETTY_FUNCTION__, __FILE__, __LINE__))
#else
#define requirec(expr,comment) ((void) 0)
#endif

#if defined(DEBUG)
/**
 * Check expensive precodition.   Abort if @c expr is false.
 *
 * This is theoretically the same as require(), but is checked only
 * in the debugging version.  Use this only if the time needed for
 * evaluating @c expr is of the same (or larger) order of magnitude as
 * the time required to the execution of the function itself.
 * Otherwise use require().
 * @see @ref DesignByContract
 */
#define require_(expr) require(expr)
#else
#define require_(expr) ((void) 0)
#endif

#if !defined(RELEASE)
/**
 * Check postcondition.  Abort if @c expr is false.
 *
 * Use ensure() to state an assurance about the return value and the
 * state of objects after your function returns.  You are obliged to
 * fulfill these promised.  Any caller may rely on them, if he has
 * satisfied all preconditions.
 * @warning @c expr must not have side-effects.
 * If your function is very short, inlined and called frequently,
 * you may consider to use ensure_() instead.
 * @see @ref DesignByContract
 */
#define ensure(expr)							\
    ((expr) ? ((void) 0) : AssertionsPrivate::assertionFailed		\
     ("postcondition", __STRING(expr), __PRETTY_FUNCTION__,  __FILE__, __LINE__))
#else
#define ensure(expr) ((void) 0)
#endif

#if defined(DEBUG)
/**
 * Check expensive postcondition.  Abort if @c expr is false.
 *
 * This is theoretically the same as ensure(), but is checked only
 * in the debugging version.  Use this only if the time needed for
 * evaluating @c expr is of the same (or larger) order of magnitude as
 * the time required to the execution of the function itself.
 * Otherwise use ensure().
 * @see @ref DesignByContract
 */
#define ensure_(expr) ensure(expr)
#else
#define ensure_(expr) ((void) 0)
#endif

#if !defined(RELEASE)
/**
 * Check assertion.  Abort if @c expr is false.
 *
 * Use verify() to state a proposition about the correctnes of the
 * program.  In principle it must be logically deducible that given
 * the expression is true.  Reason to include verify() statements
 * nevertheless are: a) You might be wrong. b) (You and) Others will
 * find it easier to understand you code.  If the trueness of @c expr
 * depends of the functions arguments, use require() instead.  If it
 * depends on some runtime event, do not use assertions at all.
 * @warning @c expr must not have side-effects.
 * If checking @c expr makes your function run significantly slower,
 * you may consider to use verify_() instead.
 * @see @ref DesignByContract
 */
#define verify(expr)							\
    ((expr) ? ((void) 0) : AssertionsPrivate::assertionFailed		\
     ("assertion", __STRING(expr), __PRETTY_FUNCTION__,__FILE__, __LINE__))

// Internal macro used by verify_{eq,ne,lt,gt,le,ge,null,notnull}
#define _verify_rel(x, y, r, n) \
    (((x) r (y)) ? ((void) 0) : AssertionsPrivate::assertionFailedVerbose( \
    x, y, __STRING(r), __STRING(x r y), \
    __PRETTY_FUNCTION__, __FILE__, __LINE__))

#else
#define verify(expr) ((void) 0)
#define _verify_rel(x, y, r, n) ((void) 0)
#endif

// assertions printing the actual values if not fulfilled.
#define verify_eq(x, y) _verify_rel(x, y, ==, eq)
#define verify_ne(x, y) _verify_rel(x, y, !=, ne)
#define verify_lt(x, y) _verify_rel(x, y, <, lt)
#define verify_gt(x, y) _verify_rel(x, y, >, gt)
#define verify_le(x, y) _verify_rel(x, y, <=, le)
#define verify_ge(x, y) _verify_rel(x, y, >=, ge)
#define verify_null(x) _verify_rel(x, 0, ==, eq)
#define verify_notnull(x) _verify_rel(x, 0, !=, ne)


#if defined(DEBUG)
/**
 * Check expensive assertion.  Abort if @c expr is false.
 *
 * This is theoretically the same as verify(), but is checked only
 * in the debugging version.  Use this only if evaluating @c expr
 * slows your function down significantly. Otherwise use verify().
 * @see @ref DesignByContract
 */
#define verify_(expr) verify(expr)
#else
#define verify_(expr) ((void) 0)
#endif



#if !defined(RELEASE)
/**
 * Program has a defect.  Abort immediatelly.
 *
 * This is a shorthand for verify(false).  Use defect() for points in
 * the code which cannot be reached in a correct program, like some
 * default clauses in a switch statement.
 * @see @ref DesignByContract
 */
#define defect()                                                 \
    AssertionsPrivate::assertionFailed("control flow assertion", \
	"", __PRETTY_FUNCTION__,__FILE__, __LINE__)
#else
#define defect()      ((void) 0)
#endif


// #if defined(DEBUG)
/**
 * Check polymorphic type precodition.
 *
 * This performs a down-cast to type @c T.  In the debug version the
 * correctness of this cast is checked and an assertion violation is
 * reported when not.
 * @param T pointer type to derived class.
 * @param s pointer to object of super class of T which is actually y of type T.
 * @return @c s converted to type @c T (if possible)
 * Discussion: Due to C++ adhering to the contravariance principle,
 * many polymorphic functions need to down-cast their arguments.  In
 * such cases it can be considered a pre-condition, that this cast
 * must not fail.  It is preferrable to use required_cast() rather
 * than @c dynamic_cast or @c static_cast in such cases, since @c
 * dynamic_cast has considerable run-time over-head and @c static_cast
 * is unsafe.
 *
 * @warning To avoid compile errors, the functions above are commented
 * out. The required cast behaves now like a "normal" static cast.
 */

/*
  #define required_cast(T, o)                                         \
    ( dynamic_cast<T>(o)                                            \
    ? static_cast<T>(o)                                             \
    : (AssertionsPrivate::assertionFailed(                          \
	"type cast assertion", #o " -> " #T,                        \
	__PRETTY_FUNCTION__,  __FILE__, __LINE__), (T)0) )
  #else
*/
#define required_cast(T, o)                                         \
    static_cast<T>(o)
// #endif

/**
 * Test an expression and abort program if it is false.
 * @warning DO NOT USE THIS!
 *
 * hope() is similar to verify(), but the test will be executed in
 * standard, debug and release versions regardless.  Use hope() instead of
 * verify() when there is no logically infallible argument for the
 * expresion being true, but you have good reason to believe that the
 * contrary will hardly ever occur.  The use of hope() is discouraged,
 * you should write proper runtime error handling.  But during
 * development or for very obscure sources of failure, it is
 * admissible to just hope for the best.  It is better to catch an
 * error using hope() than to let is pass unnoticed, because there was
 * no time to do better.
 * @see verify
 * @see @ref DesignByContract
 */

#define hope(expr)							\
    ((expr) ? (void) 0 : AssertionsPrivate::hopeDisappointed		\
     (__STRING(expr), __PRETTY_FUNCTION__,__FILE__, __LINE__))

#endif // _ASSERTIONS_H
