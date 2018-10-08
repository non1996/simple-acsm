#include "list.h"

class(list_node) {
	void *data;
	list_node *next;
	list_node *prev;
};

class(list_iterator) {
	list *list;
	list_node *node;
};

static inline list_node *list_node_new() {
	list_node *instance = mem_alloc_zero(list_node, 1);
	return instance;
}

static inline list_node *list_node_new_arg(void *data) {
	list_node *instance = list_node_new();
	instance->data = data;
	return instance;
}

static inline void list_node_delete(list_node *self) {
	mem_free(self);
}

static inline list_iterator *list_iterator_new(list *l) {
	list_iterator *instance = mem_alloc(list_iterator, 1);
	instance->list = l;
	return instance;
}

void list_iterator_delete(list_iterator * self) {
	mem_free(self);
}

constructor(list) {
	self->invalid = list_node_new();
	constructor_end;
}

distructor(list) {
	list_node_delete(self->invalid);
	list_clear(self);
}

void list_remove_node(list *self, list_node *node) {
	list_node *temp;
	if (self->size == 0)
		return;

	self->size--;

	if (self->size == 0) {
		temp = self->head;
		self->head = self->tail = NULL;
		list_node_delete(temp);
		return;
	}

	if (self->head == node) {
		temp = self->head;
		self->head = self->head->next;
		self->head->prev = NULL;
		list_node_delete(temp);
		return;
	}

	if (self->tail == node) {
		temp = self->tail;
		self->tail = self->tail->prev;
		self->tail->next = NULL;
		list_node_delete(temp);
		return;
	}

	node->prev->next = node->next;
	node->next->prev = node->prev;
	list_node_delete(node);
}

void list_clear(list *self) {
	list_node *temp;
	uint32_t i;

	for (i = 0; i < self->size; ++i) {
		temp = self->head;
		self->head = self->head->next;
		list_node_delete(temp);
	}

	self->tail = self->head = NULL;
	self->size = 0;
}

bool list_empty(list * self) {
	return self->size == 0;
}

size_t list_size(list * self) {
	return self->size;
}

void list_push_back(list *self, void *data) {
	list_node *node = list_node_new_arg(data);

	if (!self->size) {
		self->head = self->tail = node;
	}
	else {
		self->tail->next = node;
		node->prev = self->tail;
		self->tail = node;
	}
	self->size++;
}

void list_push_front(list *self, void *data) {
	list_node *node = list_node_new_arg(data);

	if (!self->size) {
		self->head = self->tail = node;
	}
	else {
		self->head->prev = node;
		node->next = self->head;
		self->head = node;
	}
	self->size++;
}

void *list_pop_front(list *self) {
	void *data;

	if (!self->size)
		return nullptr;

	data = self->head->data;

	list_remove_node(self, self->head);

	return data;
}

void *list_pop_back(list *self) {
	void *data;

	if (!self->size)
		return nullptr;

	data = self->tail->data;
	list_remove_node(self, self->tail);

	return data;
}

void *list_front(list *self) {
	return self->head ? self->head->data : NULL;
}

void *list_back(list *self) {
	return self->tail ? self->tail->data : NULL;
}

list_iterator * list_begin(list * self) {
	list_iterator *iter = list_iterator_new(self);
	iter->node = self->head;
	return NULL;
}

list_iterator *list_erase(list *self, list_iterator *iter) {
	list_node *temp;

	assert(iter->node != self->invalid);

	temp = iter->node;
	if (iter->node == self->head)
		list_iterator_next(iter);
	else
		list_iterator_prev(iter);
	list_remove_node(self, temp);
	return iter;
}

list_iterator *list_insert_behind(list *self, list_iterator *iter, void *data) {
	list_node *node;
	
	if (iter->node == self->invalid)
		return iter;

	node = list_node_new_arg(data);
	if (iter->node == iter->list->tail) {
		list_push_back(iter->list, node);
		return iter;
	}

	iter->node->next->prev = node;
	node->next = iter->node->next;
	node->prev = iter->node;
	iter->node->next = node;

	return iter;
}

list_iterator *list_insert_front(list *self, list_iterator *iter, void *data) {
	list_node *node;
	
	if (iter->node == self->invalid)
		return iter;

	node = list_node_new_arg(data);
	if (iter->node == iter->list->head) {
		list_push_front(iter->list, node);
	}

	iter->node->prev->next = node;
	node->prev = iter->node->prev;
	node->next = iter->node;
	iter->node->prev = node;

	return iter;
}

bool list_iterator_getend(list_iterator *self) {
	return self->node == self->list->invalid;
}

void list_iterator_next(list_iterator *self) {
	if (list_iterator_getend(self))
		return;

	if (self->node == self->list->tail)
		self->node = self->list->invalid;
	else
		self->node = self->node->next;
}

void list_iterator_prev(list_iterator *self) {
	if (list_iterator_getend(self))
		return;

	if (self->node == self->list->head)
		self->node = self->list->invalid;
	else
		self->node = self->node->prev;
}

void *list_iterator_get(list_iterator *self) {
	return self->node != self->list->invalid ?
		self->node->data : NULL;
}


