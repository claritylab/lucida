// camelCase method names
%rename("%(lowercamelcase)s", notregexmatch$name="^[A-Z]") "";
%include <arrays_java.i>

// Special typemap for arrays of audio.
%include "arrays_java.i"
%apply short[] {const int16 *SDATA};

%typemap(in) (size_t n, char **ptr) {
  int i = 0;
  $1 = (*jenv)->GetArrayLength(jenv, $input);
  $2 = (char **) malloc(($1)*sizeof(char *));
  /* make a copy of each string */
  for (i = 0; i<$1; i++) {
    jstring j_string = (jstring)(*jenv)->GetObjectArrayElement(jenv, $input, i);
    const char * c_string = (*jenv)->GetStringUTFChars(jenv, j_string, 0);
    $2[i] = malloc((strlen(c_string)+1)*sizeof(char));
    strcpy($2[i], c_string);
    (*jenv)->ReleaseStringUTFChars(jenv, j_string, c_string);
    (*jenv)->DeleteLocalRef(jenv, j_string);
  }
}

%typemap(freearg) (size_t n, char **ptr) {
  int i;
  for (i=0; i<$1; i++)
    free($2[i]);
  free($2);
}

%typemap(jni) (size_t n, char **ptr) "jobjectArray"
%typemap(jtype) (size_t n, char **ptr) "String[]"
%typemap(jstype) (size_t n, char **ptr) "String[]"

%typemap(javain) (size_t n, char **ptr) "$javainput"
