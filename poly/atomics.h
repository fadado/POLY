#ifdef __STDC_NO_ATOMICS__
#error I need <stdatomic.h>. Sorry.
#endif
#ifndef SYNCOP_H
#define SYNCOP_H

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
// { v := *shared; *shared := 1; v }
#define TAS(...)    M_1_2(__VA_ARGS__,\
                        atomic_flag_test_and_set_explicit,\
                        atomic_flag_test_and_set)(__VA_ARGS__)

// void atomic_flag_clear(volatile atomic_flag *shared);
// void atomic_flag_clear_explicit(volatile atomic_flag *shared, memory_order order);
//
// { *shared := 0 }
#define CLEAR(...)  M_1_2(__VA_ARGS__,\
                        atomic_flag_clear_explicit,\
                        atomic_flag_clear)(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////
// Shared registers
////////////////////////////////////////////////////////////////////////

// void atomic_store(volatile A *shared, C desired);
// void atomic_store_explicit(volatile A *shared, C desired, memory_order order);
//
// { *shared := desired }
#define STORE(...)  M_2_3(__VA_ARGS__,\
                        atomic_store_explicit,\
                        atomic_store)(__VA_ARGS__)

// C atomic_load(const volatile A *shared);
// C atomic_load_explicit(const volatile A *shared, memory_order order);
//
// { *shared }
#define LOAD(...)   M_1_2(__VA_ARGS__,\
                        atomic_load_explicit,\
                        atomic_load)(__VA_ARGS__)

// C atomic_exchange(volatile A *shared, C desired);
// C atomic_exchange_explicit(volatile A *shared, C desired, memory_order order);
//
// { v := *shared; shared := desired; v }
#define FAS(...)    M_2_3(__VA_ARGS__,\
                        atomic_exchange_explicit,\
                        atomic_exchange)(__VA_ARGS__)

// _Bool atomic_compare_exchange_strong(volatile A *shared, C *expected, C desired);
// _Bool atomic_compare_exchange_strong_explicit(volatile A *shared, C *expected, C desired, memory_order success, memory_order failure);
//
// { t := (*shared = *expected); t ? (*shared := desired) : (*expected := *shared); t }
#define CASS(...)   M_3_5(__VA_ARGS__,\
                        atomic_compare_exchange_strong_explicit,_4_ignored,\
                        atomic_compare_exchange_strong)(__VA_ARGS__)

// _Bool atomic_compare_exchange_weak(volatile A *shared, C *expected, C desired);
// _Bool atomic_compare_exchange_weak_explicit(volatile A *shared, C *expected, C desired, memory_order success, memory_order failure);
//
// { t := (*shared = *expected); t ? (*shared := desired) : (*expected := *shared); t }
#define CASW(...)   M_3_5(__VA_ARGS__,\
                        atomic_compare_exchange_weak_explicit,_4_ignored,\
                        atomic_compare_exchange_weak)(__VA_ARGS__)

/* CAS usage protocol:
 *
    static _Atomic(A) shared = ...
    ...

    C x = LOAD(&shared);
    do {
        C y = ϕ(x);
    } while (!CASW(&shared, &x, y));

or, less efficient because `volatile` goes directly to the main memory,
ignoring caches:

    volatile C x = LOAD(&shared);
    while (!CASW(&shared, &x, ϕ(x)));
***********************************************************************/

// C atomic_fetch_ϕ(volatile A *shared, M operand);
// C atomic_fetch_ϕ_explicit(volatile A *shared, M operand, memory_order order);
//
// { v := *shared; *shared := v + operand; v }
#define ADD(...)    M_2_3(__VA_ARGS__,\
                        atomic_fetch_add_explicit,\
                        atomic_fetch_add)(__VA_ARGS__)

// { v := *shared; *shared := v - operand; v }
#define SUB(...)    M_2_3(__VA_ARGS__,\
                        atomic_fetch_sub_explicit,\
                        atomic_fetch_sub)(__VA_ARGS__)

// { v := *shared; *shared := v | operand; v }
#define OR(...)     M_2_3(__VA_ARGS__,\
                        atomic_fetch_or_explicit,\
                        atomic_fetch_or)(__VA_ARGS__)

// { v := *shared; *shared := v ^ operand; v }
#define XOR(...)    M_2_3(__VA_ARGS__,\
                        atomic_fetch_xor_explicit,\
                        atomic_fetch_xor)(__VA_ARGS__)

// { v := *shared; *shared := v & operand; v }
#define AND(...)    M_2_3(__VA_ARGS__,\
                        atomic_fetch_and_explicit,\
                        atomic_fetch_and)(__VA_ARGS__)

////////////////////////////////////////////////////////////////////////
// Extensions
////////////////////////////////////////////////////////////////////////

// Fetch And Increment
// { v := *shared; *shared := v + 1; v }
#define FAI(shared,...)  ADD((shared), 1 __VA_OPT__(,)__VA_ARGS__)

// Fetch And Decrement
// { v := *shared; *shared := v - 1; v }
#define FAD(shared,...)  SUB((shared), 1 __VA_OPT__(,)__VA_ARGS__)

// { v := *shared; *shared := v * operand; v }
#define MUL(SHARED,OPERAND,...) ({\
    typeof(*(SHARED)) v = *(SHARED);\
    while (!CASW((SHARED), &v, v * (OPERAND) __VA_OPT__(,)__VA_ARGS__));\
    v;\
})

// { v := *shared; *shared := v / operand; v }
#define DIV(SHARED,OPERAND,...) ({\
    typeof(*(SHARED)) v = *(SHARED);\
    while (!CASW((SHARED), &v, v / (OPERAND) __VA_OPT__(,)__VA_ARGS__));\
    v;\
})

// { v := *shared; *shared := v % operand; v }
#define MOD(SHARED,OPERAND,...) ({\
    typeof(*(SHARED)) v = *(SHARED);\
    while (!CASW((SHARED), &v, v % (OPERAND) __VA_OPT__(,)__VA_ARGS__));\
    v;\
})

////////////////////////////////////////////////////////////////////////
// Fences
////////////////////////////////////////////////////////////////////////

// type kill_dependency(type y);

// void atomic_thread_fence(memory_order order);
// void atomic_signal_fence(memory_order order);

#endif // vim:ai:sw=4:ts=4:et:syntax=cpp
