#ifndef __MODEL_H
#define __MODEL_H

struct memoryfile
{    
	char *name;    
	int length;    
	char *data;
};

extern const struct memoryfile modelfiles[];

#endif

