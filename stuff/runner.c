#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <fcsim.h>
#include "runner.h"
#include "timer.h"

struct runner {
	struct fcsim_arena arena;
	struct fcsim_handle *handle;
	pthread_t thread;

	struct runner_tick ticks[3];
	int prod_idx;
	int next_idx;
	int cons_idx;

	int running;
	pthread_cond_t running_cond;
	uint64_t frame_limit_us;

	pthread_mutex_t mutex;
};

int new_idx(struct runner *runner)
{
	int taken[3] = { 0, 0, 0 };
	int i;

	taken[runner->prod_idx] = 1;
	taken[runner->cons_idx] = 1;
	taken[runner->next_idx] = 1;

	for (i = 0; i < 3; i++) {
		if (!taken[i])
			return i;
	}

	return -1;
}

void *thread_func(void *arg)
{
	struct runner *runner = arg;
	int prod_idx;
	int tick;
	uint64_t prev_ts;
	uint64_t curr_ts;
	uint64_t delta_us;
	uint64_t frame_limit_us;

	while (1) {
		pthread_mutex_lock(&runner->mutex);
		while (!runner->running)
			pthread_cond_wait(&runner->running_cond, &runner->mutex);
		pthread_mutex_unlock(&runner->mutex);

		tick = 0;
		prod_idx = 1;
		pthread_mutex_lock(&runner->mutex);
		runner->cons_idx = 0;
		runner->next_idx = 0;
		runner->prod_idx = 1;
		frame_limit_us = runner->frame_limit_us;
		pthread_mutex_unlock(&runner->mutex);

		prev_ts = timer_get_us();

		while (1) {
			fcsim_step(runner->handle);
			fcsim_get_block_stats(runner->handle, runner->ticks[prod_idx].stats);
			curr_ts = timer_get_us();
			delta_us = curr_ts - prev_ts;
			if (delta_us < frame_limit_us) {
				timer_sleep_us(frame_limit_us - delta_us);
				prev_ts = curr_ts + frame_limit_us - delta_us;
			} else {
				prev_ts = curr_ts;
			}
			runner->ticks[prod_idx].index = tick++;
			pthread_mutex_lock(&runner->mutex);
			runner->next_idx = prod_idx;
			prod_idx = new_idx(runner);
			runner->prod_idx = prod_idx;
			if (!runner->running)
				break;
			frame_limit_us = runner->frame_limit_us;
			pthread_mutex_unlock(&runner->mutex);
		}
		pthread_mutex_unlock(&runner->mutex);
	}

	return NULL;
}

struct runner *runner_create(void)
{
	struct runner *runner;

	runner = malloc(sizeof(*runner));
	if (!runner)
		return runner;

	runner->running = 0;
	runner->prod_idx = 0;
	runner->next_idx = 0;
	runner->cons_idx = 0;
	pthread_mutex_init(&runner->mutex, NULL);
	pthread_cond_init(&runner->running_cond, NULL);

	pthread_create(&runner->thread, NULL, thread_func, runner);

	return runner;
}

void runner_load(struct runner *runner, struct fcsim_arena *arena)
{
	int i;

	for (i = 0; i < 3; i++) {
		runner->ticks[i].stats = malloc(sizeof(struct fcsim_block_stat) *
						arena->block_cnt);
	}
	runner->handle = fcsim_new(arena);
	fcsim_get_block_stats(runner->handle, runner->ticks[0].stats);
}

void runner_start(struct runner *runner) {
	pthread_mutex_lock(&runner->mutex);
	runner->running = 1;
	pthread_cond_signal(&runner->running_cond);
	pthread_mutex_unlock(&runner->mutex);
}

void runner_stop(struct runner *runner) {
	pthread_mutex_lock(&runner->mutex);
	runner->running = 0;
	pthread_mutex_unlock(&runner->mutex);
}

void runner_get_tick(struct runner *runner, struct runner_tick *tick)
{
	pthread_mutex_lock(&runner->mutex);
	runner->cons_idx = runner->next_idx;
	*tick = runner->ticks[runner->cons_idx];
	pthread_mutex_unlock(&runner->mutex);
}

