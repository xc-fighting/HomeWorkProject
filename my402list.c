#include "my402list.h"
#include <stdlib.h>

extern int  My402ListLength(My402List* list) {
	//Returns the number of elements in the list
	return list->num_members;
}
extern int  My402ListEmpty(My402List* list) {
	//Returns TRUE if the list is empty. Returns FALSE otherwise.
	if (list->anchor.next == NULL && list->anchor.prev == NULL) {
		return TRUE;
	}
	return FALSE;
}

extern int  My402ListAppend(My402List* list, void* obj) {
	//If list is empty, just add obj to the list. Otherwise, add obj after Last(). 
	//This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.
	if (My402ListEmpty(list) == TRUE) {
		list->anchor.next = (My402ListElem*)malloc(sizeof(My402ListElem));
		if(list->anchor.next==NULL){
			return FALSE;
		}
		My402ListElem* first = list->anchor.next;
		first->obj = obj;
		first->prev = &(list->anchor);
		first->next = &(list->anchor);
		list->anchor.prev = first;
	}
	else {
		//get the old last element in order to append element behind it
		My402ListElem* oldlast = list->anchor.prev;
		//unlink the link between last and anchor
		list->anchor.prev = NULL;
		//append a new last element,assign it with newlast
		oldlast->next = (My402ListElem*)malloc(sizeof(My402ListElem));
		if(oldlast->next==NULL){
			return FALSE;
		}
		My402ListElem* newlast = oldlast->next;
		//assign the obj to it
		newlast->obj = obj;
		newlast->prev = oldlast;
		newlast->next = &(list->anchor);
		list->anchor.prev = newlast;
	}
	//after all of these above,remenmber to add the number of nodes
	list->num_members++;
	return TRUE;
}
extern int  My402ListPrepend(My402List* list, void* obj) {
	//If list is empty, just add obj to the list. Otherwise, add obj before First(). 
	//This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.
	if (My402ListEmpty(list) == TRUE) {
		list->anchor.next = (My402ListElem*)malloc(sizeof(My402ListElem));
		My402ListElem* p = list->anchor.next;
		list->anchor.prev = p;
		p->obj = obj;
		p->next = &(list->anchor);
		p->prev = &(list->anchor);
	}
	else {
		//store the first element
		My402ListElem* first = list->anchor.next;
		list->anchor.next = (My402ListElem*)malloc(sizeof(My402ListElem));
		My402ListElem* newhead = list->anchor.next;
		newhead->obj = obj;
		newhead->next = first;
		newhead->prev = &(list->anchor);
		first->prev = newhead;
	}
	list->num_members++;
	return TRUE;
}

/*
 problem with it,do not need to traverse again.
*/
extern void My402ListUnlink(My402List* list, My402ListElem* item) {
	//Unlink and delete elem from the list. Please do not delete the object pointed to by elem and do not check if elem is on the list.
	if (My402ListEmpty(list) == 1) {
		return;
	}
	My402ListElem* pre = item->prev;
	My402ListElem* next = item->next;
	if (pre == next) {
		list->anchor.next = NULL;
		list->anchor.prev = NULL;
		free(item);
		list->num_members--;
		return;
	}
	else {
		item->prev = NULL;
		item->next = NULL;
		item->obj = NULL;
		free(item);
		pre->next = next;
		next->prev = pre;
		list->num_members--;
		return;
	}			

}
extern void My402ListUnlinkAll(My402List* list) {
	//Unlink and delete all elements from the list and make the list empty. Please do not delete the objects pointed to by the list elements
	if (My402ListEmpty(list) == TRUE) {
		return;
	}
	My402ListElem* p = list->anchor.next;
	while (p != &(list->anchor)) {
		My402ListElem* pre = p->prev;
		My402ListElem* next = p->next;
		if (pre == next) {
			list->anchor.next = NULL;
			list->anchor.prev = NULL;
			free(p);
			list->num_members--;
			return;
		}
		else {
			pre->next = next;
			next->prev = pre;
			p->next = NULL;
			p->prev = NULL;
			//p->obj = NULL;
			free(p);
			p = next;
			list->num_members--;
		}
	}
	return;
}
extern int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* item) {
	/*
	 Insert obj between elem and elem->next. If elem is NULL, then this is the same as Append(). 
	 This function returns TRUE if the operation is performed successfully and returns FALSE otherwise. 
	 Please do not check if elem is on the list.
	*/
	list->num_members++;
	if (item == NULL) {
		int flag=My402ListAppend(list, obj);
		if (flag == TRUE) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	else {
			My402ListElem* nextptr = item->next;
			nextptr->prev = NULL;
			item->next = (My402ListElem*)malloc(sizeof(My402ListElem));
			if (item->next == NULL) {
				return FALSE;
			}
			My402ListElem* cur = item->next;
			cur->obj = obj;
			cur->next = nextptr;
			nextptr->prev = cur;
			cur->prev = item;
			return TRUE;
		
	}
}
extern int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* item) {
	/*
	Insert obj between elem and elem->prev.
	If elem is NULL, then this is the same as Prepend().
	This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.
	Please do not check if elem is on the list.
	
	*/
	list->num_members++;
	if (item == NULL) {
		int flag = My402ListPrepend(list, obj);
		if (flag == 1) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	 }
	else {
			My402ListElem* preptr = item->prev;
			preptr->next = NULL;
			item->prev = (My402ListElem*)malloc(sizeof(My402ListElem));
			if (item->prev == NULL) {
				return FALSE;
			}
			My402ListElem* cur = item->prev;
			cur->obj = obj;
			cur->next = item;
			cur->prev = preptr;
			preptr->next = cur;
			return TRUE;
		
		
	}
}

extern My402ListElem *My402ListFirst(My402List* list) {
	//Returns the first list element or NULL if the list is empty.
	if (My402ListEmpty(list) == TRUE) {
		return NULL;
	}
	return list->anchor.next;
}
extern My402ListElem *My402ListLast(My402List* list) {
	//Returns the last list element or NULL if the list is empty.
	if (My402ListEmpty(list) == TRUE) {
		return NULL;
	}
	return list->anchor.prev;
}
extern My402ListElem *My402ListNext(My402List* list, My402ListElem* item) {
	//Returns elem->next or NULL if elem is the last item on the list. Please do not check if elem is on the list.
	if (item->next == &(list->anchor)) {
		return NULL;
	 }
	return item->next;
}
extern My402ListElem *My402ListPrev(My402List* list, My402ListElem* item) {
	//Returns elem->prev or NULL if elem is the first item on the list. Please do not check if elem is on the list.
	if (item->prev == &(list->anchor)) {
		return NULL;
	 }
	return item->prev;
}

extern My402ListElem *My402ListFind(My402List* list, void* obj) {
	//Returns the list element elem such that elem->obj == obj. Returns NULL if no such element can be found.
	if (My402ListEmpty(list) == TRUE) {
		return NULL;
	}
	else {
		My402ListElem* p = list->anchor.next;
		while (p != &(list->anchor)) {
			if (p->obj == obj) {
				return p;
			}
			else {
				p = p->next;
			}
		}
		return NULL;
	}
}

extern int My402ListInit(My402List* list) {
	//Initialize the list into an empty list. Returns TRUE if all is well and returns FALSE if there is an error initializing the list.
	list->num_members = 0;
	list->anchor.next = NULL;
	list->anchor.prev = NULL;
	list->anchor.obj = NULL;
	return TRUE;
}
/*extern void traverse(My402List* list) {
	if (My402ListEmpty(list) == TRUE) {
		return;
	}
	else {
		My402ListElem* p = list->anchor.next;
		while (p != &(list->anchor)) {
			printf("%d ", *((int*)p->obj));
			p = p->next;
		}
		printf("\n");
	}
}*/
/*void InsertionSort(My402List *list) {
	if (My402ListEmpty(list) == TRUE || My402ListLength(list) == 1) {
		return;
	}
	//My402ListElem* head = list->anchor.next;
	My402ListElem* end = &(list->anchor);
	My402ListElem* cur = list->anchor.next;
	My402ListElem* p = cur->next;
	while (p != end) {
		if ( *((int*)(p->obj)) > *((int*)(cur->obj))) {
			cur = cur->next;
			p = p->next;
		}
		else {
			//unlink p
			//store the value of p first
			void* temp = p->obj;
			My402ListElem *q = list->anchor.next;
			while (*((int*)(q->obj)) < *((int*)(temp))) {
				q = q->next;
			}
			My402ListUnlink(list, p);
			My402ListInsertBefore(list, temp, q);
			p = cur->next;
		}
	}
}*/
