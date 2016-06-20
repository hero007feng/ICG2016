#ifndef SIM_H
#define SIM_H

#include "display.h"

void add_source(int width, int height, float *x, float *src, float dt);
void set_bnd(int width, int height, int b, float *x);
void diffuse(int width, int height, int b, float *x, float *x0, float diff, float dt);
void advect(int width, int height, int b, float *d, float *d0, float *u, float *v, float dt);
void project(int width, int height, float *u, float *v, float *p, float *div);

void dens_step(int width, int height, float *x, float *x0, float *u, float *v, float diff, float dt);
void vel_step(int width, int height, float *u, float *v, float *u0, float *v0, float visc, float dt);

#endif
