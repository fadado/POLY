#ifndef POLY_PASS_H
#define POLY_PASS_H

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

/*
 *  THREAD_TYPE (name)
 *      Channel* input;  // Channel or Port
 *      Channel* output;
 *      ...
 *  END_TYPE
 *
 *  THREAD_BODY (name)
 *      ...
 *      receive from `input` and send to `output`
 *      ...
 *  END_BODY
 */
#define filter(T,I,O,...) \
    create (T, .input=(I), .output=(O) __VA_OPT__(,)__VA_ARGS__)

/*
 *  THREAD_TYPE (name)
 *      Channel* future;
 *      ...
 *  END_TYPE
 *
 *  THREAD_BODY (name)
 *      ...
 *      send result to `future`
 *      ...
 *  END_BODY
 */
#define promise(T,F,...) \
    create (T, .future=(F) __VA_OPT__(,)__VA_ARGS__)

#endif // vim:ai:sw=4:ts=4:syntax=cpp
