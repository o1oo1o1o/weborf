/*
Weborf
Copyright (C) 2007  Salvo "LtWorf" Tomaselli

Weborf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@author Salvo "LtWorf" Tomaselli <tiposchi@tiscali.it>
*/

#include "options.h"
#include "types.h"


#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "instance.h"
#include "mystring.h"

/**
This function converts a string to upper case
*/
void strToUpper(char *str) {
    int i = -1;
    while (str[++i]) {
        str[i] = toupper(str[i]);
    }
}

/**
Replaces escape sequences in the form %HEXCODE with the correct char
This is used for URLs, after the transformation the URL will probably
represent a file that exists on filesystem.
Since after this replace the string will be unchanged or shorter, no
additional buffer will be needed.

This function is in-place, doesn't create copies but changes the original string.

If the string terminates with an % not followed by the HEXCODE,
it will set the string as an empty one.
*/
void replaceEscape(char *o_string) {
    char e_seq[3];
    char *string=o_string;

    size_t o_len=strlen(o_string);
    unsigned int i_count=0;

    e_seq[2] = 0;

    //Parses the string
    while ((string=strstr(string,"%"))!=NULL) {

        size_t c=o_len- (string-o_string) - (i_count*2);

        if (c<3) { //Safety check
            goto error;
        }

        e_seq[0] = string[1];
        e_seq[1] = string[2];

        //Shortening string of 2 chars
        c-=2;
        memmove(string,string+2,c);
        string[c]=0;


        //Replaces the 3rd character with the char corresponding to the escape
        string[0] = strtoul(e_seq, NULL, 16);

        i_count++;  //Counting the iterations, multiply by 2 to know how many chars were removed
        string++;   //Incrementing pointer, so if the escape was to create a % we don't re-convert it
    }

    return;
error:
    o_string[0]='\0';
    return;
}

/**
This function replaces, within the string string, the substring substr with the char with.

This function is in-place, doesn't create copies but changes the original string.
*/
void strReplace(char *string, char *substr, char with) {
    char *pointer;
    int substrlen = strlen(substr);

    while ((pointer = strstr(string, substr)) != NULL) {
        delChar(pointer, 0, substrlen - 1);
        pointer[0] = with;
        string=pointer;
    }
}

/**
This function deletes n chars from the string, starting from the position pos.
In case deleting of more chars than the string's len itself, the string will be returned unchanged.

This function doesn't create copies but changes the original string.
*/
void delChar(char *string, int pos, size_t n) {
    size_t l=strlen(string+pos);

    if (l<n) return;
    l-=n;
    memmove(string+pos,string+pos+n,l);
    string[l]=0;

    return;
}

/**
Returns true if str ends with end

str String to compare
end second string
size of str
size of end

If str ends with end, true is returned
false otherwise
*/
bool endsWith(char *str, char *end,size_t len_str,size_t len_end) {
    return strcmp(str+len_str-len_end,end)==0;
}
