#ifndef AG_SIMT_H
#define AG_SIMT_H

#include <cstddef>
#include <vector>
#include <algorithm>
#if defined(__arm__) || defined(__AARCH64EL__)
#include <aegaeon-primitives.h>
#endif

#if defined(__arm__)
#define KERNEL_ATTR __attribute__((naked))
#else
#define KERNEL_ATTR __attribute__((noreturn))
#endif

namespace ag {

// Strategised SIMT class
template <typename S>
struct simt {
  // Execute (lambda) function in critical section
  template<class FN> static inline
  void critical(FN func)
  {
    S::critical(func);
  }

  // A reference class that updates the referred value atomically
  template <class T>
  class atomic_ref
  {
    volatile T &a;

  public:
    explicit atomic_ref(volatile T &a) : a(a) {}

    inline operator T() const { return a; }

    inline T operator++() {
      return S::incAndFetch(&a);
    }
    inline T operator++(int) {
      return S::fetchAndInc(&a);
    }
    inline T operator--() {
      return S::decAndFetch(&a);
    }
    inline T operator--(int) {
      return S::fetchAndDec(&a);
    }

    template <typename A>
    inline void operator+=(const A other) {
      S::addAndFetch(&a, other);
    }
  };

  //
  // A class that manages an atomic variable
  //
  template <class T>
  class atomic
  {
    volatile T a;
  public:
    explicit atomic(T init=T())
      : a(init)
    {
    }

    inline operator T() const { return a; }

    inline T operator++() {
      return S::incAndFetch(&a);
    }
    inline T operator++(int) {
      return S::fetchAndInc(&a);
    }
    inline T operator--() {
      return S::decAndFetch(&a);
    }
    inline T operator--(int) {
      return S::fetchAndDec(&a);
    }

    template <typename A>
    inline void operator+=(const A other) {
      S::addAndFetch(&a, other);
    }
  };

  // Temporary atomicity
  template <class T> static inline
  atomic_ref<T> atom(T& a) {
    return atomic_ref<T>(a);
  }

  // Job class to track running job(s)
  typedef typename S::JobId JobId;
  static const JobId InvalidJobId = S::InvalidJobId;

  struct Job
  {
    explicit Job(JobId id=InvalidJobId)
      : m_id(id) {
    }
    Job(Job &&other)
      : m_id(other.m_id)
    {
      other.m_id = InvalidJobId;
    }
    ~Job() {
      wait();
    }
    inline void wait() {
      S::jobwait(m_id);
    }
    Job &operator=(Job other) {
      m_id = other.m_id;
      other.m_id = InvalidJobId;

      return *this;
    }
  private:
    Job(const Job &other) {}
    JobId m_id;
  };

  // 1D loop
  template<class FN> static inline
  Job par_for(unsigned count, FN func)
  {
    return Job(S::par_for(count, func));
  }

  // 1D loop with offset
  template<class FN> static inline
  Job par_for(unsigned count, unsigned offset, FN func)
  {
    return Job(S::par_for(count, offset, func));
  }

  // 2D loop
  template<class FN> static inline
  Job par_for_2D(unsigned outerCount, unsigned innerCount, FN func)
  {
    return Job(S::par_for_2D(outerCount, innerCount, func));
  }

  // 2D loop with offset
  template<class FN> static inline
  Job par_for_2D(unsigned outerCount, unsigned innerCount, unsigned outerOffset, unsigned innerOffset, FN func)
  {
    return Job(S::par_for_2D(outerCount, innerCount, outerOffset, innerOffset, func));
  }

  // 3D loop
  template<class FN> static inline
  Job par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, FN func)
  {
    return Job(S::par_for_3D(zCount, yCount, xCount, func));
  }

  // 3D loop with offset
  template<class FN> static inline
  Job par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, unsigned zOffset, unsigned yOffset, unsigned xOffset, FN func)
  {
    return Job(S::par_for_3D(zCount, yCount, xCount, zOffset, yOffset, zOffset, func));
  }

  //
  // reductions
  //
  // Reduce using a critical section around the reduction step
  template <class FN1, class FN2, typename R> static inline
  R reduce_critical(unsigned count, FN1 func, FN2 reduce, R initial)
  {
    R result = initial;

    par_for(count, [=](unsigned i) {
      FN2 red = reduce;
      R &res = result;

      R temp = func(i);
      critical([&](){
        res = red(res, temp);
      });

    });

    return result;
  }

  // Parallel computation followed by serial reduction
  template <class FN1, class FN2, typename R> static inline
  R reduce_serial(unsigned count, FN1 func, FN2 reduce, R initial)
  {
    // Compute function in parallel into a temporary vector
    R *inter = new R[count+1];
    inter[count] = initial;

    par_for(count, [=](unsigned i) {
      inter[i] = func(i);
    });

    // Reduce temporary vector using scalar code
    unsigned step = count;
    while (step > 1) {
      // Deal with trailing odd entry by reducing with entry[0]
      if (step & 1) {
        inter[0] = reduce(inter[0], inter[step-1]);
      }
      // Reduce remaining results in pairs in parallel
      step = step/2;
      for (unsigned i = 0; i < step; ++i) {
        inter[i] = reduce(inter[i], inter[i+step]);
      }
    }

    const R result = inter[0];

    delete [] inter;

    return result;
  }

  // Parallel computation followed by multiple parallel reductions
  template <class FN1, class FN2, typename R> static inline
  R reduce_parallel(unsigned count, FN1 func, FN2 reduce, R initial)
  {
    // Compute function in parallel, collecting results in a temporary vector
    R *inter = new R[count+1];
    inter[count] = initial;

    par_for(count, [=](unsigned i) {
      inter[i] = func(i);
    });

    // Reduce the results in a series of parallel steps
    unsigned step = count+1;
    while (step > 1) {
      // Deal with trailing odd entry by reducing with entry[0]
      if (step & 1) {
        inter[0] = reduce(inter[0], inter[step-1]);
      }
      // Reduce remaining results in pairs in parallel
      step = step/2;
      par_for(step, [=](unsigned i) {
        inter[i] = reduce(inter[i], inter[i+step]);
      });
    }

    const R result = inter[0];

    delete [] inter;

    return result;
  }

  template <class FN1, class FN2, typename R> static inline
  R reduce(unsigned count, FN1 func, FN2 reduce, R initial)
  {
#define REDUCE_SERIAL
#ifdef REDUCE_CRITICAL
    return reduce_critical(count, func, reduce, initial);
#endif
#ifdef REDUCE_PARALLEL
    return reduce_parallel(count, func, reduce, initial);
#endif
#ifdef REDUCE_SERIAL
    return reduce_serial(count, func, reduce, initial);
#endif
  }
};

// SIMT strategy that uses OMP to implement parallelism
struct simt_strat_omp {
#ifndef __GNUC__
  template <typename T, typename A>
  static T __sync_fetch_and_add(T *pValue, const A arg)
  {
    T res;
#pragma omp critical
    {
      res = *pValue;
      *pValue += arg;
    }

    return res;
  }
  template <typename T, typename A>
  static T __sync_add_and_fetch(T *pValue, const A arg)
  {
    T res;
#pragma omp critical
    {
      *pValue += arg;
      res = *pValue;
    }

    return res;
  }
#endif

  typedef unsigned JobId;
  static const JobId InvalidJobId = (JobId)-1;

  static void jobwait(JobId id) {
  }

  template<class FN> static inline
  void critical(FN func)
  {
#pragma omp critical
    func();
  }

  template <typename T, typename A> static inline
  T addAndFetch(T *pValue, A arg) {
    return __sync_add_and_fetch(pValue, arg);
  }
  template <typename T, typename A> static inline
  T fetchAndAdd(T *pValue, A arg) {
    return __sync_fetch_and_add(pValue, arg);
  }
  template <typename T> static inline
  T incAndFetch(T *pValue) {
    return addAndFetch(pValue, 1);
  }
  template <typename T> static inline
  T decAndFetch(T *pValue) {
    return addAndFetch(pValue, -1);
  }
  template <typename T> static inline
  T fetchAndInc(T *pValue) {
    return fetchAndAdd(pValue, 1);
  }
  template <typename T> static inline
  T fetchAndDec(T *pValue) {
    return fetchAndAdd(pValue, -1);
  }

  // 1D loop
  template<class FN> static inline
  JobId par_for(unsigned count, FN func)
  {
#ifdef _WIN32
#pragma omp parallel for schedule(dynamic, 1)
#else
#pragma omp parallel for num_threads(count) schedule(static, 1)
#endif
    for (int i = 0; i < int(count); ++i) {
      func(unsigned(i));
    }

    return 0;
  }

  // 1D loop with offset
  template<class FN> static inline
  JobId par_for(unsigned count, unsigned offset, FN func)
  {
#ifdef _WIN32
#pragma omp parallel for schedule(dynamic, 1)
#else
#pragma omp parallel for num_threads(count) schedule(static, 1)
#endif
    for (int i = 0; i < int(count); ++i) {
      func(unsigned(i)+offset);
    }

    return 0;
  }

  // 2D loop
  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, FN func)
  {
    return par_for(outerCount*innerCount, [innerCount, func](unsigned i) {
      func(i / innerCount, i % innerCount);
    });
  }

  // 2D loop with offset
  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, unsigned outerOffset, unsigned innerOffset, FN func)
  {
    return par_for(outerCount*innerCount, [innerCount, innerOffset, outerOffset, func](unsigned i) {
      func(i / innerCount + outerOffset, i % innerCount + innerOffset);
    });
  }

  // 3D loop
  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, FN func)
  {
    return par_for(xCount*yCount*zCount, [=](unsigned i) {
      const unsigned x = i % xCount;
      const unsigned y = (i/xCount) % yCount;
      const unsigned z = i/xCount/yCount;

      func(z, y, x);
    });
  }

  // 3D loop with offset
  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, unsigned zOffset, unsigned yOffset, unsigned xOffset, FN func)
  {
    return par_for(xCount*yCount*zCount, [=](unsigned i) {
      const unsigned x = i % xCount + xOffset;
      const unsigned y = (i/xCount) % yCount + yOffset;
      const unsigned z = i/xCount/yCount + zOffset;

      func(z, y, x);
    });
  }
};

// SIMT strategy that implements "parallelism" by serial execution
struct simt_strat_serial {
  typedef unsigned JobId;
  static const JobId InvalidJobId = (JobId)-1;

  static void jobwait(JobId) {}

  template<class FN> static inline
  void critical(FN func)
  {
    func();
  }

  template <typename T, typename A> static inline
  T addAndFetch(T *pValue, A arg)
  {
    return *pValue += arg;
  }
  template <typename T, typename A> static inline
  T fetchAndAdd(T *pValue, A arg)
  {
    T res = *pValue;
    *pValue += arg;
    return res;
  }
  template <typename T> static inline
  T incAndFetch(T *pValue) {
    return addAndFetch(pValue, 1);
  }
  template <typename T> static inline
  T decAndFetch(T *pValue) {
    return addAndFetch(pValue, -1);
  }
  template <typename T> static inline
  T fetchAndInc(T *pValue) {
    return fetchAndAdd(pValue, 1);
  }
  template <typename T> static inline
  T fetchAndDec(T *pValue) {
    return fetchAndAdd(pValue, -1);
  }

  template<class FN> static inline
  JobId par_for(unsigned count, FN func)
  {
    for (unsigned i = 0; i < count; ++i) {
      func(i);
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for(unsigned count, unsigned offset, FN func)
  {
    for (unsigned i = 0; i < count; ++i) {
      func(i + offset);
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, FN func)
  {
    for (unsigned i = 0; i < outerCount; ++i) {
      for (unsigned j = 0; j < innerCount; ++j) {
        func(i, j);
      }
    }

    return 0;
  }
  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, unsigned outerOffset, unsigned innerOffset, FN func)
  {
    for (unsigned i = 0; i < outerCount; ++i) {
      for (unsigned j = 0; j < innerCount; ++j) {
        func(i, j);
      }
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, FN func)
  {
    for (unsigned i = 0; i < zCount; ++i) {
      for (unsigned j = 0; j < yCount; ++j) {
        for (unsigned k = 0; k < xCount; ++k) {
          func(i, j, k);
        }
      }
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, unsigned zOffset, unsigned yOffset, unsigned xOffset, FN func)
  {
    for (unsigned i = 0; i < zCount; ++i) {
      for (unsigned j = 0; j < yCount; ++j) {
        for (unsigned k = 0; k < xCount; ++k) {
          func(i + zOffset, j + yOffset, k + xOffset);
        }
      }
    }

    return 0;
  }
};

#if defined(__arm__) || defined(__AARCH64EL__)
// SIMT strategy that uses the aegaeon TAU to implement parallelism
struct simt_strat_aegaeon {
  typedef int JobId;
  static const JobId InvalidJobId = (JobId)-1;

  static void jobwait(JobId id) {
    tau_jobwait(id);
  }

  static const int StackSize = 4096;

  // Execute (lambda) function in critical section
  template<class FN> static inline
  void critical(FN func)
  {
    throw "Not supported";
  }

  static inline
  int addAndFetch(volatile int *pValue, int arg)
  {
    int res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
#if defined(__arm__)
        "ldrex  %[res], [%[pValue]]\n"
        "add    %[res], %[res], %[arg]\n"
        "strex  %[flag], %[res], [%[pValue]]\n"
        "teq    %[flag], #0\n"
#else
        "ldxr   %w[res], [%[pValue]]\n"
        "add    %w[res], %w[res], %w[arg]\n"
        "stxr   %w[flag], %w[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
#endif
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue), [arg]"r"(arg)
        : "cc", "memory"
    );

    return res;
  }

  static inline
  float addAndFetch(volatile float *pValue, float arg)
  {
    float res;
    int tmp;
    unsigned flag;

    __asm __volatile (
        "1:\n"
#ifdef __arm__
        "ldrex  %[tmp], [%[pValue]]\n"
        "vmov   %[res], %[tmp]\n"
        "vadd.f32   %[res], %[res], %[arg]\n"
        "vmov   %[tmp], %[res]\n"
        "strex  %[flag], %[tmp], [%[pValue]]\n"
        "teq    %[flag], #0\n"
#else
        "ldxr   %w[tmp], [%[pValue]]\n"
	"fmov   %s[res], %w[tmp]\n"
        "fadd   %s[res], %s[res], %s[arg]\n"
	"fmov   %w[tmp], %s[res]\n"
        "stxr   %w[flag], %w[tmp], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
#endif
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&w"(res), [tmp]"=&r"(tmp)
        : [pValue]"r"(pValue), [arg]"w"(arg)
        : "cc", "memory"
    );

    return res;
  }

#ifdef __arm__
  static inline
  double addAndFetch(volatile double *pValue, double arg)
  {
    double res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
        "ldrexd r0, r1, [%[pValue]]\n"
        "vmov   %[res], r0, r1\n"
        "vadd.f64   %[res], %[res], %[arg]\n"
	"vmov   r0, r1, %[res]\n"
        "strexd  %[flag], r0, r1, [%[pValue]]\n"
        "teq    %[flag], #0\n"
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&w"(res)
        : [pValue]"r"(pValue), [arg]"w"(arg)
        : "r0", "r1", "cc", "memory"
    );

    return res;

  }
#else
  static inline
  double addAndFetch(volatile double *pValue, double arg)
  {
    double res;
    size_t tmp;
    unsigned flag;

    __asm __volatile (
        "1:\n"
        "ldxr   %[tmp], [%[pValue]]\n"
	"fmov   %d[res], %[tmp]\n"
        "fadd   %d[res], %d[res], %d[arg]\n"
	"fmov   %[tmp], %d[res]\n"
        "stxr   %w[flag], %[tmp], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&w"(res), [tmp]"=&r"(tmp)
        : [pValue]"r"(pValue), [arg]"w"(arg)
        : "cc", "memory"
    );

    return res;
  }
#endif

  static inline
  int incAndFetch(volatile int *pValue)
  {
    int res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
#ifdef __arm__
        "ldrex  %[res], [%[pValue]]\n"
        "add    %[res], %[res], #1\n"
        "strex  %[flag], %[res], [%[pValue]]\n"
        "teq    %[flag], #0\n"
#else
        "ldxr   %w[res], [%[pValue]]\n"
        "add    %w[res], %w[res], #1\n"
        "stxr   %w[flag], %w[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
#endif
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue)
        : "cc", "memory"
    );

    return res;
  }

  static inline
  int decAndFetch(volatile int *pValue)
  {
    volatile int res;
    volatile unsigned flag;

    __asm __volatile (
        "1:\n"
#ifdef __arm__
        "ldrex  %[res], [%[pValue]]\n"
        "sub    %[res], %[res], #1\n"
        "strex  %[flag], %[res], [%[pValue]]\n"
        "teq    %[flag], #0\n"
#else
        "ldxr   %w[res], [%[pValue]]\n"
        "sub    %w[res], %w[res], #1\n"
        "stxr   %w[flag], %w[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
#endif
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue)
        : "cc", "memory"
    );
 
    return res;
  }

  static inline
  unsigned addAndFetch(volatile unsigned *pValue, unsigned arg)
  {
    return (unsigned)addAndFetch((int *)pValue, (int)arg);
  }

  static inline
  unsigned incAndFetch(volatile unsigned *pValue)
  {
    return (unsigned)incAndFetch((int *)pValue);
  }

  static inline
  unsigned decAndFetch(volatile unsigned *pValue)
  {
    return (unsigned)decAndFetch((int *)pValue);
  }

#ifdef __AARCH64EL__
  static inline
  ptrdiff_t addAndFetch(volatile ptrdiff_t *pValue, ptrdiff_t arg)
  {
    ptrdiff_t res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
        "ldxr   %[res], [%[pValue]]\n"
        "add    %[res], %[res], %[arg]\n"
        "stxr   %w[flag], %[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue), [arg]"r"(arg)
        : "cc", "memory"
    );

    return res;
  }

  static inline
  ptrdiff_t incAndFetch(volatile ptrdiff_t *pValue)
  {
    ptrdiff_t  res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
        "ldxr   %[res], [%[pValue]]\n"
        "add    %[res], %[res], #1\n"
        "stxr   %w[flag], %[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue)
        : "cc", "memory"
    );

    return res;
  }

  static inline
  ptrdiff_t decAndFetch(volatile ptrdiff_t *pValue)
  {
    ptrdiff_t res;
    unsigned flag;

    __asm __volatile (
        "1:\n"
        "ldxr   %[res], [%[pValue]]\n"
        "sub    %[res], %[res], #1\n"
        "stxr   %w[flag], %[res], [%[pValue]]\n"
        "cmp    %w[flag], #0\n"
        "bne    1b\n"
        : [flag]"=&r"(flag), [res]"=&r"(res)
        : [pValue]"r"(pValue)
        : "cc", "memory"
          );
    return res;
  }

  static inline
  size_t addAndFetch(volatile size_t *pValue, size_t arg)
  {
    return (size_t)addAndFetch((ptrdiff_t *)pValue, (ptrdiff_t)arg);
  }

  static inline
  size_t incAndFetch(volatile size_t *pValue)
  {
    return (size_t)incAndFetch((ptrdiff_t *)pValue);
  }

  static inline
  size_t decAndFetch(volatile size_t *pValue)
  {
    return (size_t)decAndFetch((ptrdiff_t *)pValue);
  }
#endif // __AARCH64EL__

  static inline
  float incAndFetch(volatile float *pValue) {
    return addAndFetch(pValue, 1.0);
  }
  static inline
  float decAndFetch(volatile float *pValue) {
    return addAndFetch(pValue, -1.0);
  }
  static inline
  double incAndFetch(volatile double *pValue) {
    return addAndFetch(pValue, 1.0);
  }
  static inline
  double decAndFetch(volatile double *pValue) {
    return addAndFetch(pValue, -1.0);
  }

  template <typename T, typename A> static inline
  T fetchAndAdd(volatile T *pValue, const A arg)
  {
    return addAndFetch(pValue, arg) - arg;
  }

  template <typename T> static inline
  T fetchAndInc(volatile T *pValue) {
    return incAndFetch(pValue) - 1;
  }
  template <typename T> static inline
  T fetchAndDec(volatile T *pValue) {
    return decAndFetch(pValue) + 1;
  }

  // 1D loop

  // Marshall a 1D lambda function from CPU to TAU
  // Default version passes by reference
  template <typename FN, int FLAG>
  struct Marshall1D
  {
    /// Marshall a function into an aegaeon_args object
    static void marshall(FN &func, aegaeon_args &args) {
      args.args[0] = (void *)&func;
    }

    /// TAU kenel which unmarshalls the function and then calls it
    static void kernel(FN *func) KERNEL_ATTR
    {
      (*func)(tau_gid_x());
      tau_threadexit();
    }
  };

  // Specialised Marshalling for smaller functions which
  // copies by value in the registers
  template <typename FN>
  struct Marshall1D<FN, 1>
  {
    // Use a union for easy conversion between a lambda function
    // and TAU kernel arguments
    typedef union {
        // Use pointers because lambda functions aren't copyable
        FN *func;
        aegaeon_args *args;
    } U;

    static void marshall(FN &func, aegaeon_args &args) {
      U u;
      u.func = &func;
      args = *u.args;
    }

    static void kernel(void *a0, void *a1, void *a2, void *a3) KERNEL_ATTR
    {
      aegaeon_args args(a0, a1, a2, a3);

      U u;
      u.args = &args;
      (*u.func)(tau_gid_x());
      tau_threadexit();
    }
  };

  template<class FN> static inline
  JobId par_for(unsigned count, FN func)
  {
      return par_for(count, 0, func);
  }

  template<class FN> static inline
  JobId par_for(unsigned count, unsigned offset, FN func)
  {
    Marshall1D<FN, sizeof(FN)<=(4*sizeof(void *))?1:8> marshall;

    aegaeon_args args;
    aegaeon_jobparams params(count, 1, 1, offset);
    
    marshall.marshall(func, args);

    kernel_func prog = (kernel_func)&marshall.kernel;
    return tau_jobstart(prog, &args, &params);
  }

  // 2D loop
  template <typename FN, int FLAG>
  struct Marshall2D
  {
    void marshall(FN &func, aegaeon_args &args) {
      args.args[0] = (void *)&func;
    }

    static void kernel(FN *func) KERNEL_ATTR
    {
      (*func)(tau_gid_y(), tau_gid_x());
      tau_threadexit();
    }
  };

  template <typename FN>
  struct Marshall2D<FN, 1>
  {
    typedef union {
        FN *func;
        aegaeon_args *args;
    } U;

    void marshall(FN &func, aegaeon_args &args) {
      U u;
      u.func = &func;
      args = *u.args;
    }

    static void kernel(void *a0, void *a1, void *a2, void *a3) KERNEL_ATTR
    {
      aegaeon_args args(a0, a1, a2, a3);

      U u;
      u.args = &args;
      (*u.func)(tau_gid_y(), tau_gid_x());
      tau_threadexit();
    }
  };

  template<typename FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, FN func)
  {
    return par_for_2D(outerCount, innerCount, 0, 0, func);
  }

  template<typename FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, unsigned outerOffset, unsigned innerOffset, FN func)
  {
    Marshall2D<FN, sizeof(FN)<=(4*sizeof(void *))?1:8> marshall;

    aegaeon_args args;
    aegaeon_jobparams params(innerCount, outerCount, 1, innerOffset, outerOffset);
    
    marshall.marshall(func, args);

    kernel_func prog = (kernel_func)&marshall.kernel;
    return tau_jobstart(prog, &args, &params);
  }

  template <typename FN, int FLAG>
  struct Marshall3D
  {
    void marshall(FN &func, aegaeon_args &args) {
      args.args[0] = (void *)&func;
    }

    static void kernel(FN *func) KERNEL_ATTR
    {
      (*func)(tau_gid_z(), tau_gid_y(), tau_gid_x());
      tau_threadexit();
    }
  };

  template <typename FN>
  struct Marshall3D<FN, 1>
  {
    typedef union {
        FN *func;
        aegaeon_args *args;
    } U;

    void marshall(FN &func, aegaeon_args &args) {
      U u;
      u.func = &func;
      args = *u.args;
    }

    static void kernel(void *a0, void *a1, void *a2, void *a3) KERNEL_ATTR
    {
      aegaeon_args args(a0, a1, a2, a3);

      U u;
      u.args = &args;
      (*u.func)(tau_gid_z(), tau_gid_y(), tau_gid_x());
      tau_threadexit();
    }
  };

  template<typename FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, FN func)
  {
    return par_for_3D(zCount, yCount, xCount, 0, 0, 0, func);
  }

  template<typename FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, unsigned zOffset, unsigned yOffset, unsigned xOffset, FN func)
  {
    Marshall3D<FN, sizeof(FN)<=(4*sizeof(void *))?1:8> marshall;

    aegaeon_args args;
    aegaeon_jobparams params(xCount, yCount, zCount, xOffset, yOffset, zOffset);
    
    marshall.marshall(func, args);

    kernel_func prog = (kernel_func)&marshall.kernel;
    return tau_jobstart(prog, &args, &params);
  }
};
#else
typedef simt_strat_serial simt_strat_aegaeon;
#endif

// SIMT strategy that implements "parallelism" by execution in random order
// This is slow but should hightlight ordering issues not shown by serial execution
// Execution is still serial so this does not highlight any locking issues
struct simt_strat_random: public simt_strat_serial {
  typedef unsigned JobId;
  static const JobId InvalidJobId = (unsigned)-1;
  static void jobwait(JobId) {}

  template<class FN> static inline
  JobId par_for(unsigned count, FN func)
  {
    return par_for(count, 0, func);
  }

  template<class FN> static inline
  JobId par_for(unsigned count, unsigned offset, FN func)
  {
    std::vector<unsigned> idx(count);
    for (unsigned i = 0; i < count; ++i) {
      idx[i] = i + offset;
    }
    std::random_shuffle(idx.begin(), idx.end());

    for (unsigned i = 0; i < count; ++i) {
      func(idx[i]);
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, FN func)
  {
    return par_for_2D(outerCount, innerCount, 0, 0, func);
  }

  template<class FN> static inline
  JobId par_for_2D(unsigned outerCount, unsigned innerCount, unsigned outerOffset, unsigned innerOffset, FN func)
  {
    const unsigned total(outerCount*innerCount);
    std::vector<unsigned> idx(total);

    for (unsigned i = 0; i < idx.size(); ++i) {
      idx[i] = i;
    }
    std::random_shuffle(idx.begin(), idx.end());

    for (unsigned i = 0; i < total; ++i) {
      const unsigned outer = idx[i] / innerCount + outerOffset;
      const unsigned inner = idx[i] % innerCount + innerOffset;
      func(outer, inner);
    }

    return 0;
  }

  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, FN func)
  {
    return par_for_3D(zCount, yCount, xCount, 0, 0, 0, func);
  }

  template<class FN> static inline
  JobId par_for_3D(unsigned zCount, unsigned yCount, unsigned xCount, unsigned zOffset, unsigned yOffset, unsigned xOffset, FN func)
  {
    const unsigned total(xCount*yCount*zCount);
    std::vector<unsigned> idx(total);

    for (unsigned i = 0; i < idx.size(); ++i) {
      idx[i] = i;
    }
    std::random_shuffle(idx.begin(), idx.end());

    for (unsigned i = 0; i < total; ++i) {
      const unsigned x = idx[i] % xCount + xOffset;
      const unsigned y = (idx[i] / xCount) % zCount + yOffset;
      const unsigned z = idx[i] / xCount / yCount + zOffset;

      func(z, y, x);
    }

    return 0;
  }
};

// Instanciate simt class with specific strategies
typedef simt<simt_strat_serial> simt_serial;
typedef simt<simt_strat_omp> simt_omp;
typedef simt<simt_strat_aegaeon> simt_aegaeon;
typedef simt<simt_strat_random> simt_random;

} // ag

namespace ar {
  using ag::simt_serial;
  using ag::simt_omp;
  typedef ag::simt_aegaeon simt_tau;
  using ag::simt_random;
}
#endif // AG_SIMT_H
