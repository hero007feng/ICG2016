#include <stdint.h>
#include "sim.h"

#define IX(i, j) (int)(((i)*(width+2) + (j)))
#define SWAP(x0, x) {float *tmp = x0; x0 = x; x = tmp;}

void add_source(int width, int height, float *x, float *src, float dt)
{
    int size = (width + 1) * (height + 1);
    for (int index = 0; index < size; ++index)
    {
        x[index] += dt * src[index];
    }
}

void set_bnd(int width, int height, int b, float *x)
{
    for (int i = 1; i < height; ++i)
    {
        x[IX(0, i)] = (b == 1)? (-x[IX(1, i)]): (x[IX(1, i)]);
        x[IX(height+1, i)] = (b == 1)? (-x[IX(height, i)]): (x[IX(height, i)]);
    }

    for (int i = 1; i < height; ++i)
    {
        x[IX(i, 0)] = (b == 2)? (-x[IX(i, 1)]): (x[IX(i, 1)]);
        x[IX(i, width+1)] = (b == 2)? (-x[IX(i, width)]): (x[IX(i, width)]);
    }

    x[IX(0, 0)] = 0.5*(x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, width+1)] = 0.5*(x[IX(1, width+1)] + x[IX(0, width)]);
    x[IX(height+1, 0)] = 0.5*(x[IX(height, 0)] + x[IX(height+1, 1)]);
    x[IX(height+1, width+1)] = 0.5*(x[IX(width, height+1)] + x[IX(width+1, height)]);
}

void diffuse(int width, int height, int b, float *x, float *x0, float diff, float dt)
{
    float a = dt*diff*width*height;
    for (int count = 0; count < 20; ++count)
    {
        for (int row = 1; row <= height; ++row)
        {
            for (int col = 1; col <= width; ++col)
            {
                x[IX(row, col)] = (x0[IX(row, col)] +
                                   a*(x[IX(row-1, col)] + x[IX(row+1, col)] +
                                      x[IX(row, col-1)] + x[IX(row, col+1)]))/(1+4*a);
            }
        }
    }
    set_bnd(width, height, b, x);
}

void advect(int width, int height, int b, float *d, float *d0, float *u, float *v, float dt)
{
    for (int row = 1; row <= height; ++row)
    {
        for (int col = 1; col <= width; ++col)
        {
            float x = row - dt*width*u[IX(row, col)];
            float y = col - dt*height*v[IX(row, col)];

            if (x < 0.5) x = 0.5;
            if (x > (width + 0.5)) x = width + 0.5;
            float i0 = (int)x, i1 = i0 + 1;

            if (y < 0.5) y = 0.5;
            if (y > (height + 0.5)) y = height + 0.5;
            float j0 = (int)y, j1 = i0 + 1;
            float s1 = x - i0, s0 = 1 -s1, t1 = y - j0, t0 = 1 - t1;

            d[IX(row, col)] = (s0*(t0*d0[IX(i0, j0)] + t1*d0[IX(i0, j1)]) +
                               s1*(t0*d0[IX(i1, j0)] + t1*d0[IX(i1, j1)]));
        }
    }

    set_bnd(width, height, b, d);
}

void project(int width, int height, float *u, float *v, float *p, float *div)
{
    float w = 1.0f/width;
    float h = 1.0f/height;

    for (int row = 1; row <= height; ++row)
    {
        for (int col = 1; col <= width; ++col)
        {
            div[IX(row, col)] = -0.5*(w*(u[IX(row+1, col)] - u[IX(row-1, col)]) +
                                      h*(v[IX(row, col+1)] - v[IX(row, col-1)]));
            p[IX(row, col)] = 0;
        }
    }
    set_bnd(width, height, 0, div);
    set_bnd(width, height, 0, p);

    for (int count = 0; count < 20; ++count)
    {
        for (int row = 1; row <= height; ++row)
        {
            for (int col = 1; col <= width; ++col)
            {
                p[IX(row, col)] = (div[IX(row, col)] +
                                   p[IX(row-1, col)] + p[IX(row+1, col)] +
                                   p[IX(row, col-1)] + p[IX(row, col+1)])/4;
            }
        }
        set_bnd(width, height, 0, p);
    }

    for (int row = 1; row <= height; ++row)
    {
        for (int col = 1; col <= width; ++col)
        {
            v[IX(row, col)] -= 0.5f*(p[IX(row+1, col)] - p[IX(row-1, col)])/w;
            v[IX(row, col)] -= 0.5f*(p[IX(row, col+1)] - p[IX(row, col-1)])/h;
        }
    }
    set_bnd(width, height, 1, u);
    set_bnd(width, height, 2, v);

}

void dens_step(int width, int height, float *x, float *x0, float *u, float *v, float diff, float dt)
{
    add_source(width, height, x, x0, dt);
    SWAP(x0, x); diffuse(width, height, 0, x, x0, diff, dt);
    SWAP(x0, x); advect(width, height, 0, x, x0, u, v, dt);
}

void vel_step(int width, int height, float *u, float *v, float *u0, float *v0, float visc, float dt)
{
    add_source(width, height, u, u0, dt); add_source(width, height, v, v0, dt);
    SWAP(u0, u); diffuse(width, height, 1, u, u0, visc, dt);
    SWAP(v0, v); diffuse(width, height, 2, v, v0, visc, dt);
    project(width, height, u, v, u0, v0);
    SWAP(u0, u); SWAP(v0, v);
    advect(width, height, 1, u, u0, u0, v0, dt); advect(width, height, 2, v, v0, u0, v0, dt);
    project(width, height, u, v, u0, v0);
}

#undef IX
#undef SWAP
