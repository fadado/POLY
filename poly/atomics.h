#ifdef __STDC_NO_ATOMICS__
#error I need <stdatomic.h>. Sorry :-(
#endif
#ifndef POLY_SYNCOP_H
#define POLY_SYNCOP_H

#include <stdatomic.h>

// internal macros
#define M_1_2(_1,_2,NAME,...) NAME
#define M_2_3(_1,_2,_3,NAME,...) NAME
#define M_3_5(_1,_2,_3,_4,_5,NAME,...) NAME

////////////////////////////////////////////////////////////////////////
// Shared registers
////////////////////////////////////////////////////////////////////////

// void atomic_init(volatile A *obj, C value);

// _Bool atomic_is_lock_free(const volatile A *obj);

/*
#if ATOMIC_BOOL_LOCK_FREE
#if ATOMIC_CHAR_LOCK_FREE
#if ATOMIC_CHAR16_T_LOCK_FREE
#if ATOMIC_CHAR32_T_LOCK_FREE
#if ATOMIC_WCHAR_T_LOCK_FREE
#if ATOMIC_SHORT_LOCK_FREE
#if ATOMIC_INT_LOCK_FREE
#if ATOMIC_LONG_LOCK_FREE
#if ATOMIC_LLONG_LOCK_FREE
#if ATOMIC_POINTER_LOCK_FREE
*/

// memory_order mo = ...;
enum {
	RELAXED = memory_order_relaxed,
	CONSUME = memory_order_consume,
	ACQUIRE = memory_order_acquire,
	RELEASE = memory_order_release,
	ACQ_REL = memory_order_acq_rel,
	SEQ_CST = memory_order_seq_cst
};

////////////////////////////////////////////////////////////////////////
// Atomic flag
////////////////////////////////////////////////////////////////////////

// { f := 0 }
// atomic_flag f = ATOMIC_FLAG_INIT;

// _Bool atomic_flag_test_and_set(volatile atomic_flag *shared);
// _Bool atomic_flag_test_and_set_explicit(volatile atomic_flag *shared, memory_order order);
//
// { v := *shared; *shared := 1; (v = FLAG_STATE_SET) }
#define TAS(...)\
    M_1_2(__VA_ARGS__, atomic_flag_test_and_set_explicit, atomic_flag_test_and_set)(__VA_ARGS__)

// void atomic_flag_clear(volatile atomic_flag *shared);
// void atomic_flag_clear_explicit(volatile atomic_flag *shared, memory_order order);
//
// { *shared := FLAG_STATE_CLEAR }
#define CLEAR(...)\
    M_1_2(__VA_ARGS__, atomic_flag_clear_explicit, atomic_flag_clear)(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////
// Atomic registers
////////////////////////////////////////////////////////////////////////

// void atomic_store(volatile A *shared, C desired);
// void atomic_store_explicit(volatile A *shared, C desired, memory_order order);
//
// { *shared := desired }
#define STORE(...)\
    M_2_3(__VA_ARGS__, atomic_store_explicit, atomic_store)(__VA_ARGS__)

// C atomic_load(const volatile A *shared);
// C atomic_load_explicit(const volatile A *shared, memory_order order);
//
// { *shared }
#define LOAD(...)\
    M_1_2(__VA_ARGS__, atomic_load_explicit, atomic_load)(__VA_ARGS__)

// C atomic_exchange(volatile A *shared, C desired);
// C atomic_exchange_explicit(volatile A *shared, C desired, memory_order order);
//
// { v := *shared; shared := desired; v }
#define SWAP(...)\
    M_2_3(__VA_ARGS__, atomic_exchange_explicit, atomic_exchange)(__VA_ARGS__)

// _Bool atomic_compare_exchange_strong(volatile A *shared, C *expected, C desired);
// _Bool atomic_compare_exchange_strong_explicit(volatile A *shared, C *expected, C desired, memory_order success, memory_order failure);
//
// { t := (*shared = *expected); t ? (*shared := desired) : (*expected := *shared); t }
#define CAS(...)\
    M_3_5(__VA_ARGS__, atomic_compare_exchange_strong_explicit,_4_ignored, atomic_compare_exchange_strong)(__VA_ARGS__)

// _Bool atomic_compare_exchange_weak(volatile A *shared, C *expected, C desired);
// _Bool atomic_compare_exchange_weak_explicit(volatile A *shared, C *expected, C desired, memory_order success, memory_order failure);
//
// { t := (*shared = *expected); t ? (*shared := desired) : (*expected := *shared); t }
#define CASw(...)\
    M_3_5(__VA_ARGS__, atomic_compare_exchange_weak_explicit,_4_ignored, atomic_compare_exchange_weak)(__VA_ARGS__)

/* CAS usage protocol:
 *
 *  static _Atomic(A) shared = ...
 *  ...
 *
 *  C x = LOAD(&shared);
 *  do {
 *      C y = ϕ(x);
 *  } while (!CASw(&shared, &x, y));
 */

////////////////////////////////////////////////////////////////////////
// Fetch and ϕ
////////////////////////////////////////////////////////////////////////

// C atomic_fetch_ϕ(volatile A *shared, M operand);
// C atomic_fetch_ϕ_explicit(volatile A *shared, M operand, memory_order order);
//
// { v := *shared; *shared := v + operand; v }
#define reg_add(...)\
    M_2_3(__VA_ARGS__, atomic_fetch_add_explicit, atomic_fetch_add)(__VA_ARGS__)

// { v := *shared; *shared := v - operand; v }
#define reg_sub(...)\
    M_2_3(__VA_ARGS__, atomic_fetch_sub_explicit, atomic_fetch_sub)(__VA_ARGS__)

// { v := *shared; *shared := v | operand; v }
#define reg_or(...)\
    M_2_3(__VA_ARGS__, atomic_fetch_or_explicit, atomic_fetch_or)(__VA_ARGS__)

// { v := *shared; *shared := v ^ operand; v }
#define reg_xor(...)\
    M_2_3(__VA_ARGS__, atomic_fetch_xor_explicit, atomic_fetch_xor)(__VA_ARGS__)

// { v := *shared; *shared := v & operand; v }
#define reg_and(...)\
    M_2_3(__VA_ARGS__, atomic_fetch_and_explicit, atomic_fetch_and)(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////
// Fences
////////////////////////////////////////////////////////////////////////

// type kill_dependency(type y);

// void atomic_thread_fence(memory_order order);
// void atomic_signal_fence(memory_order order);

////////////////////////////////////////////////////////////////////////
// Extensions
////////////////////////////////////////////////////////////////////////

/* Flag as mutex:
 *
 *  atomic_flag lock = ATOMIC_FLAG_INIT;
 *  flag_flip(&lock);   // lock
 *  ...
 *  flag_clear(&lock);  // unlock
 */

// wait until the state flips from CLEAR to SET
#define FLIP(R,...)     while (TAS((R)__VA_OPT__(,)__VA_ARGS__))
#define flag_flip(R)    FLIP((R), ACQ_REL)

// change state to CLEAR
#define flag_clear(R)   CLEAR(R, RELEASE)

/* Flag as condition:
 *
 *  atomic_INTEGER done = {0};
 *  flag_wait(&done);  || flag_set(&done);
 *  flag_reset(&done); || ...
 */

// wait until the value becomes 1
#define flag_wait(R)    while (0==LOAD((R), ACQUIRE))

// change value to 1 or 0
#define flag_set(R)     STORE((R), 1, RELEASE)
#define flag_reset(R)   STORE((R), 0, RELEASE)

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
