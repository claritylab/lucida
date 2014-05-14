#include "ECString.h"
#include "ParseStats.h"
#include "utils.h"

istream&
operator >>( istream& is, ParseStats& ps )
{
  if( ps.totalMap != !! ps.totalPrecision != 0 )
    error( "Reading into non-empty parse stats." );
  ps.readInput( is );
  return is;
}

void
ParseStats::
readInput( istream& is )
{
  ECString bracket;
  is >> bracket;
  if( !is ) return;
  if( bracket != "<" ) error( "No open bracket for ParseStats" );
  is >> totalMap;
  is >> obviousMap;
  is >> correctMap;
  is >> totalPrecision;
  is >> obviousPrecision;
  is >> correctPrecision;
  is >> totalExact;
  is >> ignoredExact;
  is >> exactMap;
  is >> exactPrecision;
  is >> otherWrong;
  is >> bracket;
  if( bracket != ">" ) error( "No close bracket for ParseStats" );
}

ParseStats&
ParseStats::
operator+= ( const ParseStats& src )
{
  totalMap += src.totalMap;
  obviousMap += src.obviousMap;
  correctMap += src.correctMap;
  totalPrecision += src.totalPrecision;
  obviousPrecision += src.obviousPrecision;
  correctPrecision += src.correctPrecision;
  totalExact += src.totalExact;
  ignoredExact += src.ignoredExact;
  exactMap += src.exactMap;
  exactPrecision += src.exactPrecision;
  otherWrong += src.otherWrong;
  return *this;
}

ostream&
operator <<( ostream& os, const ParseStats& ps )
{
  os << "< " << ps.totalMap << " " << ps.obviousMap << " "
     << ps.correctMap << " " << ps.totalPrecision << " "
     << ps.obviousPrecision << " " << ps.correctPrecision
     << " " << ps.totalExact
     << " " << ps.ignoredExact
     << " " << ps.exactMap
     << " " << ps.exactPrecision
     << " " << ps.otherWrong << " >\n";
  return os;
}
