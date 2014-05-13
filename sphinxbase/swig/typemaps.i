%include <exception.i>

%apply int {int32};

#if SWIGPYTHON
%include python.i
#elif SWIGJAVA
%include java.i
#endif

// Define typemaps to wrap error codes returned by some functions,
// into runtime exceptions.
%typemap(in, numinputs=0, noblock=1) int *errcode {
  int errcode;
  $1 = &errcode;
}

%typemap(argout) int *errcode {
  if (*$1 < 0) {
    char buf[64];
    sprintf(buf, "$symname returned %d", *$1);
    SWIG_exception(SWIG_RuntimeError, buf);
  }
}

// Macro to construct iterable objects.
%define iterable(TYPE, PREFIX, VALUE_TYPE)

#if SWIGJAVA
%typemap(javainterfaces) TYPE "Iterable<"#VALUE_TYPE">"
%typemap(javainterfaces) TYPE##Iterator "java.util.Iterator<"#VALUE_TYPE">"

%javamethodmodifiers TYPE::__iter__ "private";

%typemap(javabody) TYPE %{

  private long swigCPtr;
  protected boolean swigCMemOwn;

  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @Override
  public java.util.Iterator<VALUE_TYPE> iterator() {
    return iter();
  }
%}

%typemap(javabody) TYPE##Iterator %{

  private long swigCPtr;
  protected boolean swigCMemOwn;

  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @Override
  public void remove() {
    throw new UnsupportedOperationException();
  }
%}
#endif

%inline %{
typedef struct {
  PREFIX##_iter_t *ptr;
} TYPE##Iterator;
%}

typedef struct {} TYPE;

%exception TYPE##Iterator##::next() {
  if (!arg1->ptr) {
#if SWIGJAVA
    jclass cls = (*jenv)->FindClass(jenv, "java/util/NoSuchElementException");
    (*jenv)->ThrowNew(jenv, cls, NULL);
    return $null;
#elif SWIGPYTHON
    SWIG_SetErrorObj(PyExc_StopIteration, SWIG_Py_Void());
    SWIG_fail;
#endif
  }
  $action;
}

%extend TYPE##Iterator {
  TYPE##Iterator(PREFIX##_iter_t *ptr) {
    TYPE##Iterator *iter = ckd_malloc(sizeof *iter);
    iter->ptr = ptr;
    return iter;
  }

  ~TYPE##Iterator() {
    PREFIX##_iter_free($self->ptr);
    ckd_free($self);
  }

  VALUE_TYPE * next() {
    if ($self->ptr) {
      VALUE_TYPE *value = next_##TYPE##Iterator($self->ptr);
      $self->ptr = PREFIX##_iter_next($self->ptr);
      return value;
    }

    return NULL;
  }

#if SWIGJAVA
  bool hasNext() {
    return $self->ptr != NULL;
  }
#endif
}

%extend TYPE {
  TYPE##Iterator * __iter__() {
    return new_##TYPE##Iterator(PREFIX##_iter($self));
  }
}

%enddef
