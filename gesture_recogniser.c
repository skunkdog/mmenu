#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gesture_recogniser.h"

static double distance(Point a, Point b) {
    return hypot(b.x - a.x, b.y - a.y);
}

static double path_length(const Point points[], size_t count) {
    double total = 0.0;

    for (size_t i = 1; i < count; i++) {
        total += distance(points[i - 1], points[i]);
    }

    return total;
}

static Point centroid(const Point points[], size_t count) {
    Point c = {0.0, 0.0};

    for (size_t i = 0; i < count; i++) {
        c.x += points[i].x;
        c.y += points[i].y;
    }

    c.x /= count;
    c.y /= count;
    return c;
}

/* Returns a newly allocated array of n resampled points. */
Point *resample(const Point input[], size_t count, size_t n) {
    if (input == NULL || count == 0 || n == 0) {
        return NULL;
    }

    Point *output = malloc(n * sizeof(Point));
    if (output == NULL) {
        return NULL;
    }

    if (n == 1) {
        output[0] = input[0];
        return output;
    }

    /* A zero-length path resamples to repeated copies of its first point. */
    double interval = path_length(input, count) / (double)(n - 1);
    if (interval == 0.0) {
        for (size_t i = 0; i < n; i++) {
            output[i] = input[0];
        }
        return output;
    }

    /* Working copy, because resampling inserts points into the path. */
    size_t capacity = count + n + 1;
    Point *working = malloc(capacity * sizeof(Point));
    if (working == NULL) {
        free(output);
        return NULL;
    }

    memcpy(working, input, count * sizeof(Point));
    size_t working_count = count;
    size_t output_count = 1;
    double accumulated = 0.0;
    output[0] = input[0];

    size_t i = 1;
    while (i < working_count && output_count < n) {
        Point previous = working[i - 1];
        Point current = working[i];
        double segment_length = distance(previous, current);

        if (segment_length > 0.0 &&
            accumulated + segment_length >= interval) {
            double ratio = (interval - accumulated) / segment_length;
            Point q = {
                previous.x + ratio * (current.x - previous.x),
                previous.y + ratio * (current.y - previous.y)
            };

            output[output_count++] = q;

            memmove(&working[i + 1], &working[i],
                    (working_count - i) * sizeof(Point));
            working[i] = q;
            working_count++;

            accumulated = 0.0;
            i++;  /* Continue from q toward the original current point. */
        } else {
            accumulated += segment_length;
            i++;
        }
    }

    /* Fill any remaining slots (usually only needed for rounding errors). */
    Point last = input[count - 1];
    while (output_count < n) {
        output[output_count++] = last;
    }

    free(working);
    return output;
}

void rotate_by(const Point points[], size_t count, double theta,
               Point output[]) {
    Point c = centroid(points, count);
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);

    for (size_t i = 0; i < count; i++) {
        double x = points[i].x - c.x;
        double y = points[i].y - c.y;

        output[i].x = x * cos_theta - y * sin_theta + c.x;
        output[i].y = x * sin_theta + y * cos_theta + c.y;
    }
}

void rotate_to_zero(const Point points[], size_t count, Point output[]) {
    Point c = centroid(points, count);
    double theta = atan2(c.y - points[0].y, c.x - points[0].x);

    rotate_by(points, count, -theta, output);
}

void scale_to_square(const Point points[], size_t count,
                     double size, Point output[]) {
    double min_x = points[0].x, max_x = points[0].x;
    double min_y = points[0].y, max_y = points[0].y;

    for (size_t i = 1; i < count; i++) {
        if (points[i].x < min_x) min_x = points[i].x;
        if (points[i].x > max_x) max_x = points[i].x;
        if (points[i].y < min_y) min_y = points[i].y;
        if (points[i].y > max_y) max_y = points[i].y;
    }

    double width = max_x - min_x;
    double height = max_y - min_y;
    double scale_x = (width == 0.0) ? 0.0 : size / width;
    double scale_y = (height == 0.0) ? 0.0 : size / height;

    for (size_t i = 0; i < count; i++) {
        output[i].x = points[i].x * scale_x;
        output[i].y = points[i].y * scale_y;
    }
}

void translate_to_origin(const Point points[], size_t count,
                         Point output[]) {
    Point c = centroid(points, count);

    for (size_t i = 0; i < count; i++) {
        output[i].x = points[i].x - c.x;
        output[i].y = points[i].y - c.y;
    }
}

static double path_distance(const Point a[], const Point b[], size_t count) {
    double total = 0.0;

    for (size_t i = 0; i < count; i++) {
        total += distance(a[i], b[i]);
    }

    return total / (double)count;
}

static double distance_at_angle(const Point points[], const Point template[],
                                size_t count, double theta) {
    Point *rotated = malloc(count * sizeof(Point));
    if (rotated == NULL) {
        return INFINITY;
    }

    rotate_by(points, count, theta, rotated);
    double result = path_distance(rotated, template, count);
    free(rotated);

    return result;
}

static double distance_at_best_angle(
    const Point points[], const Point template[], size_t count,
    double theta_a, double theta_b, double theta_delta
) {
    const double phi = (sqrt(5.0) - 1.0) / 2.0;

    double x1 = phi * theta_a + (1.0 - phi) * theta_b;
    double f1 = distance_at_angle(points, template, count, x1);

    double x2 = (1.0 - phi) * theta_a + phi * theta_b;
    double f2 = distance_at_angle(points, template, count, x2);

    while (fabs(theta_b - theta_a) > theta_delta) {
        if (f1 < f2) {
            theta_b = x2;
            x2 = x1;
            f2 = f1;
            x1 = phi * theta_a + (1.0 - phi) * theta_b;
            f1 = distance_at_angle(points, template, count, x1);
        } else {
            theta_a = x1;
            x1 = x2;
            f1 = f2;
            x2 = (1.0 - phi) * theta_a + phi * theta_b;
            f2 = distance_at_angle(points, template, count, x2);
        }
    }

    return (f1 < f2) ? f1 : f2;
}

/* Returns 1 on success, 0 on error. */
int recognize(
    const Point points[], size_t count,
    const Template templates[], size_t template_count,
    double size, double theta, double theta_delta,
    const char **best_label, double *score
) {
    if (points == NULL || templates == NULL || template_count == 0 ||
        count == 0 || size <= 0.0 || best_label == NULL || score == NULL) {
        return 0;
    }

    double best_distance = INFINITY;
    *best_label = NULL;

    for (size_t i = 0; i < template_count; i++) {
        if (templates[i].count != count) {
            continue;
        }

        double d = distance_at_best_angle(
            points, templates[i].points, count,
            -theta, theta, theta_delta
        );

        if (d < best_distance) {
            best_distance = d;
            *best_label = templates[i].label;
        }
    }

    if (*best_label == NULL) {
        return 0;  /* No template had the same number of points. */
    }

    *score = 1.0 - best_distance / (0.5 * sqrt(size * size + size * size));
    return 1;
}
