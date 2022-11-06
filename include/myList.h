/*
  Windows 2000 XP API Wrapper Pack
  Copyright (C) 2008 OldCigarette

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//Written by TheProphet

#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

extern HANDLE hHeap;


typedef struct _myNode
{
    DWORD data;
    struct _myNode * next;
} myNode;

typedef struct
{
    myNode * head;
    myNode * tail;
    long length;
    HANDLE lockMutex;
} myList;


__inline void GetLock(myList * l)
{
    WaitForSingleObject(l->lockMutex, INFINITE);
}

__inline void ReleaseLock(myList * l)
{
    ReleaseMutex(l->lockMutex);
}


__inline void InitializeList(myList ** l)
{
    *l = HeapAlloc(hHeap, 0, sizeof(myList));
    (*l)->head = NULL;
    (*l)->tail = NULL;
    (*l)->length = 0;
    (*l)->lockMutex = CreateMutex(NULL, FALSE, NULL);
}


__inline void PushFrontByAddr(myList * l, myNode * nouv)
{  
    if(!l->head)
        l->tail = nouv;
    
    nouv->next = l->head;
    l->head = nouv;
    l->length++;
}

__inline void PushBackByAddr(myList * l, myNode * nouv)
{
    if(!l->head)
    {
        nouv->next = NULL;
        l->tail = nouv;
        l->head = nouv;
    }
    else
    {
        nouv->next = NULL;
        l->tail->next = nouv;
        l->tail = nouv;
    }
    l->length++;
}

__inline void PushFrontByVal(myList * l, DWORD data)
{
    myNode * nouv = NULL;
    nouv = HeapAlloc(hHeap, 0, sizeof(myNode));
    nouv->next = l->head;
    nouv->data = data;
    
    if(!l->head)
        l->tail = nouv;
    
    l->head = nouv;
    l->length++;
}

__inline void PushBackByVal(myList * l, DWORD data)
{
    myNode * nouv = NULL;
    nouv = HeapAlloc(hHeap, 0, sizeof(myNode));
    nouv->data = data;
    
    if(!l->head)
    {
        nouv->next = NULL;
        l->tail = nouv;
        l->head = nouv;
    }
    else
    {
        nouv->next = NULL;
        l->tail->next = nouv;
        l->tail = nouv;
    }
    l->length++;
}

__inline void RemoveNodeByVal(myList * l, DWORD data)
{
    myNode * temp = NULL;
    myNode * cour = NULL;
    myNode ** prec = NULL;

    cour = l->head;
    prec = &l->head;

    while(cour)
    {
        if(cour->data == data)
        {
            temp = cour->next;

            HeapFree(hHeap, 0, cour);

            *prec = temp;
            cour = temp;
            l->length--;
        }
        else
        {
            prec = &(cour->next);
            cour = cour->next;
        }
    }
}

__inline void RemoveNodeByAddr(myList * l, myNode * node)
{
    myNode * temp = NULL;
    myNode * cour = NULL;
    myNode ** prec = NULL;
    
    cour = l->head;
    prec = &l->head;

    while(cour)
    {
        if(cour == node)
        {
            temp = cour->next;

            HeapFree(hHeap, 0, cour);

            *prec = temp;
            cour = temp;
            l->length--;
        }
        else
        {
            prec = &(cour->next);
            cour = cour->next;
        }
    }
}


__inline DWORD PopFrontAndFree(myList * l)
{
    myNode * res = NULL;
	DWORD data = 0;
    
    if(l->head && l->head == l->tail)
    {
        res = l->head;
        l->head = NULL;
        l->tail = NULL;
        l->length = 0;
    }
    else if(l->head)
    {
        res = l->head;
        l->head = res->next;
        l->length--;
    }
	
	//win2000 finds bug here!!!!
	
    if(res) {
		data = res->data;
        HeapFree(hHeap, 0, res);
	}

	return data;
}


__inline void PopBackAndFree(myList * l)
{
    myNode *res = NULL, *cour = NULL, *prec = NULL;
    
    if(l->head && l->head == l->tail)
    {
        res = l->head;
        l->head = NULL;
        l->tail = NULL;
        l->length = 0;
    }
    else if(l->tail)
    {
        cour = l->head;
        prec = l->head;
        while(cour->next)
        {
            prec = cour;
            cour = cour->next;
        }
        
        res = l->tail;
        l->tail = prec;
        l->length--;
    }
    
    if(res)
        HeapFree(hHeap, 0, res);
}



__inline myNode * PopFront(myList * l)
{
    myNode * res = NULL;
    
    if(l->head && l->head == l->tail)
    {
        res = l->head;
        l->head = NULL;
        l->tail = NULL;
        l->length = 0;
    }
    else if(l->head)
    {
        res = l->head;
        l->head = res->next;
        l->length--;
    }
    
    return res;
}


__inline myNode * PopBack(myList * l)
{
    myNode *res = NULL, *cour = NULL, *prec = NULL;
    
    if(l->head && l->head == l->tail)
    {
        res = l->head;
        l->head = NULL;
        l->tail = NULL;
        l->length = 0;
    }
    else if(l->tail)
    {
        cour = l->head;
        prec = l->head;
        while(cour->next)
        {
            prec = cour;
            cour = cour->next;
        }
        
        res = l->tail;
        prec->next = NULL;
        l->tail = prec;
        l->length--;
    }
    
    return res;
}


__inline BOOL IsInListByVal(myList * l, DWORD data)
{
    myNode * cour = NULL;
    BOOL found = FALSE;
    
    cour = l->head;
    while(cour && !found)
    {
        found = (cour->data == data);
        cour = cour->next;
    }
    
    return found;
}


__inline BOOL IsInListByAddr(myList * l, myNode * node)
{
    myNode * cour = NULL;
    BOOL found = FALSE;
    
    cour = l->head;
    while(cour && !found)
    {
        found = (cour == node);
        cour = cour->next;
    }
    
    return found;
}


__inline void FreeList(myList ** l)
{
    while((*l)->length)
        PopFrontAndFree(*l);
    
    CloseHandle((*l)->lockMutex);
    HeapFree(hHeap, 0, *l);
    *l = NULL;
}


#endif