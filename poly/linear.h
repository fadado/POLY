#ifndef LINEARIZABILITY_H
#define LINEARIZABILITY_H

#include <stdatomic.h>

#ifndef atomic
#define atomic(T)  _Atomic(T)
#endif

enum { // short aliases
    RELAXED = memory_order_relaxed,
    CONSUME = memory_order_consume,
    ACQUIRE = memory_order_acquire,
    RELEASE = memory_order_release,
    ACQ_REL = memory_order_acq_rel,
    SEQ_CST = memory_order_seq_cst,
};

#define M_1_2(_1,_2,NAME,...) NAME
#define M_2_3(_1,_2,_3,NAME,...) NAME

enum { FLAG_CLEAR, FLAG_SET };

// atomic_flag f = ATOMIC_FLAG_INIT;

#define TAS(...)     M_1_2(__VA_ARGS__,\
                           atomic_flag_test_and_set_explicit,\
                           atomic_flag_test_and_set)(__VA_ARGS__)

#define CLEAR(...)   M_1_2(__VA_ARGS__,\
                           atomic_flag_clear_explicit,\
                           atomic_flag_clear)(__VA_ARGS__)

// atomic_init(PTR,VAL)

//
#define LOAD(...)    M_1_2(__VA_ARGS__,\
                           atomic_load_explicit,\
                           atomic_load)(__VA_ARGS__)

#define STORE(...)   M_2_3(__VA_ARGS__,\
                           atomic_store_explicit,\
                           atomic_store)(__VA_ARGS__)

#define CASS 3 or 5 parameters!!!
#define CASW 3 or 5 parameters!!!

#define ADD
#define SUB
#define OR
#define XOR
#define AND

#endif // vim:ai:sw=4:ts=4:syntax=cpp
