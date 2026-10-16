#ifndef GESTURE_RECOGNISER_H
#define GESTURE_RECOGNISER_H

typedef struct {
    double x;
    double y;
} Point;

typedef struct {
    const char *label;
    const Point *points;
    size_t count;
} Template;


static double distance(Point a, Point b);

static double path_length(const Point points[], size_t count);

static Point centroid(const Point points[], size_t count);

Point *resample(const Point input[], size_t count, size_t n);

void rotate_by(const Point points[], size_t count, double theta,
               Point output[]);

void rotate_to_zero(const Point points[], size_t count, Point output[]);

void scale_to_square(const Point points[], size_t count,
                     double size, Point output[]);

void translate_to_origin(const Point points[], size_t count,
                         Point output[]);

static double path_distance(const Point a[], const Point b[], size_t count);

static double distance_at_angle(const Point points[], const Point template[],
                                size_t count, double theta);

static double distance_at_angle(const Point points[], const Point template[],
                                size_t count, double theta);

static double distance_at_best_angle(
    const Point points[], const Point template[], size_t count,
    double theta_a, double theta_b, double theta_delta
);

int recognize(
    const Point points[], size_t count,
    const Template templates[], size_t template_count,
    double size, double theta, double theta_delta,
    const char **best_label, double *score
);
#endif
