// -*- C++ -*-

#ifndef _VAMCOMMON_H_
#define _VAMCOMMON_H_

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

extern "C" {

// system-specific information

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1UL << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#define SIZE_T_BIT		(CHAR_BIT * sizeof(size_t))

// output debug messages without invoking malloc()

#define BUF_SZ 1000
void dbprintf(const char *fmt, ...) {
#ifdef DEBUG

#if DB_PRINT_TO_FILE
	static int debug_log = open("debug.log", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (debug_log < 0)
		abort();
#endif

	char buf[BUF_SZ];
	va_list ap;
	va_start(ap, fmt);

	int n = vsnprintf(buf, BUF_SZ, fmt, ap);
	if (n < 0 || n >= BUF_SZ)
		abort();

#if DB_PRINT_TO_FILE
	write(debug_log, buf, n);
#else
	fprintf(stderr, "<dbprintf> %s", buf);
	fflush(stderr);
#endif

#endif
}

// abort if condition is true

#define abort_on(condition) if (condition) { dbprintf("aborted at %s:%d\n", __FILE__, __LINE__); abort(); }

// redefine assert() without invoking malloc()

#ifdef MYASSERT

void __assert_fail (__const char *__assertion, __const char *__file,
					unsigned int __line, __const char *__function) throw () {
	dbprintf("ASSERTION FAILED!\n[%s:%d] in function %s:\n(%s) was false!\n", __file, __line, __function, __assertion);
	abort();
}

#define __ASSERT_FUNCTION __PRETTY_FUNCTION__

#define assert(expr) \
  (__ASSERT_VOID_CAST ((expr) ? 0 :					      \
		       (__assert_fail (__STRING(expr), __FILE__, __LINE__,    \
				       __ASSERT_FUNCTION), 0)))

#else

#include <assert.h>

#endif

// doubly-linked list code copied from Linux kernel source

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)
static inline void __list_add(struct list_head *nnew,
							  struct list_head *prev,
							  struct list_head *next)
{
	next->prev = nnew;
	nnew->next = next;
	nnew->prev = prev;
	prev->next = nnew;
}

static inline void list_add(struct list_head *nnew, struct list_head *head)
{
	__list_add(nnew, head, head->next);
}

static inline void list_add_tail(struct list_head *nnew, struct list_head *head)
{
	__list_add(nnew, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = 0;
	entry->prev = 0;
}

static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)
 
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

// conversion between size and index

#define SIZE_TO_INDEX(size) ((size - 1) / sizeof(double))
#define INDEX_TO_SIZE(index) ((index + 1) * sizeof(double))

};	// end of extern "C"

#endif
