#ifndef	_SYS_CDEFS_H
#define	_SYS_CDEFS_H	1

#define _WUR __attribute__((warn_unused_result))

#if (!defined(__GNUC__) || defined(__clang__))
    #define __attribute_malloc(deallocator) __attribute__((malloc))
#else
    #define __attribute_malloc(deallocator) __attribute__((malloc(deallocator)))
#endif

#define __LEAF , __leaf__
#define __LEAF_ATTR __attribute__ ((__leaf__))

#define __THROW	__attribute__ ((__nothrow__ __LEAF))
#define __THROWNL	__attribute__ ((__nothrow__))
#define __NTH(fct)	__attribute__ ((__nothrow__ __LEAF)) fct
#define __NTHNL(fct)  __attribute__ ((__nothrow__)) fct

#define __BEGIN_DECLS
#define __END_DECLS

#define __attribute_nonnull__(params) __attribute__ ((__nonnull__ params))
#define __nonnull(params) __attribute_nonnull__ (params)

#endif
