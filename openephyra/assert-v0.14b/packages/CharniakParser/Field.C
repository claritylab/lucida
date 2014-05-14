
#include "Field.h"

Field::
Field( int length, const char mask[] )
:fragmentation_( length ),
 mask_( mask )
{
    if( fragmentation_ <= 0 )
	error( "fragmentation <= 0" );
}

bool
Field::
in( int integer ) const
{
    if( integer < fragmentation_ )
    {
	if( integer < 0 )
	    error( "Field given integer < 0" );
	return bool( mask_[ integer ] );
    }
    else
    {
	int	index = integer % fragmentation_;
	return bool( mask_[ index ] );
    }
}
