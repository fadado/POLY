
// TODO: all!

typedef cnd_t Condition;

#define condition_init		cnd_init
#define condition_destroy	cnd_destroy
#define condition_wait		cnd_wait
#define condition_notify	cnd_signal
#define condition_broadcast	cnd_broadcast
#define condition_wait_for	cnd_timedwait /*BUG*/
