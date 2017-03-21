
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
/* void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max ); */
void * compute_image(void * arg);

void show_help()
{
    printf("Use: mandel [options]\n");
    printf("Where options are:\n");
    printf("-m <max>     The maximum number of iterations per point. (default=1000)\n");
    printf("-n <threads> The number of threads to use. (default=1)\n");
    printf("-x <coord>   X coordinate of image center point. (default=0)\n");
    printf("-y <coord>   Y coordinate of image center point. (default=0)\n");
    printf("-s <scale>   Scale of the image in Mandlebrot coordinates. (default=4)\n");
    printf("-W <pixels>  Width of the image in pixels. (default=500)\n");
    printf("-H <pixels>  Height of the image in pixels. (default=500)\n");
    printf("-o <file>    Set output file. (default=mandel.bmp)\n");
    printf("-h           Show this help text.\n");
    printf("\nSome examples are:\n");
    printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
    printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
    printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

struct thread_args {
    struct bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max;
    int sector;
    double sector_width;
};


int main( int argc, char *argv[] )
{
    char c;

    // These are the default configuration values used
    // if no command line arguments are given.

    const char *outfile = "mandel.bmp";
    double xcenter = 0;
    double ycenter = 0;
    int    n_threads = 1;
    double scale = 4;
    int    image_width = 500;
    int    image_height = 500;
    int    max = 1000;

    // For each command line argument given,
    // override the appropriate configuration value.

    while((c = getopt(argc,argv,"x:y:n:s:W:H:m:o:h"))!=-1) {
        switch(c) {
            case 'x':
                xcenter = atof(optarg);
                break;
            case 'y':
                ycenter = atof(optarg);
                break;
            case 'n':
                n_threads = atoi(optarg);
                break;
            case 's':
                scale = atof(optarg);
                break;
            case 'W':
                image_width = atoi(optarg);
                break;
            case 'H':
                image_height = atoi(optarg);
                break;
            case 'm':
                max = atoi(optarg);
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'h':
                show_help();
                exit(1);
                break;
        }
    }
    if(n_threads > image_height) n_threads = image_height;

    // Display the configuration of the image.
    printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s threads=%d\n",xcenter,ycenter,scale,max,outfile,n_threads);

    // Create a bitmap of the appropriate size.
    struct bitmap *bm = bitmap_create(image_width,image_height);

    // Fill it with a dark blue, for debugging
    bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

    // array of pthread_t's so we can free them later
    pthread_t *threads;
    threads = malloc((sizeof(pthread_t)*n_threads));

    // array of thread args so we can later free them
    struct thread_args ** ta;
    ta = malloc((sizeof(struct thread_args*)*n_threads));

    int i;
    for(i = 0; i < n_threads; i++) {
        // invoke compute_image in each thread
        ta[i] = malloc(sizeof(struct thread_args));
        ta[i]->bm = bm;
        ta[i]->xmin = xcenter-scale;
        ta[i]->xmax = xcenter+scale;
        ta[i]->ymin = ycenter-scale;
        ta[i]->ymax = ycenter+scale;
        ta[i]->sector = i;
        ta[i]->sector_width = (double)image_height/n_threads;
        ta[i]->max = max;
        pthread_create(&threads[i], NULL, &compute_image, ta[i]);
    }

    for(i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
        free(ta[i]);
    }
    // free memory
    free(ta);
    free(threads);

    // Save the image in the stated file.
    if(!bitmap_save(bm,outfile)) {
        fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
        return 1;
    }
    bitmap_delete(bm);

    return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/
void * compute_image(void * arg) {
    int i,j;

    struct thread_args * t = (struct thread_args *) arg;

    double width = bitmap_width(t->bm);
    double height = bitmap_height(t->bm);

    // For every pixel in the image...
    double start = t->sector*t->sector_width;
    double end = (t->sector+1)*t->sector_width;

    // make sure we go all the way to the end
    if(height - end < t->sector_width) end = height;

    for(j=start; j < end; j++) {
        for(i=0;i<width;i++) {

            // Determine the point in x,y space for that pixel.
            double x = t->xmin + i*(t->xmax-t->xmin)/width;
            double y = t->ymin + j*(t->ymax-t->ymin)/height;

            // Compute the iterations at that point.
            int iters = iterations_at_point(x,y,t->max);

            // Set the pixel in the bitmap.
            bitmap_set(t->bm,i,j,iters);
        }
    }
    return 0;

}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
    double x0 = x;
    double y0 = y;

    int iter = 0;

    while( (x*x + y*y <= 4) && iter < max ) {

        double xt = x*x - y*y + x0;
        double yt = 2*x*y + y0;

        x = xt;
        y = yt;

        iter++;
    }

    return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
    /* int color1 = 255*i/max; */
    /* int color2 = 255*i/max; */
    /* int color3 = 255*i/max; */
    int color1 = (int)log(i)*i % 255;
    int color2 = (int)log(i)*i*i % 125;
    int color3 = (int)log(i)*i*i*i % 80;
    return MAKE_RGBA(color1,color2,color3,0);
}




