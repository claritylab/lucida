%include <file.i>

// Special typemap for arrays of audio.
%typemap(in) \ 
  (const void *SDATA, size_t NSAMP) = (const char *STRING, size_t LENGTH);

%typemap(check) size_t NSAMP {
  char buf[64];
  if ($1 % sizeof(int16)) {
    sprintf(buf, "block size must be a multiple of %zd", sizeof(int16));
    SWIG_exception(SWIG_ValueError, buf);
  }
}

%typemap(in) (size_t n, char **ptr) {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) malloc(($1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
        $2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"must be a list of strings");
        free($2);
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"list type expected");
    return NULL;
  }
}

%typemap(freearg) (size_t n, char **ptr) {
  free($2);
}
