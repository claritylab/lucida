
#ifndef FIELD_H
#define FIELD_H

#include "utils.h"

class Field
{
public:
    Field( int length, const char mask[] );
    bool in( int integer ) const;
private:
    int 		fragmentation_;
    const char *	mask_;
};

#endif /* ! FIELD_H */
