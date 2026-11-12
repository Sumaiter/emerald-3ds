#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__
#include <stdint.h>
typedef int32_t _LOCK_T;
typedef struct __lock_t { _LOCK_T lock; uint32_t thread_tag; uint32_t counter; } _LOCK_RECURSIVE_T;
typedef uint32_t _COND_T;
#define __LOCK_INITIALIZER ((_LOCK_T)0)
#define __LOCK_INITIALIZER_RECURSIVE ((_LOCK_RECURSIVE_T){__LOCK_INITIALIZER,0,0})
#define __COND_INITIALIZER ((_COND_T)0)
static __inline void __libc_lock_init(_LOCK_T *lock) { *lock = __LOCK_INITIALIZER; }
static __inline void __libc_lock_close(_LOCK_T *lock) { (void)lock; }
static __inline void __libc_lock_init_recursive(_LOCK_RECURSIVE_T *lock) { *lock = __LOCK_INITIALIZER_RECURSIVE; }
static __inline void __libc_lock_close_recursive(_LOCK_RECURSIVE_T *lock) { (void)lock; }
static __inline void __libc_lock_acquire(_LOCK_T *lock) { (void)lock; }
static __inline void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock) { (void)lock; }
static __inline void __libc_lock_release(_LOCK_T *lock) { (void)lock; }
static __inline void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *lock) { (void)lock; }
static __inline int __libc_lock_try_acquire(_LOCK_T *lock) { (void)lock; return 0; }
static __inline int __libc_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock) { (void)lock; return 0; }
static __inline int __libc_cond_init(_COND_T *cond) { *cond = __COND_INITIALIZER; return 0; }
static __inline void __libc_cond_close(_COND_T *cond) { (void)cond; }
static __inline int __libc_cond_signal(_COND_T *cond) { (void)cond; return 0; }
static __inline int __libc_cond_broadcast(_COND_T *cond) { (void)cond; return 0; }
static __inline int __libc_cond_wait(_COND_T *cond, _LOCK_T *lock, uint64_t timeout_ns) { (void)cond; (void)lock; (void)timeout_ns; return 0; }
static __inline int __libc_cond_wait_recursive(_COND_T *cond, _LOCK_RECURSIVE_T *lock, uint64_t timeout_ns) { (void)cond; (void)lock; (void)timeout_ns; return 0; }
#endif
