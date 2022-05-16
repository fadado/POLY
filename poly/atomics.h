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

// atomic_flag f = ATOMIC_FLAG_INIT; // assume equal to 0

// _Bool atomic_flag_test_and_set(volatile atomic_flag *shared);
// _Bool atomic_flag_test_and_set_explicit(volatile atomic_flag *shared, memory_order order);
//
// { v := *shared; *shared := 1; (v = 1) }
#define TAS(...)\
    M_1_2(__VA_ARGS__, atomic_flag_test_and_set_explicit, atomic_flag_test_and_set)(__VA_ARGS__)

// void atomic_flag_clear(volatile atomic_flag *shared);
// void atomic_flag_clear_explicit(volatile atomic_flag *shared, memory_order order);
//
// { *shared := 0 }
#define CLEAR(...)\
    M_1_2(__VA_ARGS__, atomic_flag_clear_explicit, atomic_flag_clear)(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////
// Scalar registers
////////////////////////////////////////////////////////////////////////
 
// static atomic(A) shared = {0};

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
// Idioms
////////////////////////////////////////////////////////////////////////

/*
 * atomic_flag f = ATOMIC_FLAG_INIT;
 *
 * if (TAS(&f)) REST else FIRST
 * while (TAS(&f)) REST; FIRST
 *
 * #define acquire(R)     FLIP(R, ACQ_REL)
 * #define release(R)     CLEAR(R, RELEASE)
 */

// spins until the flag state flips from CLEAR to SET
#define FLIP(R,...)      while (TAS(R __VA_OPT__(,)__VA_ARGS__))

/*
 * atomic_X r = {0};
 *
 * if (SWAP(&r,1)) REST else FIRST
 * while (SWAP(&r,1)) REST; FIRST
 *
 * #define acquire(R)      reg_flip(R, ACQUIRE, ACQ_REL)
 * #define release(R)      reg_clear(R, RELEASE)
 */

// spins until the register flips from 0 to 1
#define reg_flip(R,MO,RMW)  while (LOAD(R, MO) || SWAP(R, 1, RMW))

// change the register value to 0
#define reg_clear(R,MO)     STORE(R, 0, MO)

/*
 * atomic_X r = {1};
 *
 * #define wait(e)      reg_wait(&(e), 1, ACQUIRE)
 * #define signal(e)    reg_clear(&(e), RELEASE)
 */

// wait until the value != V
#define reg_wait(R,V,MO)    while ((V) == LOAD(R, MO))

/*
 * STORE(&r,v)      r := v
 * v = LOAD(&r)     v := r
 * v = SWAP(&r,v)   r :=: v
 */

/* CAS usage protocol:
 *
 *  static atomic(A) shared = ...
 *  ...
 *
 *  // shared := ϕ(shared)
 *  C x = LOAD(&shared, ACQUIRE);
 *  do { C y = ϕ(x); } while (!CASw(&shared, &x, y, ACQ_REL));
 */

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
