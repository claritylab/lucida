/*
 * reader.c:
 * This file is part of multifast.
 *
    Copyright 2010-2013 Kamiar Kanani <kamiar.kanani@gmail.com>

    multifast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    multifast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with multifast.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include "reader.h"
#include "ahocorasick.h"

#if (AC_PATTRN_MAX_LENGTH>=READ_BUFFER_SIZE)
#error "READ_BUFFER_SIZE must be bigger than AC_PATTRN_MAX_LENGTH"
#endif

enum pars_mode
{
    PARSMOD_UNK,
    PARSMOD_HEX,
    PARSMOD_ASC,
};

enum escape_mode
{
    ESCMOD_OFF,
    ESCMOD_ON,
};

enum hex_half
{
    XHALF_L, // Lower Half
    XHALF_H, // Higher Half
};

struct parser_s
{
    int state;
    int lineno;
    int colno;
    unsigned int xhigh, xlow;
    enum hex_half xhalfpos;
    enum pars_mode parsmod;
    enum escape_mode escmod;
    struct token_s token;
};

struct buffer_s
{
    int index;
    int max_index;
    char * pool;
};

static struct parser_s parser;
static struct buffer_s buffer;

//*****************************************************************************
// FUNCTION: reader_init
//*****************************************************************************

char * reader_init(void)
{
    parser.state=0;
    parser.lineno=1;
    parser.colno=0;
    parser.xhalfpos=XHALF_H;
    parser.parsmod=PARSMOD_UNK;
    parser.escmod=ESCMOD_OFF;
    parser.token.value = (char *) malloc (AC_PATTRN_MAX_LENGTH);
    parser.token.last=0;
    parser.token.type=ENTOK_NONE;
    parser.token.value[0]=0;

    buffer.pool = (char *) malloc (READ_BUFFER_SIZE);
    buffer.index=0;
    buffer.max_index=0;
    buffer.pool[0]=0;

    return buffer.pool;
}

//*****************************************************************************
// FUNCTION: reader_reset_buffer
//*****************************************************************************

void reader_reset_buffer(int max)
{
    buffer.index=0;
    buffer.max_index=max;
}

//*****************************************************************************
// FUNCTION: reader_get_next_token
//*****************************************************************************

struct token_s * reader_get_next_token(void)
{
    #define IZSPACE(x) (x==' '||x=='\t'||x=='\n'||x=='\r')
    #define IZIDCHAR(x) ((x>='a'&&x<='z')||(x>='A'&&x<='Z')||(x>='0'&&x<='9')||(x=='_'))
    #define IZHEXCHAR(x) ((x>='a'&&x<='f')||(x>='A'&&x<='F')||(x>='0'&&x<='9'))
    #define GETHEXVALUE(x,y) {if(x>='0'&&x<='9')y=x-48;else if(x>='a'&&x<='f')y=x-87;else if(x>='A'&&x<='F')y=x-55;}
    #define GOTOERROR(x) \
            parser.state=6; \
            snprintf (parser.token.value, AC_PATTRN_MAX_LENGTH, \
            "[Error at %d:%d] " x, \
            parser.lineno, parser.colno);

    char ch;

    if (parser.token.type!=ENTOK_EOBUF)
    {
        parser.token.type=ENTOK_NONE;
        parser.token.last=0;
        parser.token.value[0]='\0';
    }

    while(buffer.index<buffer.max_index)
    {
        ch = buffer.pool[buffer.index++];
        if (ch=='\n')
        {
            parser.lineno++;
            parser.colno=0;
        }
        else
        {
            parser.colno++;
        }

        if (parser.token.last>=AC_PATTRN_MAX_LENGTH)
        {
            GOTOERROR("Very big pattern/ID")
        }

        switch(parser.state)
        {
        case 0:
            if (ch=='#')
            {
                parser.state=1;
            }
            else if (ch=='a'||ch=='x')
            {
                parser.state=2;
                parser.token.type=ENTOK_AX;
                parser.token.value[parser.token.last++]=ch;
                parser.token.value[parser.token.last]='\0';
                parser.parsmod=(ch=='a')?PARSMOD_ASC:PARSMOD_HEX;
                return &parser.token;
            }
            else if (!IZSPACE(ch))
            {
                GOTOERROR("Expected 'a' or 'x'")
            }
            break;
        case 1:
            if (ch=='\n')
                parser.state=0;
            break;
        case 2:
            if (ch=='(')
            {
                parser.state=3;
            }
            else if (ch=='{')
            {
                parser.state=5;
                parser.xhalfpos=XHALF_H;
            }
            else if (!IZSPACE(ch))
            {
                GOTOERROR("Expected '(' or '{'")
            }
            break;
        case 3:
            if (IZIDCHAR(ch))
            {
                parser.token.value[parser.token.last++]=ch;
            }
            else if (ch==')')
            {
                parser.state=4;
                parser.token.type=ENTOK_ID;
                parser.token.value[parser.token.last]='\0';
                return &parser.token;
            }
            else
            {
                GOTOERROR("Invalid character in ID")
            }
            break;
        case 4:
            if (ch=='{')
            {
                parser.state=5;
                parser.xhalfpos=XHALF_H;
            }
            else if (!IZSPACE(ch))
            {
                GOTOERROR("Expected '{'")
            }
            break;
        case 5:
            switch (parser.parsmod)
            {
            case PARSMOD_HEX:
                if (IZHEXCHAR(ch))
                {
                    switch (parser.xhalfpos)
                    {
                    case XHALF_H:
                        GETHEXVALUE(ch, parser.xhigh)
                        parser.xhigh<<=4;
                        parser.xhalfpos=XHALF_L;
                        break;
                    case XHALF_L:
                        GETHEXVALUE(ch, parser.xlow)
                        parser.token.value[parser.token.last++]=(char)(parser.xhigh|parser.xlow);
                        parser.xhalfpos=XHALF_H;
                        break;
                    }
                }
                else if (ch=='}')
                {
                    if (parser.xhalfpos==XHALF_L)
                    {
                        GOTOERROR("Odd number of hex digits")
                    }
                    else
                    {
                        parser.state=1;
                        parser.token.type=ENTOK_STRING;
                        parser.xhalfpos=XHALF_H;
                        return &parser.token;
                    }
                }
                else if (!IZSPACE(ch))
                {
                    GOTOERROR("Unexpected Character in Hex")
                }
                break;
            case PARSMOD_ASC:
                if (ch=='\\' && parser.escmod==ESCMOD_OFF)
                {
                    parser.escmod=ESCMOD_ON;
                }
                else if (ch=='}' && parser.escmod==ESCMOD_OFF)
                {
                    parser.state=1;
                    parser.token.type=ENTOK_STRING;
                    parser.xhalfpos=XHALF_H;
                    return &parser.token;
                }
                else
                {
                    parser.token.value[parser.token.last++]=ch;
                    parser.escmod=ESCMOD_OFF;
                }
                break;
            case PARSMOD_UNK:
                GOTOERROR("Unexpected Error!")
                break;
            }
            break;
        case 6:
            parser.token.type=ENTOK_ERR;
            return &parser.token;
            break;
        default:
            break;
        }
    }

    if (buffer.max_index<READ_BUFFER_SIZE-1)
        parser.token.type=ENTOK_EOF;
    else
        parser.token.type=ENTOK_EOBUF;

    return &parser.token;
}

//*****************************************************************************
// FUNCTION: reader_release
//*****************************************************************************

void reader_release (void)
{
    free(parser.token.value);
    free(buffer.pool);
}

/*****************************************************************************
 * Reader Graph:
 *****************************************************************************

                     all-\n
                       _|_
           />---#---> /   \
          /          (  1  )
         /<----\n---< \___/
        //
       //
  SP  //                SP              [1-9,aA-zZ]               SP
  _|_//                _|_                  _|_                  _|_
 /   \                /   \                /   \                /   \
(  0  )-----a,x----->(  2  )------(------>(  3  )-------)----->(  4  )
 \___/                \___/                \___/               /\___/
   ^                    \                                     /
   |                     \                                   /
   |                      \                                 /
    \                      \              ___              /
     \                      \-----{----> /   \ <-----{----/
      \                                 (  5  )
       \<----------}-------------------< \___/
                                           |
                                         all-}
*/
