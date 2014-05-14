#include <yamcha.h>
#include <stdio.h>
#include <stdexcept>

// $Id: example.cpp,v 1.3 2002/10/21 12:03:06 taku-ku Exp $;

/*
 Example of define call-back function

 compile: 
  c++ `yamcha-config --cflags` example.cpp -o example `yamcha-config --libs` 

 How to run:
 - Training: use YAMCHA parameter

    make CORPUS=foo MODEL=bar YAMCHA=/somewhere/example train
   
    if you want to rewrite all features, just make FEATURE parameter empty

    make CORPUS=foo MODEL=bar YAMCHA=/somewhere/example FEATURE=" " train

 - Testing: same as yamcha
   /somewhere/example -m MODEL < test
*/


/*
 --- DEFINITION of CALLBACK --

 There are two arguments: 
 1. pointer to the YamCha::Chunker instance 
 2. current position

In this example, add previous up to two words and their pos-tags which marked as "B" tag
In the following example, the words and pos-tags with "<<" marks are
added as new features when the tag at current position is identified.

Confidence NN B <<
in IN O
the DT B        <<
pound NN I
is VBZ O
widely RB O
expected VBN O
to TO O
take VB O      << CURRENT POSITION
..
*/
 
int addBeginningOfChunk (YamCha::Chunker *c, int pos)
{
  char tmp[1024];
  int i = 0;

  // seek from pos-1 to the beginning of sentence
  for (int j = pos-1; j >=0 ; j--) {

    // getTag (j) returns the tag at j-th position
    const char *a = c->getTag (j); 

    // if beginning 
    if (a[0] == 'B') {

      // getContext(j, k) returns the context at j-th token and k-th column
      sprintf (tmp, "BEGIN:%d:0:%s", i, c->getContext (j, 0));

      // addFeature (char *) added a new feature
      c->addFeature (tmp);

      sprintf (tmp, "BEGIN:%d:1:%s", i, c->getContext (j, 1));
      c->addFeature (tmp);

      i++;
    }

    if (i == 2) break; // add new features up to 2
  }

  return i;
}

int main (int argc, char **argv) 
{
  try {

    // make instance YamCha::Chunker
    YamCha::Chunker p (argc, argv);

    // set call-back function
    p.setSelector (&addBeginningOfChunk);

    // parse from STDIN and put the resutls to STDOUT
    while (p.parse (std::cin, std::cout)) {};

    return 0;
  }
   
  catch (std::exception &e) {
    std::cerr << e.what () << std::endl;
    return -1;
  }
}
