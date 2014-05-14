#ifndef HEADFIND_H
#define HEADFIND_H

#include "ECString.h"
#include "InputTree.h"

void readHeadInfo(ECString& path);

int headPosFromTree(InputTree* tree);

int headPriority(ECString lhsString, ECString rhsString, int ansPriority);

#endif				/* ! HEADFIND_H */
