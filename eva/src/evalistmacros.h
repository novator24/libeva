
/* We define three data structures:
 * 1.  a Stack.  a singly-ended, singly-linked list.
 * 2.  a Queue.  a doubly-ended, singly-linked list.
 * 3.  a List.  a doubly-ended, doubly-linked list.
 *
 * Stack operations:
 *    PUSH(stack, node)                                        [O(1)]
 *    POP(stack, rv_node)                                      [O(1)]
 *    INSERT_AFTER(stack, above_node, new_node)                [O(1)]
 *    IS_EMPTY(stack)                                          [O(1)]
 *    REVERSE(stack)                                           [O(N)]
 *    SORT(stack, comparator)                                  [O(NlogN)]
 *    GET_BOTTOM(stack, rv_node)                               [O(N)]
 * Queue operations:
 *    ENQUEUE(queue, node)                                     [O(1)]
 *    DEQUEUE(queue, rv_node)                                  [O(1)]
 *    PREPEND(queue, node)                                     [O(1)]
 *    IS_EMPTY(queue)                                          [O(1)]
 *    REVERSE(queue)                                           [O(N)]
 *    SORT(queue, comparator)                                  [O(NlogN)] 
 * List operations:
 *    PREPEND(list, node)                                      [O(1)]
 *    APPEND(list, node)                                       [O(1)]
 *    REMOVE_FIRST(list)                                       [O(1)]
 *    REMOVE_LAST(list)                                        [O(1)]
 *    REMOVE(list, node)                                       [O(1)]
 *    INSERT_AFTER(list, position_node, new_node)              [O(1)]
 *    INSERT_BEFORE(list, position_node, new_node)             [O(1)]
 *    IS_EMPTY(list)                                           [O(1)]
 *    REVERSE(list)                                            [O(N)]
 *    SORT(list)                                               [O(NlogN)]
 *
 * note: the SORT operation is stable, i.e.  if two
 * elements are equal according to the comparator,
 * then their relative order in the list
 * will be preserved.
 *
 * In the above, 'stack', 'queue', and 'list' are
 * a comma-separated list of arguments, actually.
 * In particular:
 *   stack => NodeType*, top, next_member_name
 *   queue => NodeType*, head, tail, next_member_name
 *   list => NodeType*, first, last, prev_member_name, next_member_name
 * We recommend making macros that end in GET_STACK_ARGS, GET_QUEUE_ARGS,
 * and GET_LIST_ARGS that return the relevant N-tuples.
 */

#define EVA_LOG2_MAX_LIST_SIZE          (GLIB_SIZEOF_SIZE_T*8)

/* --- Stacks --- */
#define EVA_STACK_PUSH(stack, node) EVA_STACK_PUSH_(stack, node)
#define EVA_STACK_POP(stack, rv_node) EVA_STACK_POP_(stack, rv_node)
#define EVA_STACK_INSERT_AFTER(stack, above_node, new_node) \
        EVA_STACK_INSERT_AFTER_(stack, above_node, new_node)
#define EVA_STACK_IS_EMPTY(stack) EVA_STACK_IS_EMPTY_(stack)
#define EVA_STACK_REVERSE(stack) EVA_STACK_REVERSE_(stack)
#define EVA_STACK_FOREACH(stack, iter_var, code) EVA_STACK_FOREACH_(stack, iter_var, code)
#define EVA_STACK_SORT(stack, comparator) EVA_STACK_SORT_(stack, comparator)
#define EVA_STACK_GET_BOTTOM(stack, rv_node) EVA_STACK_GET_BOTTOM_(stack, rv_node)

#define EVA_STACK_PUSH_(type, top, next, node) 				\
  G_STMT_START{								\
    type _eva_tmp = (node);                                             \
    _eva_tmp->next = (top);						\
    (top) = _eva_tmp;							\
  }G_STMT_END
#define EVA_STACK_POP_(type, top, next, rv_node) 	                \
  G_STMT_START{								\
    rv_node = (top);							\
    (top) = (top)->next;						\
  }G_STMT_END
#define EVA_STACK_INSERT_AFTER_(type, top, next, above_node, new_node)  \
  G_STMT_START{								\
    type _eva_tmp = (new_node);                                         \
    type _eva_above = (above_node);                                     \
    _eva_tmp->next = _eva_above->next;				        \
    _eva_above->next = _eva_tmp;					\
  }G_STMT_END
#define EVA_STACK_IS_EMPTY_(type, top, next)				\
  ((top) == NULL)
#define EVA_STACK_REVERSE_(type, top, next)				\
  G_STMT_START{								\
    type _eva___prev = NULL;                                            \
    type _eva___at = (top);						\
    while (_eva___at != NULL)						\
      {									\
	type _eva__next = _eva___at->next;				\
        _eva___at->next = _eva___prev;					\
	_eva___prev = _eva___at;					\
	_eva___at = _eva__next;						\
      }									\
    (top) = _eva___prev;						\
  }G_STMT_END
#define EVA_STACK_FOREACH_(type, top, next, iter_var, code)              \
  for (iter_var = top; iter_var != NULL; )                              \
    {                                                                   \
      type _eva__next = iter_var->next;                                 \
      code;                                                             \
      iter_var = _eva__next;                                            \
    }
/* sort algorithm:
 *   in order to implement SORT in a macro, it cannot use recursion.
 *   but that's ok because you can just manually implement a stack,
 *   which is probably faster anyways.
 */
#define EVA_STACK_SORT_(type, top, next, comparator)			\
  G_STMT_START{								\
    type _eva_stack[EVA_LOG2_MAX_LIST_SIZE];				\
    guint _eva_stack_size = 0;						\
    guint _eva_i;                                                       \
    type _eva_at;							\
    for (_eva_at = top; _eva_at != NULL; )				\
      {									\
	type _eva_a = _eva_at;						\
	type _eva_b;							\
        type _eva_cur_list;                                             \
        int _eva_comparator_rv;                                         \
	_eva_at = _eva_at->next;					\
	if (_eva_at)							\
	  {								\
	    _eva_b = _eva_at;						\
	    _eva_at = _eva_at->next;					\
	    comparator (_eva_a, _eva_b, _eva_comparator_rv);		\
	    if (_eva_comparator_rv > 0)					\
	      {								\
                /* sort first two elements */                           \
		type _eva_swap = _eva_b;				\
		_eva_b = _eva_a;					\
		_eva_a = _eva_swap;					\
		_eva_a->next = _eva_b;					\
		_eva_b->next = NULL;					\
	      }								\
	    else							\
              {                                                         \
                /* first two elements already sorted */                 \
	        _eva_b->next = NULL;					\
              }                                                         \
	  }								\
	else								\
	  {								\
            /* only one element remains */                              \
	    _eva_a->next = NULL;					\
            _eva_at = NULL;                                             \
	  }								\
	_eva_cur_list = _eva_a;						\
									\
	/* merge _eva_cur_list up the stack */				\
	for (_eva_i = 0; TRUE; _eva_i++)				\
	  {								\
	    /* expanding the stack is marked unlikely,         */	\
	    /* since in the case it matters (where the number  */	\
	    /* of elements is big), the stack rarely grows.    */	\
	    if (G_UNLIKELY (_eva_i == _eva_stack_size))                 \
	      {                                                         \
		_eva_stack[_eva_stack_size++] = _eva_cur_list;          \
		break;                                                  \
	      }                                                         \
	    else if (_eva_stack[_eva_i] == NULL)                        \
	      {                                                         \
		_eva_stack[_eva_i] = _eva_cur_list;                     \
		break;                                                  \
	      }                                                         \
	    else                                                        \
	      {                                                         \
		/* Merge _eva_stack[_eva_i] and _eva_cur_list. */       \
		type _eva_merge_list = _eva_stack[_eva_i];              \
                type _eva_new_cur_list;                                 \
		_eva_stack[_eva_i] = NULL;                              \
                                                                        \
                _EVA_MERGE_NONEMPTY_LISTS (_eva_merge_list,             \
                                           _eva_cur_list,               \
                                           _eva_new_cur_list,           \
                                           type, next, comparator);     \
                _eva_cur_list = _eva_new_cur_list;			\
                _eva_stack[_eva_i] = NULL;				\
              }                                                         \
	  }								\
      }									\
                                                                        \
    /* combine all the elements on the stack into a final output */     \
    top = NULL;                                                         \
    for (_eva_i = 0; _eva_i < _eva_stack_size; _eva_i++)		\
      if (_eva_stack[_eva_i] != NULL)                                   \
        {                                                               \
          if (top == NULL)                                              \
            top = _eva_stack[_eva_i];                                   \
          else                                                          \
            {                                                           \
              type _eva_new_top;                                        \
              _EVA_MERGE_NONEMPTY_LISTS (_eva_stack[_eva_i],            \
                                         top,                           \
                                         _eva_new_top,                  \
                                         type, next, comparator);       \
              top = _eva_new_top;                                       \
            }                                                           \
        }                                                               \
  }G_STMT_END

#define EVA_STACK_GET_BOTTOM_(type, top, next, rv_node)                  \
  G_STMT_START{                                                         \
    rv_node = top;                                                      \
    if (rv_node != NULL)                                                \
      while (rv_node->next)                                             \
        rv_node = rv_node->next;                                        \
  }G_STMT_END

/* --- Queues --- */
#define EVA_QUEUE_ENQUEUE(queue, node) EVA_QUEUE_ENQUEUE_(queue, node)
#define EVA_QUEUE_DEQUEUE(queue, rv_node) EVA_QUEUE_DEQUEUE_(queue, rv_node) 
#define EVA_QUEUE_PREPEND(queue, node) EVA_QUEUE_PREPEND_(queue, node) 
#define EVA_QUEUE_IS_EMPTY(queue) EVA_QUEUE_IS_EMPTY_(queue) 
#define EVA_QUEUE_REVERSE(queue) EVA_QUEUE_REVERSE_(queue) 
#define EVA_QUEUE_SORT(queue, comparator) EVA_QUEUE_SORT_(queue, comparator) 

#define EVA_QUEUE_ENQUEUE_(type, head, tail, next, node)                \
  G_STMT_START{                                                         \
    type _eva_tmp = (node);                                             \
    if (tail)                                                           \
      tail->next = _eva_tmp;                                            \
    else                                                                \
      head = _eva_tmp;                                                  \
    tail = _eva_tmp;                                                    \
    node->next = NULL;                                                  \
  }G_STMT_END
#define EVA_QUEUE_DEQUEUE_(type, head, tail, next, rv_node)             \
  G_STMT_START{                                                         \
    rv_node = head;                                                     \
    if (head)                                                           \
      {                                                                 \
        head = head->next;                                              \
        if (head == NULL)                                               \
          tail = NULL;                                                  \
      }                                                                 \
  }G_STMT_END
#define EVA_QUEUE_PREPEND_(type, head, tail, next, node)                \
  G_STMT_START{                                                         \
    type _eva_tmp = (node);                                             \
    _eva_tmp->next = head;                                              \
    head = _eva_tmp;                                                    \
    if (tail == NULL)                                                   \
      tail = head;                                                      \
  }G_STMT_END

#define EVA_QUEUE_IS_EMPTY_(type, head, tail, next)                     \
  ((head) == NULL)

#define EVA_QUEUE_REVERSE_(type, head, tail, next)                      \
  G_STMT_START{                                                         \
    type _eva_queue_new_tail = head;                                    \
    EVA_STACK_REVERSE_(type, head, next);                               \
    tail = _eva_queue_new_tail;                                         \
  }G_STMT_END

#define EVA_QUEUE_SORT_(type, head, tail, next, comparator)             \
  G_STMT_START{                                                         \
    EVA_STACK_SORT_(type, head, next, comparator);                      \
    EVA_STACK_GET_BOTTOM_(type, head, next, tail);                      \
  }G_STMT_END

/* --- List --- */
#define EVA_LIST_PREPEND(list, node) EVA_LIST_PREPEND_(list, node)
#define EVA_LIST_APPEND(list, node) EVA_LIST_APPEND_(list, node)
#define EVA_LIST_REMOVE_FIRST(list) EVA_LIST_REMOVE_FIRST_(list)
#define EVA_LIST_REMOVE_LAST(list) EVA_LIST_REMOVE_LAST_(list)
#define EVA_LIST_REMOVE(list, node) EVA_LIST_REMOVE_(list, node)
#define EVA_LIST_INSERT_AFTER(list, at, node) EVA_LIST_INSERT_AFTER_(list, at, node)
#define EVA_LIST_INSERT_BEFORE(list, at, node) EVA_LIST_INSERT_BEFORE_(list, at, node)
#define EVA_LIST_IS_EMPTY(list) EVA_LIST_IS_EMPTY_(list)
#define EVA_LIST_REVERSE(list) EVA_LIST_REVERSE_(list)
#define EVA_LIST_SORT(list, comparator) EVA_LIST_SORT_(list, comparator)

#define EVA_LIST_PREPEND_(type, first, last, prev, next, node)          \
  G_STMT_START{                                                         \
    type _eva_tmp = (node);                                             \
    if (first)                                                          \
      first->prev = (_eva_tmp);                                         \
    else                                                                \
      last = (_eva_tmp);                                                \
    node->next = first;                                                 \
    node->prev = NULL;                                                  \
    first = node;                                                       \
  }G_STMT_END
#define EVA_LIST_APPEND_(type, first, last, prev, next, node)           \
  EVA_LIST_PREPEND_(type, last, first, next, prev, node)
#define EVA_LIST_REMOVE_FIRST_(type, first, last, prev, next)           \
  G_STMT_START{                                                         \
    first = first->next;                                                \
    if (first == NULL)                                                  \
      last = NULL;                                                      \
    else                                                                \
      first->prev = NULL;                                               \
  }G_STMT_END
#define EVA_LIST_REMOVE_LAST_(type, first, last, prev, next)            \
  EVA_LIST_REMOVE_FIRST_(type, last, first, next, prev)
#define EVA_LIST_REMOVE_(type, first, last, prev, next, node)           \
  G_STMT_START{                                                         \
    type _eva_tmp = (node);                                             \
    if (_eva_tmp->prev)                                                 \
      _eva_tmp->prev->next = _eva_tmp->next;                            \
    else                                                                \
      first = _eva_tmp->next;                                           \
    if (_eva_tmp->next)                                                 \
      _eva_tmp->next->prev = _eva_tmp->prev;                            \
    else                                                                \
      last = _eva_tmp->prev;                                            \
  }G_STMT_END

#define EVA_LIST_INSERT_AFTER_(type, first, last, prev, next, at, node) \
  G_STMT_START{                                                         \
    type _eva_at = (at);                                                \
    type _eva_node = (node);                                            \
    _eva_node->prev = _eva_at;                                          \
    _eva_node->next = _eva_at->next;                                    \
    if (_eva_node->next)                                                \
      _eva_node->next->prev = _eva_node;                                \
    else                                                                \
      last = _eva_node;                                                 \
    _eva_at->next = _eva_node;                                          \
  }G_STMT_END
#define EVA_LIST_INSERT_BEFORE_(type, first, last, prev, next, at, node)\
  EVA_LIST_INSERT_AFTER_(type, last, first, next, prev, at, node)
#define EVA_LIST_IS_EMPTY_(type, first, last, prev, next)               \
  ((first) == NULL)
#define EVA_LIST_REVERSE_(type, first, last, prev, next)                \
  G_STMT_START{                                                         \
    type _eva_at = first;                                               \
    first = last;                                                       \
    last = _eva_at;                                                     \
    while (_eva_at)                                                     \
      {                                                                 \
        type _eva_old_next = _eva_at->next;                             \
        _eva_at->next = _eva_at->prev;                                  \
        _eva_at->prev = _eva_old_next;                                  \
        _eva_at = _eva_old_next;                                        \
      }                                                                 \
  }G_STMT_END
#define EVA_LIST_SORT_(type, first, last, prev, next, comparator)       \
  G_STMT_START{                                                         \
    type _eva_prev = NULL;                                              \
    type _eva_at;                                                       \
    EVA_STACK_SORT_(type, first, next, comparator);                     \
    for (_eva_at = first; _eva_at; _eva_at = _eva_at->next)             \
      {                                                                 \
        _eva_at->prev = _eva_prev;                                      \
        _eva_prev = _eva_at;                                            \
      }                                                                 \
    last = _eva_prev;                                                   \
  }G_STMT_END

/* --- Internals --- */
#define _EVA_MERGE_NONEMPTY_LISTS(a,b,out,type,next,comparator)         \
  G_STMT_START{                                                         \
    type _eva_out_at;                                                   \
    int _eva_comparator_rv;                                             \
    /* merge 'a' and 'b' into 'out' -- in order to make the sort stable,*/  \
    /* always put elements if 'a' first in the event of a tie (i.e. */  \
    /* when comparator_rv==0)                                       */  \
    comparator (a, b, _eva_comparator_rv);                              \
    if (_eva_comparator_rv <= 0)                                        \
      {                                                                 \
        out = a;                                                        \
        a = a->next;                                                    \
      }                                                                 \
    else                                                                \
      {                                                                 \
        out = b;                                                        \
        b = b->next;                                                    \
      }                                                                 \
    _eva_out_at = out;			                                \
    while (a && b)                                                      \
      {                                                                 \
        comparator (a, b, _eva_comparator_rv);                          \
        if (_eva_comparator_rv <= 0)				        \
          {							        \
            _eva_out_at->next = a;		                        \
            _eva_out_at = a;			                        \
            a = a->next;		                                \
          }                                                             \
        else                                                            \
          {							        \
            _eva_out_at->next = b;		                        \
            _eva_out_at = b;			                        \
            b = b->next;	                                        \
          }							        \
      }							                \
    _eva_out_at->next = (a != NULL) ? a : b;                            \
  }G_STMT_END
