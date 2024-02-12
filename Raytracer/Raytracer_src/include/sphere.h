#pragma once

#include "materials.h"
#include "geometry.h"

struct Sphere {
	Vec3f center;
	float radius;
	Materials materials;

	Sphere(const Vec3f& c, const float& r, const Materials& m) : center(c), radius(r), materials(m) {}
	
	bool rayIntersect(const Vec3f& orig, const Vec3f& dir, float& cameraToIntersectionDistance) const {
		Vec3f origToCenter = center - orig;
		Vec3f origToCenterDir = origToCenter;
		origToCenterDir.normalize();
		if (origToCenterDir * dir > 0) {
			float tca = origToCenter * dir;
			float d2 = origToCenter * origToCenter - tca * tca;
			float thc = sqrtf(radius * radius - d2);
			cameraToIntersectionDistance = tca - thc < 0 ? tca + thc : tca - thc;
			return d2 > (radius * radius) ? false : true;
		}
		else {
			// ����ڱ�����Ȼ���������ڣ����ڱ��н���������������ѡ��ȫ������
			return false;
		}	
	}
};