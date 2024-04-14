#pragma once
#include <optional>
#include "Math/Vec3.h"

class Ray;
class AABB;
class Plane;

class Collisions {
public:
	static std::optional<Vector3f> raycastTriangle(const Ray& ray, const Vector3f triangle[]);

private:
	static bool isPointInTriangle(const Vector3f& point, const Vector3f triangle[]);
	static std::optional<Vector3f> raycastPlaneNoNormal(const Ray& ray, const Plane& plane);
};

