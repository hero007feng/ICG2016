#include <stdlib.h>
#include <stdbool.h>

#define IX(i,j) ((i)+(N+2)*(j))
#define SWAP(x0,x) {float * tmp=x0;x0=x;x=tmp;}
#define FOR_EACH_CELL for ( i=1 ; i<=N ; i++ ) { for ( j=1 ; j<=N ; j++ ) {
#define END_FOR }}
#define ABS(a) ((a>0)? a:-a)

void add_source ( int N, float * x, float * s, float dt )
{
	int i, size=(N+2)*(N+2);
	for ( i=0 ; i<size ; i++ ) x[i] += dt*s[i];
}

void set_bnd ( int N, int b, float * x )
{
	int i;

	for ( i=1 ; i<=N ; i++ ) {
		x[IX(0  ,i)] = b==1 ? -x[IX(1,i)] : x[IX(1,i)];
		x[IX(N+1,i)] = b==1 ? -x[IX(N,i)] : x[IX(N,i)];
		x[IX(i,0  )] = b==2 ? -x[IX(i,1)] : x[IX(i,1)];
		x[IX(i,N+1)] = b==2 ? -x[IX(i,N)] : x[IX(i,N)];
	}
	x[IX(0  ,0  )] = 0.5f*(x[IX(1,0  )]+x[IX(0  ,1)]);
	x[IX(0  ,N+1)] = 0.5f*(x[IX(1,N+1)]+x[IX(0  ,N)]);
	x[IX(N+1,0  )] = 0.5f*(x[IX(N,0  )]+x[IX(N+1,1)]);
	x[IX(N+1,N+1)] = 0.5f*(x[IX(N,N+1)]+x[IX(N+1,N)]);
}

void lin_solve ( int N, int b, float * x, float * x0, bool * ob_map, float a, float c )
{
	int i, j, k;
	float sum;
	float *ob_count_map;
	int size = (N+2)*(N+2);

	if (b == 0) {
		ob_count_map = (float *)malloc(size*sizeof(float));
		FOR_EACH_CELL
			ob_count_map[IX(i,j)] = ob_map[IX(i+1,j)] + ob_map[IX(i-1,j)] + ob_map[IX(i,j+1)] + ob_map[IX(i,j-1)];
		END_FOR
		set_bnd(N, 0, ob_count_map);
	}

	for ( k=0 ; k<20 ; k++ ) {
		FOR_EACH_CELL
			if (ob_map[IX(i,j)]) {continue;}

			switch (b) {
				case 0:
					x[IX(i,j)] = (x0[IX(i,j)] + a*(x[IX(i-1,j)]*(!ob_map[IX(i-1,j)])
												  +x[IX(i+1,j)]*(!ob_map[IX(i+1,j)])
												  +x[IX(i,j-1)]*(!ob_map[IX(i,j-1)])
												  +x[IX(i,j+1)]*(!ob_map[IX(i,j+1)])
												  +x0[IX(i,j)]*ob_count_map[IX(i,j)]))/c;
					break;

				case 1:
				case 2:
					sum = 0;
					sum += b==1 ? -x[IX(1,i)] : x[IX(1,i)];
					sum += b==1 ? -x[IX(N,i)] : x[IX(N,i)];
					sum += b==2 ? -x[IX(i,1)] : x[IX(i,1)];
					sum += b==2 ? -x[IX(i,N)] : x[IX(i,N)];
					x[IX(i,j)] = (x0[IX(i,j)] + a*sum)/c;
					break;

				default:
				x[IX(i,j)] = (x0[IX(i,j)] + a*(x[IX(i-1,j)]+x[IX(i+1,j)]+x[IX(i,j-1)]+x[IX(i,j+1)]))/c;
			}
		END_FOR
		set_bnd ( N, b, x );
	}
}

void diffuse ( int N, int b, float * x, float * x0, bool * ob_map, float diff, float dt )
{
	float a=dt*diff*N*N;
	lin_solve ( N, b, x, x0, ob_map, a, 1+4*a );
}

void advect ( int N, int b, float * d, float * d0, float * u, float * v, bool * ob_map, float dt )
{
	int i, j, i0, j0, i1, j1, m, n;
	float x, y, s0, t0, s1, t1, dt0;
	float dx, dy;
	int step;

	dt0 = dt*N;
	FOR_EACH_CELL
		x = i-dt0*u[IX(i,j)]; y = j-dt0*v[IX(i,j)];
		if (x<0.5f) x=0.5f; if (x>N+0.5f) x=N+0.5f;
		if (y<0.5f) y=0.5f; if (y>N+0.5f) y=N+0.5f;
		i0=(int)x; j0=(int)y;

		// THE BROKEN CODE
		/*
		m = i0-i; n = j0-j;
		step = (ABS(m) > ABS(n))? ABS(m):ABS(n);
		if (step) {
			dx = (float)m / (float)step; dy = (float)n / (float)step;

			int k;
			x = i; y = j;
			for (k = 0; k < step; k++) {
				x += dx; y += dy;
				i0 = (int)x; j0 = (int)y;
				if (ob_map[IX(i0,j0)]) {
					k = -1;
					break;
				}
			}
			if (k == -1) {
				i0 = (int)(x-dx); j0 = (int)(y-dy);
			} else {
				i0 = (int)x; j0 = (int)y;
			}
		}
		*/
		
		i1=i0+1; j1=j0+1;
		s1 = x-i0; s0 = 1-s1; t1 = y-j0; t0 = 1-t1;
		d[IX(i,j)] = s0*(t0*d0[IX(i0,j0)]+t1*d0[IX(i0,j1)])+
					 s1*(t0*d0[IX(i1,j0)]+t1*d0[IX(i1,j1)]);
	END_FOR
	set_bnd ( N, b, d );
}

void project ( int N, float * u, float * v, float * p, float * div, bool * ob_map )
{
	int i, j;

	FOR_EACH_CELL
		div[IX(i,j)] = -0.5f*(u[IX(i+1,j)]-u[IX(i-1,j)]+v[IX(i,j+1)]-v[IX(i,j-1)])/N;
		p[IX(i,j)] = 0;
	END_FOR	
	set_bnd ( N, 0, div ); set_bnd ( N, 0, p );

	lin_solve ( N, 0, p, div, ob_map, 1, 4 );

	FOR_EACH_CELL
		u[IX(i,j)] -= 0.5f*N*(p[IX(i+1,j)]-p[IX(i-1,j)]);
		v[IX(i,j)] -= 0.5f*N*(p[IX(i,j+1)]-p[IX(i,j-1)]);
	END_FOR
	set_bnd ( N, 1, u ); set_bnd ( N, 2, v );
}

void dens_step ( int N, float * x, float * x0, float * u, float * v, bool * ob_map, float diff, float dt )
{
	add_source ( N, x, x0, dt );
	SWAP ( x0, x ); diffuse ( N, 0, x, x0, ob_map, diff, dt );
	SWAP ( x0, x ); advect ( N, 0, x, x0, u, v, ob_map, dt );
}

void vel_step ( int N, float * u, float * v, float * u0, float * v0, bool * ob_map, float visc, float dt )
{
	add_source ( N, u, u0, dt ); add_source ( N, v, v0, dt );
	SWAP ( u0, u ); diffuse ( N, 1, u, u0, ob_map, visc, dt );
	SWAP ( v0, v ); diffuse ( N, 2, v, v0, ob_map, visc, dt );
	project ( N, u, v, u0, v0, ob_map );
	SWAP ( u0, u ); SWAP ( v0, v );
	advect ( N, 1, u, u0, u0, v0, ob_map, dt ); advect ( N, 2, v, v0, u0, v0, ob_map, dt );
	project ( N, u, v, u0, v0, ob_map );
}