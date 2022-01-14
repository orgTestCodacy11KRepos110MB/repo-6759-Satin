//
//  Bezier.c
//  Satin
//
//  Created by Reza Ali on 6/28/20.
//

#include <malloc/_malloc.h>
#include <string.h>

#include "Bezier.h"

typedef struct Point2DStructure Point2D;
struct Point2DStructure {
    int index;
    simd_float2 position;
    Point2D *next;
};

void freePolyline2D(Polyline2D *line) {
    if (line->count <= 0 && line->data == NULL) { return; }
    free(line->data);
    line->data = NULL;
    line->count = 0;
}

void addPointToPolyline2D(simd_float2 p, Polyline2D *line) {
    if (line->count == 0 || line->data == NULL) {
        line->data = (simd_float2 *)malloc(1 * sizeof(simd_float2));
        line->count = 1;
        line->data[0] = p;
    } else {
        int newCount = line->count + 1;
        line->data = (simd_float2 *)realloc(line->data, newCount * sizeof(simd_float2));
        line->data[line->count] = p;
        line->count = newCount;
    }
}

void removeFirstPointInPolyline2D(Polyline2D *line) {
    if (line->count > 0 || line->data != NULL) {
        int newCount = line->count - 1;
        if (newCount > 0) {
            size_t newSize = newCount * sizeof(simd_float2);
            simd_float2 *newData = (simd_float2 *)malloc(newSize);
            memcpy(newData, line->data + 1, newSize);
            free(line->data);
            line->data = newData;
            line->count = newCount;
        } else {
            freePolyline2D(line);
        }
    }
}

void removeLastPointInPolyline2D(Polyline2D *line) {
    if (line->count > 0 || line->data != NULL) {
        int newCount = line->count - 1;
        if (newCount > 0) {
            size_t newSize = newCount * sizeof(simd_float2);
            simd_float2 *newData = (simd_float2 *)malloc(newSize);
            memcpy(newData, line->data, newSize);
            free(line->data);
            line->data = newData;
            line->count = newCount;
        } else {
            freePolyline2D(line);
        }
    }
}

void appendPolyline2D(Polyline2D *dst, Polyline2D *src) {
    if (src->count > 0 && src->data != NULL) {

        if (dst->count == 0 || dst->data == NULL) {
            int newCount = src->count;
            size_t newSize = newCount * sizeof(simd_float2);
            dst->data = (simd_float2 *)malloc(newSize);
            memcpy(dst->data, src->data, newSize);
            dst->count = newCount;
        } else {
            int newCount = dst->count + src->count;
            size_t newSize = newCount * sizeof(simd_float2);
            dst->data = (simd_float2 *)realloc(dst->data, newSize);
            memcpy(dst->data + dst->count, src->data, src->count * sizeof(simd_float2));
            dst->count = newCount;
        }
    }
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

Polyline2D getAdaptiveLinearPath2(simd_float2 a, simd_float2 b, float distanceLimit) {
    const float length = simd_length(b - a);
    if(length > distanceLimit) {
        const int sections = MAX(ceilf(length / distanceLimit), 2);
        const float inc = 1.0 / (float)(sections-1);
        simd_float2 *data = (simd_float2 *)malloc(sections * sizeof(simd_float2));
        simd_float2 t = 0.0;
        for(int i = 0; i < sections; i++) {
            data[i] = simd_mix(a, b, t);
            t += inc;
            t = MIN(MAX(t, 0.0), 1.0);
        }
        return (Polyline2D) { .count = sections, .data = data };
    }
    else {
        simd_float2 *data = (simd_float2 *)malloc(2 * sizeof(simd_float2));
        data[0] = a;
        data[1] = b;
        return (Polyline2D) { .count = 2, .data = data };
    }
}

simd_float2 quadraticBezier2(simd_float2 a, simd_float2 b, simd_float2 c, float t) {
    float oneMinusT = 1.0 - t;
    return oneMinusT * oneMinusT * a + 2.0 * oneMinusT * t * b + t * t * c;
}

simd_float2 quadraticBezierVelocity2(simd_float2 a, simd_float2 b, simd_float2 c, float t) {
    float oneMinusT = 1.0 - t;
    return 2.0 * oneMinusT * (b - a) + 2 * t * (c - b);
}

simd_float2 quadraticBezierAcceleration2(simd_float2 a, simd_float2 b, simd_float2 c, float t) {
    return 2.0 * (c - 2.0 * b + a);
}

float quadraticBezierCurvature2(simd_float2 a, simd_float2 b, simd_float2 c, float t) {
    simd_float3 vel = simd_make_float3(quadraticBezierVelocity2(a, b, c, t), 0.0);
    simd_float3 acc = simd_make_float3(quadraticBezierAcceleration2(a, b, c, t), 0.0);
    return simd_length(simd_cross(vel, acc)) / pow(simd_length(vel), 3.0);
}

Polyline2D getQuadraticBezierPath2(simd_float2 a, simd_float2 b, simd_float2 c, int res) {
    simd_float2 *data = (simd_float2 *)malloc(res * sizeof(simd_float2));
    const float resMinusOne = res - 1;
    for (int i = 0; i < res; i++) {
        const float t = (float)i / resMinusOne;
        data[i] = quadraticBezier2(a, b, c, t);
    }
    return (Polyline2D) { .count = res, .data = data };
}

int _adaptiveQuadraticBezierCurve2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 aVel,
                                   simd_float2 bVel, simd_float2 cVel, float angleLimit, int depth,
                                   Point2D *start, Point2D *end) {
    if (depth > 8) { return end->index; }

    const float startMiddleAngle = acos(simd_dot(aVel, bVel));
    const float middleEndAngle = acos(simd_dot(bVel, cVel));

    if ((startMiddleAngle + middleEndAngle) > angleLimit) {
        // Split curve into two curves (start, end)

        const simd_float2 ab = (a + b) * 0.5;
        const simd_float2 bc = (b + c) * 0.5;
        const simd_float2 abc = (ab + bc) * 0.5;

        // Start Curve:  a,      ab,     abc
        // End Curve:    abc,    bc,     c

        Point2D *middle = (Point2D *)malloc(sizeof(Point2D));
        middle->index = start->index + 1;
        middle->position = abc;
        middle->next = end;

        start->next = middle;

        simd_float2 sVel = simd_normalize(quadraticBezierVelocity2(a, ab, abc, 0.5));

        middle->index = _adaptiveQuadraticBezierCurve2(a, ab, abc, aVel, sVel, bVel, angleLimit,
                                                       depth + 1, start, middle);
        end->index = middle->index + 1;

        simd_float2 eVel = simd_normalize(quadraticBezierVelocity2(abc, bc, c, 0.5));
        return _adaptiveQuadraticBezierCurve2(abc, bc, c, bVel, eVel, cVel, angleLimit, depth + 1,
                                              middle, end);
    } else {
        return end->index;
    }
}

Polyline2D getAdaptiveQuadraticBezierPath2(simd_float2 a, simd_float2 b, simd_float2 c,
                                           float angleLimit) {
    simd_float2 aVel = simd_normalize(quadraticBezierVelocity2(a, b, c, 0.0));
    simd_float2 bVel = simd_normalize(quadraticBezierVelocity2(a, b, c, 0.5));
    simd_float2 cVel = simd_normalize(quadraticBezierVelocity2(a, b, c, 1.0));

    Point2D *start = (Point2D *)malloc(sizeof(Point2D));
    Point2D *end = (Point2D *)malloc(sizeof(Point2D));

    start->index = 0;
    start->position = a;
    start->next = end;

    end->index = 1;
    end->position = c;
    end->next = start;

    const int count =
        _adaptiveQuadraticBezierCurve2(a, b, c, aVel, bVel, cVel, angleLimit, 0, start, end) + 1;
    simd_float2 *data = (simd_float2 *)malloc(count * sizeof(simd_float2));

    Point2D *iterator = start;
    for (int i = 0; i < count; i++) {
        data[iterator->index] = iterator->position;
        Point2D *cache = iterator;
        iterator = iterator->next;
        free(cache);
    }

    return (Polyline2D) { .count = count, .data = data };
}

simd_float2 cubicBezier2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d, float t) {
    float oneMinusT = 1.0 - t;
    float oneMinusT2 = oneMinusT * oneMinusT;
    float oneMinusT3 = oneMinusT2 * oneMinusT;
    return oneMinusT3 * a + 3.0 * oneMinusT2 * t * b + 3.0 * oneMinusT * t * t * c + t * t * t * d;
}

simd_float2 cubicBezierVelocity2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d,
                                 float t) {
    float oneMinusT = 1.0 - t;
    float oneMinusT2 = oneMinusT * oneMinusT;
    return 3.0 * oneMinusT2 * (b - a) + 6.0 * oneMinusT * t * (c - b) + 3.0 * t * t * (d - c);
}

simd_float2 cubicBezierAcceleration2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d,
                                     float t) {
    return 6.0 * (1.0 - t) * (c - 2.0 * b + a) + 6.0 * t * (d - 2.0 * c + b);
}

float cubicBezierCurvature2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d, float t) {
    simd_float3 vel = simd_make_float3(cubicBezierVelocity2(a, b, c, d, t), 0.0);
    simd_float3 acc = simd_make_float3(cubicBezierAcceleration2(a, b, c, d, t), 0.0);
    return simd_length(simd_cross(vel, acc)) / pow(simd_length(vel), 3.0);
}

Polyline2D getCubicBezierPath2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d,
                               int res) {
    simd_float2 *data = (simd_float2 *)malloc(res * sizeof(simd_float2));
    const float resMinusOne = res - 1;
    for (int i = 0; i < res; i++) {
        const float t = (float)i / resMinusOne;
        data[i] = cubicBezier2(a, b, c, d, t);
    }
    return (Polyline2D) { .count = res, .data = data };
}

int _adaptiveCubicBezierCurve2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d,
                               simd_float2 aVel, simd_float2 bVel, simd_float2 cVel,
                               float angleLimit, int depth, Point2D *start, Point2D *end) {
    if (depth > 8) { return end->index; }

    const float startMiddleAngle = acos(simd_dot(aVel, bVel));
    const float middleEndAngle = acos(simd_dot(bVel, cVel));

    if ((startMiddleAngle + middleEndAngle) > angleLimit) {
        // Split curve into two curves (start, end)

        const simd_float2 ab = (a + b) * 0.5;
        const simd_float2 bc = (b + c) * 0.5;
        const simd_float2 cd = (c + d) * 0.5;
        const simd_float2 abc = (ab + bc) * 0.5;
        const simd_float2 bcd = (bc + cd) * 0.5;
        const simd_float2 abcd = (abc + bcd) * 0.5;

        // Start Curve:  a,      ab,     abc,    abcd
        // End Curve:    abcd,   bcd,    cd,     d

        Point2D *middle = (Point2D *)malloc(sizeof(Point2D));
        middle->index = start->index + 1;
        middle->position = abcd;
        middle->next = end;

        start->next = middle;

        simd_float2 sVel = simd_normalize(cubicBezierVelocity2(a, ab, abc, abcd, 0.5));

        middle->index = _adaptiveCubicBezierCurve2(a, ab, abc, abcd, aVel, sVel, bVel, angleLimit,
                                                   depth + 1, start, middle);
        end->index = middle->index + 1;

        simd_float2 eVel = simd_normalize(cubicBezierVelocity2(abcd, bcd, cd, d, 0.5));
        return _adaptiveCubicBezierCurve2(abcd, bcd, cd, d, bVel, eVel, cVel, angleLimit, depth + 1,
                                          middle, end);
    } else {
        return end->index;
    }
}

Polyline2D getAdaptiveCubicBezierPath2(simd_float2 a, simd_float2 b, simd_float2 c, simd_float2 d,
                                       float angleLimit) {
    simd_float2 aVel = simd_normalize(cubicBezierVelocity2(a, b, c, d, 0.0));
    simd_float2 bVel = simd_normalize(cubicBezierVelocity2(a, b, c, d, 0.5));
    simd_float2 cVel = simd_normalize(cubicBezierVelocity2(a, b, c, d, 1.0));

    Point2D *start = (Point2D *)malloc(sizeof(Point2D));
    Point2D *end = (Point2D *)malloc(sizeof(Point2D));

    start->index = 0;
    start->position = a;
    start->next = end;

    end->index = 1;
    end->position = d;
    end->next = start;

    const int count =
        _adaptiveCubicBezierCurve2(a, b, c, d, aVel, bVel, cVel, angleLimit, 0, start, end) + 1;
    simd_float2 *data = (simd_float2 *)malloc(count * sizeof(simd_float2));

    Point2D *iterator = start;
    for (int i = 0; i < count; i++) {
        data[iterator->index] = iterator->position;
        Point2D *cache = iterator;
        iterator = iterator->next;
        free(cache);
    }

    return (Polyline2D) { .count = count, .data = data };
}

simd_float3 cubicBezier3(simd_float3 a, simd_float3 b, simd_float3 c, simd_float3 d, float t) {
    float oneMinusT = 1.0 - t;
    return oneMinusT * oneMinusT * oneMinusT * a + 3.0 * oneMinusT * oneMinusT * t * b +
           3.0 * oneMinusT * t * t * c + t * t * t * d;
}

simd_float3 quadraticBezier3(simd_float3 a, simd_float3 b, simd_float3 c, float t) {
    float oneMinusT = 1.0 - t;
    return oneMinusT * oneMinusT * a + 2.0 * oneMinusT * t * b + t * t * c;
}