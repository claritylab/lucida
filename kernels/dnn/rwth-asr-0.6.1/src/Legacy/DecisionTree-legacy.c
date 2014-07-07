// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// This file was cut&pasted from the original CART software by Klaus Beulen
// and modified to work with Sprint.

#warning "Memory leak: This software does not free its data structures."

#define max(x,y)  ((x)<(y))?(y):(x)

// ===========================================================================
// from defines.h

#define M_PHOLEN 32     /* maximum number of chars per phoneme */

#define N_BUF    1024		/* length of character buffer */

#define	YES     -1
#define	NO      0

#define USED	1
#define UNUSED	0


// ===========================================================================
// from types.c

enum QuestType { PhonemClass, State, Position };

struct	Question
{
    char* name;
    enum QuestType 	type;
    union QuestUnion
    {
	char *set;
	int  state;
	int boundary;
    } value;
    int isSingleton;
};

struct ModelFeatures
{
    struct Phone *phone;
    int state;
    int boundary;
};

struct	LightTreeNode
{
    unsigned int  question;
    unsigned int  number;
    short    context;
    struct  LightTreeNode	*leftChild, *rightChild;
};

struct LightTree
{
    struct LightTreeNode *root;
    int n_Clusters;
    struct Question *Questions;
    int n_Questions;
    char **Phonems;
    int n_Phonems;
    int Boundary;
    int Silence;
};

// ===========================================================================
// from macros.h

#define GETMEM(var,count,type) if((var = (type*)calloc((count), sizeof(type))) == NULL) \
                                 {\
									 printf("Can't calloc " #var "\n");\
									 exit(1);\
								 }

#define REGETMEM(var,count,type) if((var = (type*)realloc(var, (count) * sizeof(type))) == NULL) \
                                 {\
									 printf("Can't realloc " #var "\n");\
									 exit(1);\
								 }

// ===========================================================================
// from globals.c

const char	BoundaryStr[] = "#", SilenceStr[] = "si";
const int	N_Segments = 3;

// ===========================================================================
// from cluster.c

struct LightTreeNode *CreateLightNode(void)
{
    struct LightTreeNode *locTree;
    
    GETMEM(locTree, 1, struct LightTreeNode);

    if(locTree != NULL)
	{
	    locTree->leftChild = NULL;
	    locTree->rightChild = NULL;
	    return(locTree);
	}
    else
	syserr("Cant't create node! Malloc failed!\n");
    return 0;
}

// ===========================================================================
// from file_io.c

int GetPhonemes(FILE *phofile, char ***phoArray, int *Boundary, int *Silence)
{
    int		pho;
    char    buf[N_BUF];

    *Boundary = *Silence = -1;
    pho = 0;

    while(1)
	{
	    if(fgets(buf, N_BUF, phofile) == NULL ||
	       strcmp(buf, "\n") == 0)
		break;

	    if(pho == 0)
		{
		    GETMEM(*phoArray, 1, char*);
		}
	    else
		{
		    REGETMEM(*phoArray, pho+1, char*);
		}
	    GETMEM((*phoArray)[pho], M_PHOLEN, char);

	    sscanf(buf, "%s", (*phoArray)[pho]);
	    /*
	      printf("phoneme read:%s\n", (*phoArray)[pho]);
	    */
	    if(strcmp((*phoArray)[pho], BoundaryStr) == 0)
		*Boundary = pho;
	    if(strcmp((*phoArray)[pho], SilenceStr) == 0)
		*Silence = pho;

	    pho++;
	}

    if(*Boundary == -1)
        error("boundary not defined");
    if(*Silence == -1)
	error("silence not defined");

    return(pho);
}

int GetQuestions(FILE *infile, struct Question **Questions,
		 char **Phonems, int N_Phonems,
		 BoundaryStyle boundaryStyle)
{

    char buf[N_BUF], /* buf2[N_BUF], */ *token;
    int /* i, */ c, n_quest, state, pho;

    n_quest = -1;

    /* parse the question file */
    while(1)
	{
	    /* read one line, if line empty or EOF => stop reading */
	    if(fgets(buf, N_BUF, infile) == NULL || strcmp(buf, "\n") == 0)
		break;
		
	    /* get question name */
	    token = strtok(buf, " \n");

	    /* (re)alloc 'Question' array */
	    n_quest++;
	    if(n_quest == 0)
		{
		    GETMEM(*Questions, 1, struct Question);
		}
	    else
		{
		    REGETMEM(*Questions, n_quest+1, struct Question);
		}
	    GETMEM((*Questions)[n_quest].name, strlen(token)+1, char);
	    GETMEM((*Questions)[n_quest].value.set, N_Phonems, char);

	    for(pho = 0; pho < N_Phonems; pho++)
		(*Questions)[n_quest].value.set[pho] = UNUSED;

	    if((*Questions)[n_quest].name == NULL)
		syserr("Can't malloc 'Questions[n_quest].name'");

	    strcpy((*Questions)[n_quest].name, token);

	    (*Questions)[n_quest].type = PhonemClass;

	    (*Questions)[n_quest].isSingleton = NO;

	    /* Look-Ahead */
	    token = strtok(NULL, " \n");

	    while(token)
		{
		    c = 0;
		    while(c < N_Phonems)
			{
			    if(strcmp(token, Phonems[c])==0)
				{
				    /* mark the phonem in this quest. as used */
				    (*Questions)[n_quest].value.set[c] = USED;
				    c = N_Phonems+1;
				}
			    else
				c++;
			}

		    if(c == N_Phonems)
			error("Can't find phoneme %s in phoneme list", token);

		    token = strtok(NULL, " \n");
		}
	    /*
	      printf("question %d read: %s ", n_quest, buf);
	      for(c = 0; c < N_Phonems; c++)
	      if((*Questions)[n_quest].value.set[c] == USED)
	      printf("%s ", Phonems[c]);

	      printf("\n");
	    */
	}


    /* adding state questions */
    for(state = 0; state < N_Segments; state++)
	{
	    sprintf(buf, "STATE-%d", state);
	    n_quest++;
	    if(n_quest == 0)
		{
		    GETMEM(*Questions, 1, struct Question);
		}
	    else
		{
		    REGETMEM(*Questions, n_quest+1, struct Question);
		}
	    GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	    strcpy((*Questions)[n_quest].name, buf);
	    (*Questions)[n_quest].type = State;
	    (*Questions)[n_quest].value.state = state;
	    (*Questions)[n_quest].isSingleton = NO;
	}

    /* adding position question */
    switch (boundaryStyle) {
    case noPosDep: 
	break;
    case posDep: {
	sprintf(buf, "POSITION-WORD-BOUNDARY");
	n_quest++;
	if(n_quest == 0)
	    {
		GETMEM(*Questions, 1, struct Question);
	    }
	else
	    {
		REGETMEM(*Questions, n_quest+1, struct Question);
	    }
	GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	strcpy((*Questions)[n_quest].name, buf);
	(*Questions)[n_quest].type = Position;
	(*Questions)[n_quest].value.boundary = 1;
	(*Questions)[n_quest].isSingleton = NO;
    } break;
    case superPosDep: {
	sprintf(buf, "ONE-PHONEME-WORD");
	n_quest++;
	if(n_quest == 0)
	    {
		GETMEM(*Questions, 1, struct Question);
	    }
	else
	    {
		REGETMEM(*Questions, n_quest+1, struct Question);
	    }
	GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	strcpy((*Questions)[n_quest].name, buf);
	(*Questions)[n_quest].type = Position;
	(*Questions)[n_quest].value.boundary = 1;
	(*Questions)[n_quest].isSingleton = NO;

	sprintf(buf, "POSITION-WORD-BEGINNING");
	n_quest++;
	if(n_quest == 0)
	    {
		GETMEM(*Questions, 1, struct Question);
	    }
	else
	    {
		REGETMEM(*Questions, n_quest+1, struct Question);
	    }
	GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	strcpy((*Questions)[n_quest].name, buf);
	(*Questions)[n_quest].type = Position;
	(*Questions)[n_quest].value.boundary = 2;
	(*Questions)[n_quest].isSingleton = NO;

	sprintf(buf, "POSITION-WORD-END");
	n_quest++;
	if(n_quest == 0)
	    {
		GETMEM(*Questions, 1, struct Question);
	    }
	else
	    {
		REGETMEM(*Questions, n_quest+1, struct Question);
	    }
	GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	strcpy((*Questions)[n_quest].name, buf);
	(*Questions)[n_quest].type = Position;
	(*Questions)[n_quest].value.boundary = 3;
	(*Questions)[n_quest].isSingleton = NO;
    } break;
    default:
	defect();
    }


    /* adding singleton questions */
    for(pho = 0; pho < N_Phonems; pho++) {
	if(strcmp(Phonems[pho], SilenceStr) && strcmp(Phonems[pho], BoundaryStr)) {
	    /* copy question name into 'buf' */
	    strcpy(buf, Phonems[pho]);
	    
	    /* add question */
	    n_quest++;
	    if(n_quest == 0) {
		GETMEM(*Questions, 1, struct Question);
	    } else {
		REGETMEM(*Questions, n_quest+1, struct Question);
	    }
	    GETMEM((*Questions)[n_quest].name, strlen(buf)+1, char);
	    GETMEM((*Questions)[n_quest].value.set, N_Phonems, char);
	    for(c = 0; c < N_Phonems; c++)
		(*Questions)[n_quest].value.set[c] = UNUSED;
	    strcpy((*Questions)[n_quest].name, buf);
	    (*Questions)[n_quest].type = PhonemClass;
	    (*Questions)[n_quest].value.set[pho] = USED;
	    (*Questions)[n_quest].isSingleton = YES;
	}
    }

    return(n_quest+1);
}

int ReadDefFiles(FILE *file, struct LightTree *lightTree, BoundaryStyle boundaryStyle)
{
    int count;
    char buffer[N_BUF];

    /* read phonemes */
    lightTree->n_Phonems = GetPhonemes(
	file, &(lightTree->Phonems),
	&(lightTree->Boundary), &(lightTree->Silence));

    /* skip phone part */
    count = 0;
    while (count < 2) {
	if (fgets(buffer, N_BUF, file) == NULL)
	    break;
        if(strcmp(buffer, "\n"))
            count = 0;
        else
            count++;
    }

    /* read questions */
    lightTree->n_Questions = GetQuestions(
	file, 
	&(lightTree->Questions), lightTree->Phonems, lightTree->n_Phonems,
	boundaryStyle);
    
    return(0);
}

int BuildTree(FILE *file, struct LightTreeNode *tree)
{
    char buf[N_BUF];
    /*  short number = 0;
	short quest;
	char context; 
	int n_Clusters = 0; */
    int max_a, max_b;
    int nArgs;
    char arg1[10], arg2[10], arg3[10];

	
    if(fgets(buf, N_BUF, file) == 0)
	return(0);

    nArgs = sscanf(buf, "%*[^(](%[^,],%[^,],%[^)])\n", arg1, arg2, arg3);
    switch(nArgs) {
    case 3:
	tree->number = atoi(arg3);
	tree->question = atoi(arg1);
	if(arg2[0] == 'l')
	    strcpy(arg2, "-1");
	else if(arg2[0] == 'r')
	    strcpy(arg2, "1");
	tree->context = atoi(arg2);
	tree->leftChild = CreateLightNode();
	tree->rightChild = CreateLightNode();
	max_a = BuildTree(file, tree->leftChild);
	max_b = BuildTree(file, tree->rightChild);
	return(max(max_a, max_b));
	break;

    case 2:
	if(isalpha(arg2[0])) {
	    tree->number = 0;
	    tree->question = atoi(arg1);
	    if(arg2[0] == 'l')
		strcpy(arg2, "-1");
	    else if(arg2[0] == 'r')
		strcpy(arg2, "1");
	    tree->context = atoi(arg2);
	    tree->leftChild = CreateLightNode();
	    tree->rightChild = CreateLightNode();
	    max_a = BuildTree(file, tree->leftChild);
	    max_b = BuildTree(file, tree->rightChild);
	    return(max(max_a, max_b));
	} else {
	    tree->number = atoi(arg2);
	    tree->question = atoi(arg1);
	    tree->context = 0;
	    tree->leftChild = NULL;
	    tree->rightChild = NULL;
	    return(tree->question);
	}
	break;
    case 1:
	tree->number = 0;
	tree->question = atoi(arg1);
	tree->context = 0;
	tree->leftChild = NULL;
	tree->rightChild = NULL;
	return(tree->question);
	break;
            
    default:
	return(-1);
	break;
    }
}
