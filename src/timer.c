#include <stdint.h>
#include <time.h>

uint64_t timer_get_us(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void timer_sleep_us(uint64_t us)
{
	struct timespec ts;

	ts.tv_sec = us / 1000000;
	ts.tv_nsec = (us % 1000000) * 1000;

	nanosleep(&ts, NULL);
}
