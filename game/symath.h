#ifndef SYMATH_H
#define SYMATH_H

#include <math.h>
#include "raymath.h"

Vector2 vector3_xz(Vector3 v); //Returns a Vector2 consisting of x and z of a Vector3.
Vector3 vector2_to_xz(Vector2 v, float y); //Returns a Vector3 where x, z are the x, y of a Vector2.
Vector2 vector2_rotate_cw(Vector2 v); //Rotate a Vector2 by 90 degrees.
Vector2 closest_point_on_line(Vector2 v1, Vector2 v2, Vector2 p); //Returns a point on a line segment from v1 to v2 that is the closest to p.
Vector2 unclipping_vector(Vector2 p, float r, Vector2 near, Vector2 push_dir); //Returns how much a circle must move in a direction to not be clipping with a point.

Vector2 vector3_xz(Vector3 v) {
  return (Vector2){v.x, v.z};
}

Vector3 vector2_to_xz(Vector2 v, float y) {
  return (Vector3){v.x, y, v.y};
}

Vector2 vector2_rotate_cw(Vector2 v) {
  return (Vector2){v.y, -v.x};
}

Vector2 closest_point_on_line(Vector2 v1, Vector2 v2, Vector2 p) {
  Vector2 a = Vector2Subtract(v2, v1);
  Vector2 b  = Vector2Subtract(v1, p);
  float ab = Vector2DotProduct(a, b);
  float aa = Vector2DotProduct(a, a);
  float t = -ab / aa;
  if (t < 0.0f)
    return v1;
  if (t > 1.0f)
    return v2;
  return (Vector2){
    (1 - t) * v1.x + t * v2.x,
    (1 - t) * v1.y + t * v2.y
  };
}

Vector2 unclipping_vector(Vector2 p, float r, Vector2 near, Vector2 push_dir) {
  float dist = Vector2Distance(p, near) - r;
  if (dist >= 0)
    return Vector2Zero();
  Vector2 dir = Vector2Normalize(Vector2Subtract(near, p));
  float dot = Vector2DotProduct(push_dir, dir);
  if (dot > 0.0f)
    return Vector2Zero();
  return Vector2Scale(push_dir, dot * dist);
}

#endif