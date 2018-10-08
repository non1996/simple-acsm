#ifndef LINKLIST_H
#define LINKLIST_H

#include "pattern_match.h"

class_decl(list_iterator);
class_decl(list_node);

class(list) {
	list_node *head;
	list_node *tail;
	list_node *invalid;
	uint32_t size;
};

constructor(list);
distructor(list);

void list_clear(list *self);
bool list_empty(list *self);
size_t list_size(list *self);
void list_push_back(list *self, void *data);
void list_push_front(list *self, void *data);
void *list_pop_front(list *self);
void *list_pop_back(list *self);
void *list_front(list *self);
void *list_back(list *self);
list_iterator *list_begin(list *self);
list_iterator *list_erase(list *self, list_iterator *iter);
list_iterator *list_insert_behind(list *self, list_iterator *iter, void *data);
list_iterator *list_insert_front(list *self, list_iterator *iter, void *data);

void list_iterator_delete(list_iterator *self);
bool list_iterator_getend(list_iterator *self);
void list_iterator_next(list_iterator *self);
void list_iterator_prev(list_iterator *self);
void *list_iterator_get(list_iterator *self);


#define list_foreach_begin(type, elem, l)	\
{												\
	list_iterator *iter; 						\
	for (iter = list_begin(l);				\
		!list_iterator_getend(iter);			\
		 list_iterator_next(iter)) {			\
		type *elem = (type*)list_iterator_get(iter);

#define list_foreach_end						\
	}											\
	list_iterator_delete(iter);								\
}

#define list_foreach_return(value)				\
do {											\
	list_iterator_delete(iter);								\
    return value;								\
}while (0)

#define list_clear_deep(type, list)				\
do {											\
	list_foreach_begin(type, elem, (list)) {	\
		delete(type, elem);					\
	}list_foreach_end							\
	list(clear);								\
}while (0)

#endif //LINKLIST_H