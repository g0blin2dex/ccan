/* Licensed under BSD-MIT - see LICENSE file for details */
#ifndef CCAN_TAL_H
#define CCAN_TAL_H
#include "config.h"
#include <ccan/compiler/compiler.h>
#include <ccan/likely/likely.h>
#include <ccan/typesafe_cb/typesafe_cb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

/**
 * tal_t - convenient alias for void to mark tal pointers.
 *
 * Since any pointer can be a tal-allocated pointer, it's often
 * useful to use this typedef to mark them explicitly.
 */
typedef void tal_t;

/**
 * TAL_TAKE - fake tal_t to indicate function will own arguments.
 *
 * Various functions take a context on which to allocate: if you use
 * TAL_TAKE there instead, it means that the argument(s) are actually
 * tal objects.  The returned value will share the same parent; it may
 * even be the same pointer as the arguments.  The arguments themselves
 * will be reused, freed, or made a child of the return value: they are
 * no longer valid for external use.
 */
#define TAL_TAKE ((tal_t *)-2L)

/**
 * tal - basic allocator function
 * @ctx: NULL, or tal allocated object to be parent.
 * @type: the type to allocate.
 *
 * Allocates a specific type, with a given parent context.
 */
#define tal(ctx, type) tal_arr((ctx), type, 1)

/**
 * talz - zeroing allocator function
 * @ctx: NULL, or tal allocated object to be parent.
 * @type: the type to allocate.
 *
 * Equivalent to tal() followed by memset() to zero.
 */
#define talz(ctx, type) tal_arrz((ctx), type, 1)

/**
 * tal_free - free a tal-allocated pointer.
 * @p: NULL, or tal allocated object to free.
 *
 * This calls the destructors for p (if any), then does the same for all its
 * children (recursively) before finally freeing the memory.
 */
void tal_free(const tal_t *p);

/**
 * tal_arr - allocate an array of objects.
 * @ctx: NULL, or tal allocated object to be parent.
 * @type: the type to allocate.
 * @count: the number to allocate.
 */
#define tal_arr(ctx, type, count) \
	((type *)tal_alloc_((ctx), tal_sizeof_(sizeof(type), (count)), false))

/**
 * tal_arrz - allocate an array of zeroed objects.
 * @ctx: NULL, or tal allocated object to be parent.
 * @type: the type to allocate.
 * @count: the number to allocate.
 */
#define tal_arrz(ctx, type, count) \
	((type *)tal_alloc_((ctx), tal_sizeof_(sizeof(type), (count)), true))

/**
 * tal_resize - enlarge or reduce a tal_arr(z).
 * @p: The tal allocated array to resize.
 * @count: the number to allocate.
 *
 * This returns the new pointer, or NULL (and destroys the old one)
 * on failure.
 */
#define tal_resize(p, count) \
	((tal_typeof(p) tal_realloc_((p), tal_sizeof_(sizeof(*p), (count)))))

/**
 * tal_steal - change the parent of a tal-allocated pointer.
 * @ctx: The new parent.
 * @ptr: The tal allocated object to move.
 *
 * This may need to perform an allocation, in which case it may fail; thus
 * it can return NULL, otherwise returns @ptr.
 *
 * Weird macro avoids gcc's 'warning: value computed is not used'.
 */
#if HAVE_STATEMENT_EXPR
#define tal_steal(ctx, ptr) \
	({ (tal_typeof(ptr) tal_steal_((ctx),(ptr))); })
#else
#define tal_steal(ctx, ptr) \
	(tal_typeof(ptr) tal_steal_((ctx),(ptr)))
#endif

/**
 * tal_add_destructor - add a callback function when this context is destroyed.
 * @ptr: The tal allocated object.
 * @function: the function to call before it's freed.
 */
#define tal_add_destructor(ptr, function)				      \
	tal_add_destructor_((ptr), typesafe_cb(void, void *, (function), (ptr)))

/**
 * tal_set_name - attach a name to a tal pointer.
 * @ptr: The tal allocated object.
 * @name: The name to use.
 *
 * The name is copied, unless we're certain it's a string literal.
 */
#define tal_set_name(ptr, name)				      \
    tal_set_name_((ptr), (name), TAL_IS_LITERAL(name))

/**
 * tal_name - get the name for a tal pointer.
 * @ptr: The tal allocated object.
 *
 * Returns NULL if no name has been set.
 */
const char *tal_name(const tal_t *ptr);

/**
 * tal_first - get the first tal object child.
 * @root: The tal allocated object to start with, or NULL.
 *
 * Returns NULL if there are no children.
 */
tal_t *tal_first(const tal_t *root);

/**
 * tal_next - get the next tal object child.
 * @root: The tal allocated object to start with, or NULL.
 * @prev: The return value from tal_first or tal_next.
 *
 * Returns NULL if there are no more children.  This should be safe to
 * call on an altering tree unless @prev is no longer a descendent of
 * @root.
 */
tal_t *tal_next(const tal_t *root, const tal_t *prev);

/**
 * tal_parent - get the parent of a tal object.
 * @ctx: The tal allocated object.
 *
 * Returns the parent, which may be NULL.
 */
tal_t *tal_parent(const tal_t *ctx);

/**
 * tal_memdup - duplicate memory.
 * @ctx: NULL, or tal allocated object to be parent (or TAL_TAKE).
 * @p: the memory to copy
 * @n: the number of bytes.
 *
 */
void *tal_memdup(const tal_t *ctx, const void *p, size_t n);

/**
 * tal_strdup - duplicate a string
 * @ctx: NULL, or tal allocated object to be parent (or TAL_TAKE).
 * @p: the string to copy
 */
char *tal_strdup(const tal_t *ctx, const char *p);

/**
 * tal_strndup - duplicate a limited amount of a string.
 * @ctx: NULL, or tal allocated object to be parent (or TAL_TAKE).
 * @p: the string to copy
 * @n: the maximum length to copy.
 *
 * Always gives a nul-terminated string, with strlen() <= @n.
 */
char *tal_strndup(const tal_t *ctx, const char *p, size_t n);

/**
 * tal_asprintf - allocate a formatted string
 * @ctx: NULL, or tal allocated object to be parent (or TAL_TAKE).
 * @fmt: the printf-style format.
 *
 * If @ctx is TAL_TAKE, @fmt is freed and its parent will be the parent
 * of the return value.
 */
char *tal_asprintf(const tal_t *ctx, const char *fmt, ...) PRINTF_FMT(2,3);

/**
 * tal_vasprintf - allocate a formatted string (va_list version)
 * @ctx: NULL, or tal allocated object to be parent (or TAL_TAKE).
 * @fmt: the printf-style format.
 * @va: the va_list containing the format args.
 *
 * If @ctx is TAL_TAKE, @fmt is freed and its parent will be the parent
 * of the return value.
 */
char *tal_vasprintf(const tal_t *ctx, const char *fmt, va_list ap)
	PRINTF_FMT(2,0);


/**
 * tal_set_backend - set the allocation or error functions to use
 * @alloc_fn: allocator or NULL (default is malloc)
 * @resize_fn: re-allocator or NULL (default is realloc)
 * @free_fn: free function or NULL (default is free)
 * @error_fn: called on errors or NULL (default is abort)
 *
 * The defaults are set up so tal functions never return NULL, but you
 * can override erorr_fn to change that.  error_fn can return, and is
 * called if alloc_fn or resize_fn fail.
 *
 * If any parameter is NULL, that function is unchanged.
 */
void tal_set_backend(void *(*alloc_fn)(size_t size),
		     void *(*resize_fn)(void *, size_t size),
		     void (*free_fn)(void *),
		     void (*error_fn)(const char *msg));


/**
 * tal_check - set the allocation or error functions to use
 * @ctx: a tal context, or NULL.
 * @errorstr: a string to prepend calls to error_fn, or NULL.
 *
 * This sanity-checks a tal tree (unless NDEBUG is defined, in which case
 * it simply returns true).  If errorstr is not null, error_fn is called
 * when a problem is found, otherwise it is not.
 */
bool tal_check(const tal_t *ctx, const char *errorstr);

#ifdef CCAN_TAL_DEBUG
/**
 * tal_dump - dump entire tal tree.
 *
 * This is a helper for debugging tal itself, which dumps all the tal internal
 * state.
 */
void tal_dump(void);
#endif

/* Internal support functions */
#if HAVE_BUILTIN_CONSTANT_P
#define TAL_IS_LITERAL(str) __builtin_constant_p(str)
#else
#define TAL_IS_LITERAL(str) (sizeof(&*(str)) != sizeof(char *))
#endif

bool tal_set_name_(tal_t *ctx, const char *name, bool literal);

static inline size_t tal_sizeof_(size_t size, size_t count)
{
	/* Multiplication wrap */
        if (count && unlikely(size * count / size != count))
		return (size_t)-1024;

        size *= count;

        /* Make sure we don't wrap adding header. */
        if (size > (size_t)-1024)
		return (size_t)-1024;

        return size;
}

#if HAVE_TYPEOF
#define tal_typeof(ptr) (__typeof__(ptr))
#else
#define tal_typeof(ptr)
#endif

void *tal_alloc_(const tal_t *ctx, size_t bytes, bool clear);

tal_t *tal_steal_(const tal_t *new_parent, const tal_t *t);

void *tal_realloc_(tal_t *ctx, size_t size);

bool tal_add_destructor_(tal_t *ctx, void (*destroy)(void *me));

#endif /* CCAN_TAL_H */
