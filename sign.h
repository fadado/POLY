
// TODO: all!

typedef cnd_t Sign;

#define sign_init	cnd_init
#define sign_destroy	cnd_destroy
#define sign_wait	cnd_wait
#define sign_give	cnd_signal
#define sign_broadcast	cnd_broadcast
#define sign_wait_for	cnd_timedwait /*BUG*/
