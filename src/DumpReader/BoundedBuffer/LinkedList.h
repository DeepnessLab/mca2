/*
 *  An implementation for a linked list with queue capabilities
 */

#ifndef LINKED_LIST_HEADER

#define LINKED_LIST_HEADER

#include <pthread.h>

#define LIST_STATUS_DEQUEUE_SUCCESS 0
#define LIST_STATUS_DEQUEUE_FAIL 1
#define LIST_STATUS_DEQUEUE_EMPTY 2

// A list item
typedef struct linked_list_item {
	void *data;						// Pointer to the data this element represents
	struct linked_list_item *prev;	// Pointer to previous list element
	struct linked_list_item *next;	// Pointer to next list element
} LinkedListItem;

// List wrapper struct
typedef struct linked_list {
	struct linked_list_item *first;	// Pointer to the first element in the list
	struct linked_list_item *last;	// Pointer to the last element in the list
	pthread_mutex_t mutex;			// mutex lock for list access
	pthread_cond_t cv;				// Condition variable for empty list
	int done;						// Termination flag
	int use_lock;
	int test_lock;
	int in, out;
} LinkedList;

/*
 * Returns the data of element in given index in given list
 */
void *list_get(LinkedList *list, int index);

/*
 * Inserts an item with the given data into the list as its last element
 */
void list_insert_last(LinkedList *list, void *data);

/*
 * Deletes the item in the given index from the list
 */
void *list_delete_item(LinkedList *list, int index);

/*
 * Returns 1 if the list contains an element whose data is the given string
 */
int list_contains_str(LinkedList *list, char *data);

/*
 * Initializes the list. Allocates any needed OS elements, if any.
 */
void list_init(LinkedList *list);

/*
 * Destroys the list. Deallocate any OS elements that were allocated
 * and frees any existing list item.
 * This function should not free the list itself.
 * When free_data is TRUE (not 0), this function also frees any
 * existing item's data pointer.
 */
void list_destroy(LinkedList *list, int free_data);

/*
 * Dequeues first element from the list and returns its data.
 * This function should block (wait) if there is no item to
 * dequeue. However, it may be iterrupted (signaled) even when the
 * queue is empty, in case of program termination (see list_set_done). In such case,
 * it should return NULL pointer and set status parameter to LIST_STATUS_DEQUEUE_FAIL.
 * In regular cases it should set status to LIST_STATUS_DEQUEUE_SUCCESS
 */
void *list_dequeue(LinkedList *list, int *status);

/*
 * Enqueues an element with the given data as the last one into the list.
 * This function should never block as list size is unbounded.
 * After enqueueing, this function should signal and wake up the next waiting
 * thread for dequeueing (if any).
 */
void list_enqueue(LinkedList *list, void *data);

void list_insert_first(LinkedList *list, void *data);

void list_set_lock(LinkedList *list, int lock);

/*
 * Sets termination flag for given list to TRUE and broadcast a signal to all waiting
 * threads to wake up from their wait on dequeue and fail the dequeue operation.
 */
void list_set_done(LinkedList *list);

int list_is_empty(LinkedList* list);

#endif
