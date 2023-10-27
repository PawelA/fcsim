#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <fcsim.h>
#include "runner.h"
#include "timer.h"

struct execution {
	struct fcsim_handle *handle;
	int tick;
	struct runner_tick ticks[3];
	int prod_idx;
	int cons_idx;
	int next_idx;
	int state;
	pthread_mutex_t mutex;
};

static void execution_produce(struct execution *exec)
{
	int temp_idx;

	pthread_mutex_lock(&exec->mutex);
	/* swap prod and next */
	temp_idx = exec->prod_idx;
	exec->prod_idx = exec->next_idx;
	exec->next_idx = temp_idx;
	/* set state to A */
	exec->state = 0;
	pthread_mutex_unlock(&exec->mutex);
}

static void execution_consume(struct execution *exec)
{
	int temp_idx;

	pthread_mutex_lock(&exec->mutex);
	if (exec->state == 0) {
		/* swap cons and next */
		temp_idx = exec->cons_idx;
		exec->cons_idx = exec->next_idx;
		exec->next_idx = temp_idx;
		/* set state to B */
		exec->state = 1;
	}
	pthread_mutex_unlock(&exec->mutex);
}

static void execution_tick(struct execution *exec)
{
	struct runner_tick *tick = &exec->ticks[exec->prod_idx];

	fcsim_step(exec->handle);
	exec->tick++;

	fcsim_get_block_stats(exec->handle, tick->stats);
	tick->index = exec->tick;

	execution_produce(exec);
}

static void init_execution(struct execution *exec, struct fcsim_arena *arena)
{
	size_t stats_size;
	int i;

	exec->handle = fcsim_new(arena);
	exec->tick = 0;

	stats_size = sizeof(struct fcsim_block_stat) * arena->block_cnt;
	for (i = 0; i < 3; i++)
		exec->ticks[i].stats = malloc(stats_size);

	fcsim_get_block_stats(exec->handle, exec->ticks[0].stats);
	exec->ticks[0].index = 0;

	exec->cons_idx = 0;
	exec->prod_idx = 1;
	exec->next_idx = 2;
	exec->state = 1;

	pthread_mutex_init(&exec->mutex, NULL);
}

struct runner {
	pthread_t thread;

	struct execution execs[2];
	int exec_idx;

	int running;
	pthread_cond_t running_cond;
	uint64_t frame_limit_us;

	pthread_mutex_t mutex;
};

uint64_t frame_limit(uint64_t prev_ts, uint64_t frame_limit_us)
{
	uint64_t curr_ts;
	uint64_t delta_us;

	curr_ts = timer_get_us();
	delta_us = curr_ts - prev_ts;
	if (delta_us < frame_limit_us) {
		timer_sleep_us(frame_limit_us - delta_us);
		prev_ts = curr_ts + frame_limit_us - delta_us;
	} else {
		prev_ts = curr_ts;
	}

	return prev_ts;
}

static void *thread_func(void *arg)
{
	struct runner *runner = arg;
	struct execution *exec;
	uint64_t ts = 0;
	uint64_t frame_limit_us;
		
	while (1) {
		pthread_mutex_lock(&runner->mutex);
		while (!runner->running)
			pthread_cond_wait(&runner->running_cond, &runner->mutex);
		exec = &runner->execs[runner->exec_idx];
		frame_limit_us = runner->frame_limit_us;
		pthread_mutex_unlock(&runner->mutex);

		execution_tick(exec);
		ts = frame_limit(ts, frame_limit_us);
	}

	return NULL;
}


struct runner *runner_create(void)
{
	struct fcsim_arena arena = { 0 };
	struct runner *runner;

	runner = malloc(sizeof(*runner));
	if (!runner)
		return runner;

	init_execution(&runner->execs[0], &arena);
	runner->exec_idx = 0;

	runner->running = 0;
	runner->frame_limit_us = 16666;
	pthread_mutex_init(&runner->mutex, NULL);
	pthread_cond_init(&runner->running_cond, NULL);

	pthread_create(&runner->thread, NULL, thread_func, runner);

	return runner;
}

void runner_load(struct runner *runner, struct fcsim_arena *arena)
{
	int new_exec_idx = !runner->exec_idx;

	init_execution(&runner->execs[new_exec_idx], arena);

	pthread_mutex_lock(&runner->mutex);
	runner->exec_idx = new_exec_idx;
	pthread_mutex_unlock(&runner->mutex);
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
	struct execution *exec = &runner->execs[runner->exec_idx];

	execution_consume(exec);
	*tick = exec->ticks[exec->cons_idx];
}

void runner_set_frame_limit(struct runner *runner, uint64_t frame_limit_us)
{
	pthread_mutex_lock(&runner->mutex);
	runner->frame_limit_us = frame_limit_us;
	pthread_mutex_unlock(&runner->mutex);
}
