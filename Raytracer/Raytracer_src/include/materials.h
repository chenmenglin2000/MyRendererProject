#pragma once

#include "geometry.h"

struct Materials {
	Vec3f diffuseColor;
	Vec4f albedo;
	float specularExponent;
	float refractiveIndex;

	Materials() {}
	Materials(const Vec3f& d, const Vec4f& a, const float& s, const float& r) : diffuseColor(d), albedo(a), specularExponent(s), refractiveIndex(r) {}
};