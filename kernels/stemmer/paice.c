
/* Stemmer Program 1994-5,by Andrew Stark   Started 12-9-94. */

/* This program first reads in a set of stemming rules, then */
/* a file containing words, one per line. It produces as output */
/* a similar file in which each word has been stemmed. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define maxrulesz 10 /* Maximum length of rule text */
#define maxwdsz 25 /* Maximum length of word stemmable */
#define maxsuffsz 8 /* Maximum length of suffix */
#define maxlinelength 80 /* Maximun length of line read from file */
#define true 1 /* Boolean true */
#define false 0 /* Boolean false */
#define stop 2 /* Stop here */
#define continue 1 /* Continue with next rule */
#define notapply 0 /* Rule did not apply */
#define notintact 3 /* Word no longer intact */
#define blank '?' /* For when replace string is empty */

struct rule {
char text[maxwdsz]; /* To return stemmer output */
char keystr[maxsuffsz]; /* Key string,ie,suffix to remove */
char repstr[maxsuffsz]; /* string to replace deleted letters */
char intact; /* Boolean-must word be intact? */
char cont; /* Boolean-continue with another rule? */
int rulenum; /* Line number of rule in rule list file */
char protect; /* Boolean-protect this ending? */
char deltotal; /* Delete how many letters? */
struct rule *next; /* Next item in linked list */
};

char addrule(struct rule r,struct rule t[26]);
struct rule stem(char *word,struct rule tbl[26]);
int tblindex(char *s);
int applyrule(struct rule *r,char *word,int isintact);
int rulewalk(char *word,struct rule t[26],int is_intact,struct rule *used);
int isvalidstr(char *s);
struct rule makerule(char *s,int line);
void readrules(FILE *fp,struct rule ttable[26]);
void inittable(struct rule t[26]);
int flagerror(char *s);
int isvowel(char s);
int isconsonant(char s);
int acceptable(char *s);

main(int argc,char *argv[]){

    struct rule trule;
struct rule ttable[26];

FILE *rulp;
FILE *inp;
FILE *outp;
char didit;
char instring[maxwdsz];
char *str;
int x;
struct rule *rp;

if(argc != 3){ /* If there is no rule file specified... */
printf("Must supply rule and input file names\n");
exit(1);
}

/* If file of stemmer rules is not opened correctly... */ 
if((rulp = fopen(argv[1],"r")) == NULL){

printf("Sorry,rule file error.You lose.\n");
exit(1); /* ..then leave abruptly with a message */
}
printf("Rule file opened.\n");

/* Same for the input file... */
if((inp = fopen(argv[2],"r")) == NULL){
printf("Sorry,source file error.You lose\n");
exit(1);
}
/* Create a new output file if one doesn't exist already */
/* (previous one will be overwritten).Name is same as input  */
/* file,but with '.stm' added at the end */
if((outp = fopen(strcat(argv[2],".stm"),"w+")) == NULL){
printf("Sorry,output file error.Try again\n");
exit(1);
}

inittable(ttable); 
readrules(rulp,ttable); /* Read in a rule set from the file */

fclose(rulp); /* And then close the file */

/* Get each line of the file,stem it (if it's a word) and write it */
/* to the output file until EOF(input) */
while(fgets(instring,maxlinelength,inp) != NULL){

/* If the line of input file begins with a letter.. */
if(isalpha(*instring)){
*(strchr(instring,'\n')) = '\0'; /* Delete the return character */

/* Remove unwanted char from the end of the line */
instring[strcspn(instring," \t\n")] = '\0';

trule = stem(instring,ttable); /* Stem the word.. */

/* ..and write the stem (+ a newline char) to the output file */
/* along with rule number */
fprintf(outp,"%s %d\n",trule.text,trule.rulenum);
}
else{
fputs(instring,outp);
}
}
fclose(inp); /* Close the input file */
fclose(outp); /* And the output file */

} /* end */

int tblindex(char *s){

    int x;
     
    for(x=0;*(s + x) != '\0';x++); /* Read to end of string */
    x--; /* Go back one letter to be at end of word */
    
    return (*(s + x)-'a'); /* Return number from 0..25 */
}

int isvalidstr(char *s){
    int x;
    
    for(x=0;*(s + x) != '\0';x++){ /* For each letter in the word... */ 

/* If it's not lower case or an apostrophe.. */
if(! islower(*(s + x)) && *(s + x) != '\''){ 
    return 0; /* ..then it`s an error.. */
}
    }
    return 1; /* ..otherwise,it`s ok */
}

char addrule(struct rule r,struct rule t[26]){
    
    int x;
    struct rule *temp;
    struct rule *trail;
    
    x = tblindex(r.keystr); /* Find out where in table to put it */

    trail = (t + x); /* Set trail pointer to address of list header */

    /* Walk along linked list with this loop */
    for(temp = (t + x)->next;temp != NULL;temp = temp->next){
    trail = temp;
    }
    /* Make a new instance of struct rule.. */
    trail->next = (struct rule *) malloc(sizeof(struct rule));

    memcpy(trail->next,&r,sizeof(struct rule)); /* ..copy r into it.. */
    trail->next->next = NULL; /* ..and set its "next" pointer to null */
return 1;
}

int applyrule(struct rule *r,char *word,int isintact){

/* Apply the rule r to word,leaving results in word.Return stop,continue */
/* or notapply as appropriate */
 
    int x;

    if(! strcmp(r->text,"dummy")){ /* If it's just a dummy list header.. */ 
return notapply; /* ..then it automatically doesn't apply */
}

    if(! isintact && r->intact){ /* If it should be intact,but isn't.. */
    return notapply; /* ..then rule fails */
    }
    
    x = strlen(word) - r->deltotal; /* Find where suffix should start */
    
    if(! strcmp(word + x,r->keystr)){ /* If ending matches key string.. */

if(! r->protect){ /* ..then if not protected.. */
strcpy(word + x,r->repstr); /* ..then swap it for rep string.. */
}
else{
return stop; /* If it is protected,then stop */
}

if(r->cont){ /* If continue flag is set,return cont */
return continue;
}
else{
return stop;
}
    }
    else{
return notapply; /* ..otherwise,this rule is not applicable */
    }
}

int rulewalk(char *word,struct rule t[26],int isintact,struct rule *used){
    
    int x;
    int result;
    struct rule *rp;
    char tempwd[maxwdsz];
    
    x = tblindex(word); /* Find out which list to walk along */
    strcpy(tempwd,word); /* Copy word for safe keeping */

    /* For each rule in list.. */
    for(rp = (t + x)->next;rp != NULL;rp = rp->next){
    strcpy(tempwd,word);    /* Copy word for safe keeping */
    
/* If rule applied to this word... */
if((result = applyrule(rp,tempwd,isintact)) != notapply){
strcpy(word,tempwd);
memcpy(used,rp,sizeof(struct rule));
return result;
}

}
return stop; /* If no rule was used,then we can stop */
}


struct rule makerule(char *s,int line){

/* Warning!In this form,makerule will fail (crash) if rule format is */
/* not exactly right! */

/* Format is: keystr,repstr,flags where keystr and repstr are strings,and */
/* flags is one of:"protect","intact","continue","protint","contint",or  */
/* "stop" (without the inverted commas in the actual file).  */

struct rule temp;

char *tempkey;
char *temprep;
char *tempflags;

int error = 0;

tempkey = strtok(s,","); /* The unusual(but very useful) strtok */
temprep = strtok(NULL,","); /* function splits the line into fields */ 
tempflags = strtok(NULL,"\t"); /* delimited by commas (or tab at the end)*/

/* Handle possible errors in... */
if(! isvalidstr(tempkey)){ /* ..key string.. */
printf("Invalid key string:line %d\n",line);
error = 1;
}
if(! isvalidstr(temprep) &&  strcmp(temprep,"?")){ /* ..replace string.. */
printf("Invalid replace string:line %d\n",line);
error = 1;
}
if(flagerror(tempflags)){ /* ..or flag field */
printf("Invalid flag:line %d\n",line);
error = 1;
}
if(error){ /* If there was an error then don't compile this rule */
*temp.keystr = '?'; /* Error signal */
return temp;
}

/* If there's no replace string,then put a null char instead */ 
*(temprep) = (*(temprep) == blank) ? '\0' : *(temprep); 

strcpy(temp.keystr,tempkey); /* Copy key string into the rule struct */
strcpy(temp.repstr,temprep); /* Copy replace string to same place */

if(! strcmp(tempflags,"protect")){ /* If flag field = "protect"... */
temp.cont = false; /* ..set continue to false.. */
temp.protect = true; /* ..set protect to true */
temp.intact = false; /* Guess what? */
}

if(! strcmp(tempflags,"intact")){
temp.cont = false;
temp.protect = false;
temp.intact = true;
}

if(! strcmp(tempflags,"continue")){
temp.cont = true;
temp.protect = false;
temp.intact = false;
}

if(! strcmp(tempflags,"contint")){
temp.cont = true;
temp.intact = true;
temp.protect = false;
}

if(! strcmp(tempflags,"protint")){
temp.cont = false;
temp.protect = true;
temp.intact = true;
}

if(! strcmp(tempflags,"stop")){
temp.cont = false;
temp.protect = false;
temp.intact = false;
}

temp.deltotal = strlen(tempkey); /* Delete total = length of key string */
temp.rulenum = line; /* Line number of rule in file */

return temp;
}

void readrules(FILE *fp,struct rule ttable[26]){

char s[maxlinelength];
int n = 0;
struct rule temp;


while(fgets(s,maxlinelength,fp)){/* Read a line at a time until eof */

n += 1; /* Increment line counter */
temp = makerule(s,n); /* Make line into a rule */
printf("Line: %d Key: %s Rep: %s\n",n,temp.keystr,temp.repstr); 
addrule(temp,ttable); /* Add rule to the table */

}
} 

int flagerror(char *s){

return (strcmp(s,"continue") && /* If s is not equal to "continue".. */
strcmp(s,"intact") && /* ..or "intact".. */
strcmp(s,"stop") && /* ..or "stop",etc... */
strcmp(s,"protect") &&
strcmp(s,"protint") &&
strcmp(s,"contint"));

}


struct rule stem(char *s,struct rule ttable[26]){

/* Stem the word,using the given table */

int isintact = 1;
int result;
char tx[maxwdsz];
char trail[maxwdsz];
struct rule r;
char *aposn;

r.rulenum = 0; /* Initialise rulenum to 0,in case no rule is used */

/* If s is already an invalid stem before we even start.. */
if(! acceptable(s)){
strcpy(r.text,s); /* ..then just return an unaltered s.. */
return r;
}

strcpy(tx,s); /* Make a copy of s,so we don't inadvertently alter it*/
strcpy(trail,s);

/* While there is still stemming to be done.. */
while((result = rulewalk(tx,ttable,isintact,&r)) != stop){

isintact = false; /* Because word is no longer intact */

if(! acceptable(tx)){ /* Exit from loop if not acceptable stem */
break;
}
strcpy(trail,tx); /* Set up trail for next iteration */
}


/* Package stemmer output along with rule */
strcpy(r.text,(result == stop) ? tx : trail);

    /* -------Remove apostrophe if it exists ---------------- */
r.text[strcspn(r.text,"\'")] = '\0';
        
return r; /* Return the rule,stemmed */
} 

void inittable(struct rule t[26]){

int x;

/* For each item in the list.. */
for(x = 0;x < 26;x++){ 
(t + x)->next = NULL; /* Set the 'next' pointer to NULL,and.. */
strcpy((t + x)->text,"dummy"); /* ..set the text field to 'dummy' */
}
}

int isvowel(char s){

/* Return true (1) if it's a vowel,or false otherwise */
/* NB - Treats 'y' as a vowel for the stemming purposes! */

switch (s) {
case 'a':case 'e':case 'i': case 'o':case 'u':case 'y':
return 1;
}
return 0;
}

int isconsonant(char s){

/* Return true (1) if it's a consonant,or false otherwise */

return islower(s) && ! isvowel(s);

} 

int acceptable(char *s){

/* Acceptability condition:if the stem begins with a vowel,then it */
/* must contain at least 2 letters,one of which must be a consonant*/
/* If,however,it begins with a vowel then it must contain three */
/* letters and at least one of these must be a vowel or 'y',ok? */

int x;

/* If longer than 3 chars then don't worry */
if((x = strlen(s)) > 3){
return 1;
}

/* If first is a vowel,then second must be a consonant */
if(isvowel(*s)){
return isconsonant(*(s + 1));
}

/* If first is a consonant,then second or third must be */
/* a vowel and length must be >3 */
return isvowel((*(s + 1)) || isvowel(*(s + 2))) && (x  > 3);
}

