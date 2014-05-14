
#include "ECArgs.h"
#include <assert.h>
#include <stdio.h>
#include "utils.h"
#include "algo.h"

ECArgs::
ECArgs(int argc, char *argv[])
{
  nargs_ = 0;
  for(int i = 1 ; i < argc ; i++)
    {
      ECString arg(argv[i]);
      if(arg[0] != '-')
	{
	  argList[nargs_] = arg;
	  nargs_++;
	}
      else
	{
	  nopts_++;
	  int l = arg.length();
	  assert(l > 1);
	  ECString opt(1,arg[1]);
	  optList.push_back(opt);
	  if(l == 2) optList.push_back("");
	  else
	    {
	      ECString v(arg,2,l-2);
	      optList.push_back(v);
	    }
	}
    }
}

bool
ECArgs::
isset(char c)
{
  ECString sig = "";
  sig += c;
  list<ECString>::iterator
    oIter = find(optList.begin(), optList.end(), sig);
  bool found = (oIter != optList.end());
  return found;
}


ECString
ECArgs::
value(char c)
{
  ECString sig;
  sig += c;

  list<ECString>::iterator oIter = find(optList.begin(), optList.end(), sig);
  bool found = (oIter != optList.end());
  if(!found)
    {
      cerr << "Looking for value of on-line argument " << sig << endl;
      error("could not find value");
    }
  ++oIter;
  return *oIter;
}


