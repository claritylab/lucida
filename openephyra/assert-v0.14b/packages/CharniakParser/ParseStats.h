
#ifndef PARSESTATS_H
#define PARSESTATS_H

#include <iostream.h>

class ParseStats
{
 public:
  ParseStats() : totalMap( 0 ), obviousMap( 0 ),
                 correctMap( 0 ), totalPrecision( 0 ),
                 obviousPrecision( 0 ), correctPrecision( 0 ),
                 totalExact( 0 ), ignoredExact( 0 ),
                 exactMap( 0 ), exactPrecision( 0 ),
                 otherWrong( 0 ) {}
  ParseStats( istream& is ){ readInput( is ); }
  friend istream& operator >>( istream& is, ParseStats& ps );
  friend ostream& operator <<( ostream& os, const ParseStats& ps );
  ParseStats &    operator+= (const ParseStats & ps);	// merge operator.
  int totalMap;
  int obviousMap;
  int correctMap;
  int totalPrecision;
  int obviousPrecision;
  int correctPrecision;
  int totalExact;
  int ignoredExact;
  int exactMap;
  int exactPrecision;
  int otherWrong;
 private:
  void readInput( istream& is );
};

#endif /* ! PARSESTATS_H */
