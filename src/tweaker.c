#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <box2d/b2Body.h>
#include <box2d/b2World.h>
#include <signal.h>

#include "default.h"
#include "xml.h"
#include "graph.h"

#define F "%.12f"

double score(struct design *design, struct block *gp)
{
	b2World *world;
	bool hit_1000 = false;
	double res = 1000.0;
	double y;
	int i;

	world = gen_world(design);

	for (i = 0; i < 250; i++) {
		y = gp->body->m_position.y;
		if (hit_1000) {
			if (res > y)
				res = y;
		} else {
			if (y > 1000.0)
				hit_1000 = true;
		}
		step(world);
	}

	b2World_dtor(world);
	free(world);

	return res;
}

int get_joint_cnt(struct design *design)
{
	struct joint *joint;
	int res = 0;

	for (joint = design->joints.head; joint; joint = joint->next)
		res++;

	return res;
}

struct joint *get_joint(struct design *design, int n)
{
	struct joint *joint = design->joints.head;
	int i;

	for (i = 0; i < n; i++)
		joint = joint->next;

	return joint;
}

double randf(double range)
{
	return ((double)rand() / RAND_MAX) * range - (range / 2);
}

void jitter_joint(struct joint *joint, double range)
{
	double dx = randf(range);
	double dy = randf(range);
	joint->x += dx;
	joint->y += dy;
}

volatile bool done = false;

void int_handler(int sig)
{
	done = true;
}

void export_joint_list(struct block *block)
{
	struct attach_node *nodes[2];
	int ids[2];
	int n;
	int out;
	int i;

	if (block->shape.type == SHAPE_ROD) {
		nodes[0] = block->shape.rod.from_att;
		nodes[1] = block->shape.rod.to_att;
		n = 2;
	} else {
		n = 0;
	}

	out = 0;
	for (i = 0; i < n; i++) {
		if (nodes[i]->prev)
			ids[out++] = nodes[i]->prev->block->id;
	}

	if (out) {
		printf(" [");
		for (i = 0; i < out; i++) {
			printf("%d", ids[i]);
			if (i + 1 < out)
				printf(", ");
		}
		printf("]");
	}
}

void export_block(struct block *block)
{
	struct shell shell;

	get_shell(&shell, &block->shape);

	if (block->shape.type == SHAPE_ROD) {
		if (block->shape.rod.width == 4.0)
			printf("Rod");
		else
			printf("Stick");
	} else {
		printf("GoalBall");
	}

	printf("#%d ("F", "F"), ", block->id, shell.x, shell.y);

	if (shell.type == SHELL_CIRC)
		printf("("F", "F"), ", shell.circ.radius * 2, shell.circ.radius * 2);
	else
		printf("("F", "F"), ", shell.rect.w, shell.rect.h);

	printf(F, shell.angle);

	export_joint_list(block);

	printf("\n");
}

void export(struct design *design)
{
	struct block *block;

	printf("BuildArea ("F", "F"), ("F", "F")\n", design->build_area.x,
						 design->build_area.y,
						 design->build_area.w,
						 design->build_area.h);
	printf("GoalArea ("F", "F"), ("F", "F")\n", design->goal_area.x,
						design->goal_area.y,
						design->goal_area.w,
						design->goal_area.h);

	for (block = design->player_blocks.head; block; block = block->next)
		export_block(block);
}

int main(void)
{
	struct xml_level xml_level;
	struct design design;
	struct block *block;
	int joint_cnt;
	int n;
	struct joint *joint;

	signal(SIGINT, int_handler);

	xml_parse(default_xml, sizeof(default_xml), &xml_level);
	convert_xml(&xml_level, &design);
	xml_free(&xml_level);

	for (block = design.player_blocks.head; block; block = block->next) {
		if (block->shape.type == SHAPE_WHEEL)
			break;
	}

	joint_cnt = get_joint_cnt(&design);

	double best_score = 1000.0;

	while (!done) {
		n = rand() % joint_cnt;
		joint = get_joint(&design, n);
		if (joint->gen)
			continue;

		double orig_x = joint->x;
		double orig_y = joint->y;
		double dx = randf(0.03);
		double dy = randf(0.03);
		joint->x += dx;
		joint->y += dy;
		double this_score = score(&design, block);
		if (this_score < best_score) {
			fprintf(stderr, F"\n", this_score);
			best_score = this_score;
		} else {
			joint->x = orig_x;
			joint->y = orig_y;
		}
	}

	export(&design);

	free_design(&design);

	return 0;
}
