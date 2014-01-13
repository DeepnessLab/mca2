/*
 *  Implementation of the linked list
 */

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include "LinkedList.h"
#include "../../Common/Flags.h"

// Internal, non-thread-safe function for finding an element in the list
LinkedListItem *find_inner(LinkedList *list, int index) {
	int i = 0;
	LinkedListItem *item = list->first;
	while (item) {
		if (index == i) {
			return item;
		}
		item = item->next;
		i++;
	}
	return NULL;
}

void *list_get(LinkedList *list, int index) {
	pthread_mutex_lock(&(list->mutex));
	LinkedListItem *item = find_inner(list, index);
	if (item) {
		pthread_mutex_unlock(&(list->mutex));
		return item->data;
	}
	pthread_mutex_unlock(&(list->mutex));
	return NULL;
}

void list_insert_last_lock(LinkedList *list, void *data, int lock) {
	if (lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_lock(&(list->mutex));
#else
		while (__sync_lock_test_and_set(&(list->test_lock), 1)) { /* busy wait */ }
#endif
	}
	
	LinkedListItem *item = (LinkedListItem*)malloc(sizeof(LinkedListItem));
	item->data = data;
	
	LinkedListItem *prev = list->last;
	list->last = item;
	if (!list->first)
		list->first = item;
	item->prev = prev;
	item->next = NULL;
	
	if (prev)
		prev->next = item;
	
	list->in++;

	if (lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_unlock(&(list->mutex));
#else
		list->test_lock = 0;
#endif
	}
}

void list_insert_last(LinkedList *list, void *data) {
	list_insert_last_lock(list, data, 1);
}

void *list_delete_item_lock(LinkedList *list, int index, int lock) {
	if (lock)
		pthread_mutex_lock(&(list->mutex));
	
	LinkedListItem *item = find_inner(list, index);
	if (!item) {
		if (lock)
			pthread_mutex_unlock(&(list->mutex));
		return NULL;
	}
	if (item->prev)
		item->prev->next = item->next;
	if (item->next)
		item->next->prev = item->prev;
	
	if (list->first == item)
		list->first = item->next;
	if (list->last == item)
		list->last = item->prev;
	
	void *res = item->data;
	
	list->out++;

	if (lock)
		pthread_mutex_unlock(&(list->mutex));
	
	free(item);
	
	return res;
}

void *list_delete_item(LinkedList *list, int index) {
	return list_delete_item_lock(list, index, 1);
}

int list_contains_str(LinkedList *list, char *data) {
	LinkedListItem *item;

	pthread_mutex_lock(&(list->mutex));
	
	item = list->first;
	
	while (item) {
		if (strcmp(data, (char*)item->data) == 0) {
			pthread_mutex_unlock(&(list->mutex));
			return 1;
		}
		item = item->next;
	}
	
	pthread_mutex_unlock(&(list->mutex));
	return 0;
}

void list_init(LinkedList *list) {
	list->first = NULL;
	list->last = NULL;
	pthread_mutex_init(&(list->mutex), NULL);
	pthread_cond_init(&(list->cv), NULL);
	list->done = 0;
	list->use_lock = 0;
	list->test_lock = 0;

	list->in = list->out = 0;
}

void list_destroy(LinkedList *list, int free_data) {
	void *res;
	while (list->first) {
		res = list_delete_item(list, 0);
		if (free_data && res)
			free(res);
	}
	pthread_mutex_destroy(&(list->mutex));
	pthread_cond_destroy(&(list->cv));
}

void *list_dequeue(LinkedList *list, int *status) {
	if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_lock(&(list->mutex));
#else
		while (__sync_lock_test_and_set(&(list->test_lock), 1)) { /* busy wait */ }
#endif
	}
	
	while (!(list->first)) {
		if (list->done) {
			if (status)
				*status = LIST_STATUS_DEQUEUE_FAIL;
			if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
				pthread_mutex_unlock(&(list->mutex));
#else
				list->test_lock = 0;
#endif
			}
			return NULL;
		}
		
		if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
			pthread_mutex_unlock(&(list->mutex));
#else
			list->test_lock = 0;
#endif
		}

		*status = LIST_STATUS_DEQUEUE_EMPTY;
		return NULL;
		/*
		if (list->use_lock)
			pthread_cond_wait(&(list->cv), &(list->mutex));
		*/
	}
	
	void *res = list_delete_item_lock(list, 0, 0);
	
	if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_unlock(&(list->mutex));
#else
		list->test_lock = 0;
#endif
	}
	
	if (status)
		*status = LIST_STATUS_DEQUEUE_SUCCESS;

	return res;
}

void list_enqueue(LinkedList *list, void *data) {
	//pthread_mutex_lock(&(list->mutex));
	
	list_insert_last_lock(list, data, list->use_lock);
	
	//pthread_cond_signal(&(list->cv));
	
	//pthread_mutex_unlock(&(list->mutex));
}

void list_insert_first(LinkedList *list, void *data) {
	if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_lock(&(list->mutex));
#else
		while (__sync_lock_test_and_set(&(list->test_lock), 1)) { /* busy wait */ }
#endif
	}

	LinkedListItem *item = (LinkedListItem*)malloc(sizeof(LinkedListItem));
	item->data = data;

	LinkedListItem *next = list->first;
	list->first = item;
	if (!list->last)
		list->last = item;
	item->prev = NULL;
	item->next = next;

	if (next)
		next->prev = item;

	list->in++;

	if (list->use_lock) {
#ifdef USE_MUTEX_LOCKS
		pthread_mutex_unlock(&(list->mutex));
#else
		list->test_lock = 0;
#endif
	}
}

void list_set_lock(LinkedList *list, int lock) {
	list->use_lock = lock;
}

void list_set_done(LinkedList *list) {
	//pthread_mutex_lock(&(list->mutex));
	
	list->done = 1;
	
	//pthread_cond_broadcast(&(list->cv));
	
	//pthread_mutex_unlock(&(list->mutex));
}

int list_is_empty(LinkedList* list) {
	return (list->first == NULL) && (list->last == NULL);
}
