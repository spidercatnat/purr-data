/*__________________________________________________________________________

    detox            extract value/content from tag-structured symbol

    Copyright (C) 2003 - 2006 jan schacher


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
    pd-port             20060517
    revised tree output 20040712
    initial build         20030502
 
 
 
____________________________________________________________________________*/


#include "m_pd.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <stdio.h>

#define VERSION "1.3"
#define CLIP(x,a,b) (x)=(x)<(a)?(a):(x)>(b)?(b):(x)
#define MAXLENGTH 4096

typedef struct _detox
{
    t_object ob;
    t_atom   t_tree[257], t_attrpair[2];
    t_atom   t_comp, t_blip;
    t_atom   t_checkat[2];
    char     t_charbuf[MAXLENGTH];
    long     t_treecount;
    short    t_debug;
    short    t_mode;
    void     *s_outlet, *s_outlet2, *s_outlet3, *s_outlet4;
} t_detox;

t_symbol *ps_nothing;

void *detox_class;
void *detox_new(t_symbol *s, short argc, t_atom *argv);
void detox_bang(t_detox *x);
void detox_float(t_detox *x, t_float f);
void detox_list(t_detox *x, t_symbol *s, short argc, t_atom *argv);
void detox_anything(t_detox *x, t_symbol *s, short argc, t_atom *argv);
void detox_free(t_detox *x);
void detox_reset(t_detox *x);
void detox_debug(t_detox *x, float f);
void detox_mode(t_detox *x, float f);
void detox_anything(t_detox *x, t_symbol *s, short argc, t_atom *argv);
void detox_action(t_detox *x);
void detox_fixsymbol(t_detox *x);
void version(t_detox *x);


void detox_setup(void)
{
    detox_class = class_new(gensym("detox"), (t_newmethod)detox_new,
        (t_method)detox_free, sizeof(t_detox), 0, A_GIMME, 0);
    class_addsymbol(detox_class, (t_method)detox_anything);
    class_addmethod(detox_class, (t_method)detox_list,
        gensym("list"), A_GIMME,0);
    class_addmethod(detox_class, (t_method)detox_debug,
        gensym("debug"), A_FLOAT, 0);
    class_addmethod(detox_class, (t_method)detox_mode,
        gensym("mode"), A_FLOAT, 0);
    class_addmethod(detox_class, (t_method)detox_reset,
        gensym("reset"), 0);
    class_addmethod(detox_class, (t_method)version,
        gensym("version"), 0);
    post("-    detox    -    jasch    -    "__DATE__" ");
    ps_nothing = gensym("");
}

void *detox_new(t_symbol *s, short argc, t_atom *argv)
{
    t_detox    *x = (t_detox *)pd_new(detox_class);
    x->s_outlet  = outlet_new(&x->ob, NULL);
    x->s_outlet2 = outlet_new(&x->ob, NULL);
    x->s_outlet3 = outlet_new(&x->ob, NULL);
    x->s_outlet4 = outlet_new(&x->ob, gensym("float"));

    x->t_treecount = 0;
    x->t_debug = 0;

    if ((argc >= 1) && (argv[0].a_type == A_FLOAT))
    {
        if (argv[0].a_w.w_float == 0)
        {
            x->t_mode = 0;
        }
        else
        {
            x->t_mode = 1;
        }
    }
    else
    {
        x->t_mode = 1;
    }
    x->t_debug = 0;
    if(x->t_debug) post("mode is %ld", x->t_mode);

    x->t_checkat[0].a_type = A_SYMBOL;
    x->t_checkat[0].a_w.w_symbol = gensym("none");
    x->t_checkat[1].a_type = A_SYMBOL;
    x->t_checkat[1].a_w.w_symbol = gensym("none");

    return (x);
}

void detox_free(t_detox *x)
{
    // notify_free((t_object *)x);
}

void detox_bang(t_detox *x)
{
    detox_action(x);
}

void detox_float(t_detox *x, t_float f)
{
    sprintf(x->t_charbuf, "%.6f", f);
    detox_action(x);
}

void detox_list(t_detox *x, t_symbol *s, short argc, t_atom *argv)
{
    long i;
    char temp[MAXLENGTH];

    x->t_charbuf[0] = 0;
    for (i = 0; i < argc; i++)
    {
        if (i) strcat(x->t_charbuf, " ");
        temp[0] = 0;
        if (argv[i].a_type == A_FLOAT)
        {
            sprintf(temp, "%.6f", argv[i].a_w.w_float);
        }
        else if (argv[i].a_type == A_SYMBOL)
        {
            // check for whitespace in symbol and add back doublequotes
            if (strchr(argv[i].a_w.w_symbol->s_name, 32) == NULL)
            {        // no whitespace : normal proc
                strcpy(temp, argv[i].a_w.w_symbol->s_name);
            }
            else
            {  // with whitespace : fix it
                x->t_checkat[0] = argv[i];
                detox_fixsymbol(x);
                strcpy(temp, x->t_checkat[1].a_w.w_symbol->s_name);
            }
        }
        if ((strlen(x->t_charbuf) + strlen(temp)) < MAXLENGTH)
        {
            strcat(x->t_charbuf, temp);
        }
        else
        {
            return;
        }
    }
    detox_action(x);
}

void detox_anything(t_detox *x, t_symbol *s, short argc, t_atom *argv)
{
    long i;
    char temp[MAXLENGTH];

    x->t_charbuf[0] = 0;
    strcpy(x->t_charbuf, s->s_name);
    for (i = 0; i < argc; i++)
    {
        strcat(x->t_charbuf, " ");
        temp[0] = 0;
        if (argv[i].a_type == A_FLOAT)
        {
            sprintf(temp, "%.6f", argv[i].a_w.w_float);
        }
        else if (argv[i].a_type == A_SYMBOL)
        {
            // check for whitespace in symbol and add back doublequotes
            if (strchr(argv[i].a_w.w_symbol->s_name, 32) == NULL)
            {   // no whitespace : normal proc
                strcpy(temp, argv[i].a_w.w_symbol->s_name);
            }
            else
            {   // with whitespace : fix it
                x->t_checkat[0] = argv[i];
                detox_fixsymbol(x);
                strcpy(temp, x->t_checkat[1].a_w.w_symbol->s_name);
            }
        }
        // post("temp is %s", temp);
        if ((strlen(x->t_charbuf) + strlen(temp)) < MAXLENGTH)
        {
            strcat(x->t_charbuf, temp);
        }
        else
        {
            return;
        }
    }
    detox_action(x);
    return;
}

void detox_reset(t_detox *x)
{
    short i;
    x->t_treecount = 0;
    for(i=0;i<256; i++) x->t_tree[i].a_w.w_symbol = ps_nothing;
}

void detox_debug(t_detox *x, float f)
{
    if (f == 0.0)
    {
        x->t_debug = 0;
    }
    else if (f != 0)
    {
        x->t_debug = 1;
    }
}

void detox_mode(t_detox *x, float f)
{
    if (f == 0.0)
    {
        x->t_mode = 0;
    }
    else if (f != 0)
    {
        x->t_mode = 1;
    }
}

void detox_action(t_detox *x)
{
    t_atom      *outlist, *attrpair; // *comp,
    char        local[MAXLENGTH], local2[MAXLENGTH];
    char        temp[MAXLENGTH];
    char        tempstring[2]; // for strtok
    char        *ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL;
    long        i, j, k, last, len,  tempcount = 0;
    short       tagtype = 0; // 0 = closed, 1 = open, 2 = closing
    short       outsize = 0;
    char        quotetype[1];
    short       digitflag = 0;
    short       punctflag = 0;
    short       quoteflag = 0;
    // short    attrpresent = 0;
    quotetype[0] = '\"';  // init to standard double quotes
    i = 0;
    strcpy(tempstring, " ");

    outlist = x->t_tree;
    attrpair = x->t_attrpair;

    x->t_attrpair[0].a_type = A_SYMBOL;
    x->t_attrpair[1].a_type = A_SYMBOL;    // check type and recast on output

    strcpy(local2, x->t_charbuf);
    last = strlen(local2);                // test for tag

    // if(x->t_debug) post("input is %s", x->t_charbuf);

    k = 0;
    while (isspace(local2[k]))
    {   // remove whitespace/tab/cr/linefeed
        k++;
    }
        // if(x->t_debug) post("k is %ld, last is %ld", k, last);

    for (i = k, j = 0; i < last; i++, j++)
    {   // remove whitespace/tab/cr/linefeed
        local[j] = local2[i];
    }
    local[j] = 0; // terminate string

    if ((local[0] != '<') || (local[j-1] != '>'))
    {
        tagtype = 0; // not a well formed tag
        if(x->t_debug) post("tagtype 0");
        goto content;
    }
    if ((local[1] == '?') || (local[1] == '!'))
    {
        tagtype = 4; // this might be a metatag
        if (x->t_debug) post("tagtype 4");
        goto content;
    }



    for (i = 0, ptr1 = NULL, quoteflag = 0; i < (long)(strlen(local) -1); i++)
    {
        if ((local[i] == '\"') || (local[i] == '\''))
        {
            quoteflag = !quoteflag;
        }
        if ((local[i] == '/') && (quoteflag == 0))
        {
            ptr1 = &local[i];
        }
    }
    //ptr1 = strchr(local, '/');    // is it a closing tag ? // bug: if slash is within quotes: should ignore

        if(x->t_debug) post("ptr1 contains %s", x->t_charbuf);

    if (ptr1 == NULL)
    {   // we have new open tag
        tagtype = 2;
        if (x->t_debug) post("tagtype 2");
    }
    else if ((local[1] == '/'))
    {   // is it at beginning
        tagtype = 3;
        if (x->t_debug) post("tagtype 3");
    }
    else if (local[1] != '/')
    {   // is it elsewhere
        if (x->t_debug) post("tagtype 1");
        tagtype = 1;
    }

    switch (tagtype)
    {
        case 1:    // closed tag :: add temporarily to tree
            ptr1 = strchr(local, '>');
            ptr2 = strchr(local, ' ');

            if (ptr2 != NULL) *ptr2 = 0;
            if (ptr1 != NULL)
            {
                *ptr1 = 0;
            }
            else
            {
                return;
            }
            len = strlen(local);
            ptr3 = (char *)memmove((local + 0), (local + 1), len-1);
            local[len-1] = 0;
            ptr3 = local;
            x->t_blip.a_type = A_SYMBOL;
            x->t_blip.a_w.w_symbol = gensym(ptr3);
            tempcount = 1;
            break;
        case 2:    // opening tag :: add to tree, increment count
            ptr2 = strchr(local, ' ');
            if (ptr2 != NULL)
            {
                *ptr2 = 0;
                if (x->t_debug)
                    post("inside opening tag w/ whitespace, %s", local);
            }
            else
            {
                local[strlen(local) - 1] = 0;
                // local[strlen(ptr2) -1] = 0;
                if (x->t_debug)
                    post("inside opening tag no whitespace, %s", local);
                // return;
            }
            len = strlen(local);
            ptr3 = (char *)memmove((local + 0),(local + 1), len-1);
            if (x->t_debug) post("ptr3 is %s", ptr3);
            local[len-1] = 0;
            ptr3 = local;
            SETSYMBOL(x->t_tree + x->t_treecount, gensym(ptr3));
            x->t_treecount++;
            CLIP(x->t_treecount, 0, 255);
            break;
        case 3:    // closing tag :: remove from tree, decrement count
            ptr2 = strchr(local, '>');
            if (ptr2 != NULL)
            {
                *ptr2 = 0;
            }
            else
            {
                return;
            }
            len = strlen(local);
            ptr3 = (char *)memmove((local + 0),(local + 2), len-2);
            local[len-2] = 0;
            ptr3 = local;
            x->t_comp.a_type = A_SYMBOL;
            x->t_comp.a_w.w_symbol = gensym(ptr3);
            if (x->t_comp.a_w.w_symbol->s_name ==
                x->t_tree[x->t_treecount-1].a_w.w_symbol->s_name)
            {
                x->t_treecount -= 1;
                CLIP(x->t_treecount, 0, 255);
            }
            else
            {
                outlet_float(x->s_outlet4, 3);
                outlet_float(x->s_outlet3, -1);
                return;
            }
            break;
        }
content:

    outlet_float(x->s_outlet4, tagtype);
    if (tagtype == 0) return;
    // the tree
    if (x->t_treecount > 0)
    {
        if (tempcount)
        {
            outsize = x->t_treecount + 1;
            x->t_tree[outsize-1] = x->t_blip;
        }
        else
        {
            outsize = x->t_treecount;
        }
        outlet_list(x->s_outlet3, 0L, outsize, outlist);  // ouput tree
    }
    else if (x->t_treecount == 0)
    {
        if (tempcount)
        {
            // ouput tree
            outlet_anything(x->s_outlet3, x->t_blip.a_w.w_symbol, 0, 0L);
        }
        else
        {
            // ouput tree
            outlet_float(x->s_outlet3, 0);
        }
    }

    strcpy(local, x->t_charbuf);

    // parse content between enclosing tags    <tag>blah</tag>
    ptr1 = strchr(local, '>');
    if (ptr1 == NULL) goto attributes;
        *ptr1 = 0;
        ++ptr1;
        ptr2 = strchr(ptr1, '<');
    if (ptr2 == NULL) goto attributes;
        *ptr2 = 0;
    outlet_anything(x->s_outlet2, gensym(ptr1), 0, 0L);

attributes:

    strcpy(local, x->t_charbuf);    // retrieve full buffer again

    switch(x->t_mode)
    {   // the old way of attribute parsing (mode 0)
        case 0:
            if (x->t_debug) post("content: before strtok %s", local);
            ptr1 = local;
            ptr2 = strchr(local, '>');  // find tag end
            ptr2++;                     // terminate local buffer after tag end
            ptr2[0] = 0;
            if (strcspn(ptr1, "=\"\'") == strlen(ptr1))
            {   // check for attributes inside tag
                break;    // no attr-chars found, bail ouit
            }

            ptr1 = strchr(local, ' '); // find first whitespace
            if (ptr1 == NULL)
            {
                break; // no whitespace found bail
            }
            while((ptr1[0] == ' ') && (ptr1 != NULL))
            {   // find next non-whitespace char -> potential start of attr
                ptr1++;
            }
            ptr2 = ptr1; // setup second pointer from end of attr name
            while ( !((ptr2[0] == ' ') || (ptr2[0] == '=')) && (ptr2 != NULL))
            {
                // find next non-white or not equal-sign ->
                // what if not well formed ??
                ptr2++;
            }
            if (ptr2 == NULL)
            {
                error("detox: mode 0, "
                      "attrloop ptr2 reached NULL looking for \" \" and \"=\"");
                return;
            }
            /*len = 0;
            // determine length of attr name: name minus whitespace/equal sign
            len = strlen(ptr1) - strlen(ptr2);
            for (i = 0; i < len; i++)
            {
                temp[i] = ptr1[i];
            }
            // terminate temp_buffer
            temp[len] = 0;
            */
            // copy start pointer to position of end pointer -> remainder
            ptr1 = ptr2;
            while (ptr1 != NULL)
            {   // move start pointer until single or double quote
                if ((ptr1[0] == '\'') || (ptr1[0] == '\"'))
                {
                    quotetype[0] = ptr1[0];
                    break;
                }
                ptr1++;
            }
            if (ptr1 == NULL)
            {   // bail if no quotes found and end reached
                error("detox: mode 0, attrloop ptr1 reached NULL");
                return;
            }
            // store quote char for later comparison
            if (x->t_debug) post("first quotetype found is %c", quotetype[0]);
            // copy end pointer to pos of startpointer + 1,
            // right after the quote
            ptr2 = ++ptr1;
            while (ptr2 != NULL)
            {   // looking for next quote of stored type
                if (ptr2[0] == quotetype[0])
                {
                    break;
                }
                ptr2++;
            }
            // post("quotetype is %c and ptr2 is %c", quotetype[0], ptr2[0]);
            if (ptr2 == NULL)
            {
                error("detox: mode 0, attrloop ptr2 reached NULL");
                return;
            }
            len = 0;
            digitflag = 1; // reset check for long
            punctflag = 0; // reset check for float

            // get the length of the attr content
            len = strlen(ptr1) - strlen(ptr2);
            // post("ptr2 ends scan at %c", ptr2[0]);
            for (i = 0; i < len; i++)
            {   // copy attr-content into temp-buffer
                temp[i] = ptr1[i];
                punctflag += (temp[i] == '.');
                // are all digits?
                digitflag = digitflag && (isdigit(temp[i]) || ispunct(temp[i]));
            }
            temp[len] = 0;
            if (x->t_debug)
                post("temp is %s, digitflag is %ld, punctflag is %ld",
                    temp, digitflag, punctflag);

            if (digitflag == 0)
            {
                // to do: check atom type and cast accordingly js 20061202
                attrpair[0].a_type = A_SYMBOL;
                attrpair[0].a_w.w_symbol = gensym(temp);
                outlet_anything(x->s_outlet, x->t_attrpair[0].a_w.w_symbol,
                    0, 0L);
            }
            else if ((digitflag == 1) && (punctflag == 0))
            {
                // to do: check atom type and cast accordingly js 20061202
                attrpair[1].a_type = A_FLOAT;
                attrpair[1].a_w.w_float =  atoi(temp);
                outlet_float(x->s_outlet, x->t_attrpair[1].a_w.w_float);
            }
            else if ((digitflag == 1) && (punctflag == 1))
            {
                // to do: check atom type and cast accordingly js 20061202
                attrpair[1].a_type = A_FLOAT;
                attrpair[1].a_w.w_float =  atof(temp);
                outlet_float(x->s_outlet, x->t_attrpair[1].a_w.w_float);
            }
            break;
        case 1:// the new way of attribute parsing (mode 1) ���� fixed 20070322
            ptr1 = local;
            ptr2 = strchr(local, '>');  // find tag end
            ptr2++;                     // terminate local buffer after tag end
            ptr2[0] = 0;
            if (strcspn(ptr1, "=\"\'") == strlen(ptr1))
            {   // check for attributes inside tag
                break; // no attr-chars found, bail ouit
            }

            ptr1 = strchr(local, ' ');  // find first whitespace
            if (ptr1 == NULL)
            {
                break; // no whitespace found bail
            }

            // �� attribute loop; example:
            // <person name = 'Alan Turing' died = '07/06/1954' born = "1912-06-23" />
            for(;;)
            {
                while ((ptr1[0] == ' ') && (ptr1 != NULL))
                {   // find next non-whitespace char -> potential start of attr
                    ptr1++;
                }
                ptr2 = ptr1; // setup second pointer from end of attr name
                while ( !((ptr2[0] == ' ') || (ptr2[0] == '=')) &&
                    (ptr2 != NULL))
                {
                    // find next non-white or not equal-sign ->
                    // what if not well formed ??
                    ptr2++;
                }
                if (ptr2 == NULL)
                {
                    error("detox: mode 1, attrloop ptr2 reached NULL "
                          "looking for \" \" and \"=\"");
                    return;
                }
                len = 0;
                // determine length of attr name: name minus
                // whitespace/equal sign
                len = strlen(ptr1) - strlen(ptr2);
                for(i = 0; i < len; i++)
                {
                    temp[i] = ptr1[i];
                }
                // terminate temp_buffer
                temp[len] = 0;
                // put tempbuffer into output symbol
                attrpair[0].a_w.w_symbol = gensym(temp);
                // � done with the attr-name
                // copy start pointer to position of end pointer -> remainder
                ptr1 = ptr2;
                while (ptr1 != NULL)
                {   // move start pointer until single or double quote
                    if ((ptr1[0] == '\'') || (ptr1[0] == '\"'))
                    {
                        // store quote char for later comparison
                        quotetype[0] = ptr1[0];
                        break;
                    }
                    ptr1++;
                }
                if (ptr1 == NULL)
                {   // bail if no quotes found and end reached
                    error("detox: mode 1, attrloop ptr1 reached NULL");
                    return;
                }
                if (x->t_debug)
                    post("first quotetype found is %c", quotetype[0]);
                // copy end pointer to pos of startpointer + 1,
                // right after the quote
                ptr2 = ++ptr1;
                while (ptr2 != NULL)
                {   // looking for next quote of stored type
                    if (ptr2[0] == quotetype[0])
                    {
                        break;
                    }
                    ptr2++;
                }
                // post("quotetype is %c and ptr2 is %c", quotetype[0], ptr2[0]);
                if (ptr2 == NULL)
                {
                    error("detox: mode 1, attrloop ptr2 reached NULL");
                    return;
                }
                len = 0;
                digitflag = 1; // reset check for long
                punctflag = 0; // reset check for float

                // get the length of the attr content
                len = strlen(ptr1) - strlen(ptr2);
                // post("ptr2 ends scan at %c", ptr2[0]);
                for (i = 0; i < len; i++)
                {   // copy attr-content into temp-buffer
                    temp[i] = ptr1[i];
                    punctflag += (temp[i] == '.');
                    digitflag = digitflag && (isdigit(temp[i]) ||
                        ispunct(temp[i])); // are all digits?
                }
                temp[len] = 0;
                if (x->t_debug)
                    post("temp is %s, digitflag is %ld, punctflag is %ld",
                        temp, digitflag, punctflag);

                if (digitflag == 0)
                {
                    // to do: check atom type and cast accordingly js 20061202
                    attrpair[1].a_type = A_SYMBOL;
                    attrpair[1].a_w.w_symbol = gensym(temp);
                }
                else if ((digitflag == 1)&&(punctflag == 0))
                {
                    // to do: check atom type and cast accordingly js 20061202
                    attrpair[1].a_type = A_FLOAT;
                    attrpair[1].a_w.w_float =  atoi(temp);
                }
                else if ((digitflag == 1)&&(punctflag == 1))
                {
                    // to do: check atom type and cast accordingly js 20061202
                    attrpair[1].a_type = A_FLOAT;
                    attrpair[1].a_w.w_float =  atof(temp);
                }
                outlet_anything(x->s_outlet, x->t_attrpair[0].a_w.w_symbol,
                    1, &x->t_attrpair[1]);

                ptr1 = ++ptr2; // move start pointer to end of atrr-content
                while (ptr1 != NULL)
                {   // find next non-whitespace char
                    if (ptr1[0] != ' ')
                    {
                        break;
                    }
                    ptr1++;
                }
                if ((ptr1[0] == '/') || (ptr1[0] == '>') ||
                    (ptr1[0] == '?') || (ptr1 == NULL))
                {   // if char is "/" or ">" or "?" or end of string finish up
                    if (x->t_debug) post("ptr1 reached ending meta characters");
                    return;
                }
                if (x->t_debug) post("ptr1 reached loop point");
            }
            break;
    }
}

// this is an ugly hack for max stripping double quotes from symbols with
// whitespace in them (language problem)
void detox_fixsymbol(t_detox *x)
{
    long i = 0;
    long len = 0;
    x->t_checkat[1].a_w.w_symbol->s_name[0] = '\"'; // first char in output is "
    len  = strlen(x->t_checkat[0].a_w.w_symbol->s_name);
    CLIP(len, 0, (MAXLENGTH-3));

    for (i = 0; i < len; i++)
    {
        if ((x->t_checkat[0].a_w.w_symbol->s_name[i] == '/') ||
            (x->t_checkat[0].a_w.w_symbol->s_name[i] == '>'))
        {   // check for syntax differences
            break;
        }
        x->t_checkat[1].a_w.w_symbol->s_name[i+1] =
            x->t_checkat[0].a_w.w_symbol->s_name[i];
    }
    // last char in output is "
    x->t_checkat[1].a_w.w_symbol->s_name[i+1] = '\"';
    // terminate string
    x->t_checkat[1].a_w.w_symbol->s_name[i+2] = 0;
}

void version(t_detox *x)
{
    post(".    detox    .    jasch    .    v. "VERSION"    .    compiled "__DATE__" - "__TIME__ );
}
