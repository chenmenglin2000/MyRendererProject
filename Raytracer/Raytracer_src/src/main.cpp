/****************************************
 * Copyright:	Hinata
 * Author:		Hinata
 * Date:		2023-04-06
 * Project:		TinySoftRaytracer
 * Description:	1. �򵥵Ĺ���׷����Ⱦ��
				2. ֧���򡢵�ƽ�桢��պС�����Դ��objģ�͵���Ⱦ
 * Unfinished:	1. �Ż�
				2. objģ�͵���Ӱ���
				3. model�����д
 * Known Issue: 1. �������ɳ�������
				2. �����������Ƶ�
				3. ��������������Ƭ����
				4. ��ɫ����Ĵ�����
				5. ���յĹ��ռ�������դ��Ⱦ����һ��
*****************************************/

#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <parallel_for.h>
#include <mutex>

#include "../include/geometry.h"
#include "../include/light.h"
#include "../include/model.h"
#include "../include/sphere.h"
#include "../include/tgaimage.h"

#define PI float(2 * acos(0.0))

const int SCREEN_WIDTH = 500;											// ͼƬ���
const int SCREEN_HEIGHT = 500;											// ͼƬ�߶�
std::vector<Vec3f> framebuffer(SCREEN_WIDTH * SCREEN_HEIGHT);			// ͼƬ����
std::vector<Vec3f> framebufferMSAA(SCREEN_WIDTH * SCREEN_HEIGHT * 4);	// ����ݻ���
const Vec3f backgroundColor(0.0f, 0.0f, 0.0f);							// ���米����ɫ
const float fov = PI / 3.0;												// 60���ӽǣ�
const Vec3f cameraOrig(0.0f, 0.0f, 6.0f);								// ���������

// ���������ͼƬ
std::optional<TGAImage> skyBoxInput() {
	TGAImage skyBox;
	if (skyBox.read_tga_file("resources/miku.tga"))
		return skyBox;
	return {};
}

// ��������������Ĭ��ֻ�����������֮������
Vec3f computeRefractDir(const Vec3f& I, const Vec3f& N, const float& refractiveIndex) {
	float cosi = -std::max(-1.f, std::min(1.f, I * N));
	float etai = 1, etat = refractiveIndex;
	Vec3f n = N;
	if (cosi < 0)
	{
		cosi = -cosi;
		std::swap(etai, etat);
		n = -N;
	}
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? Vec3f{ 0, 0, 0 } : I * eta + n * (eta * cosi - sqrtf(k));
}

/**
 * Brief �ж���������Ƭ������ཻ
 * Param orig �������
 * Param dir ���߷���
 * Param a b c ��������Ƭ����
 * t �ཻʱ���ߵ�����ǰ������
 * bc ��������
 * return false ���ཻ
 * return true �ཻ
 */
bool rayIntersectsTriangle(const Vec3f& orig, const Vec3f& dir, const Vec3f& a, const Vec3f& b, const Vec3f& c, float& t, Vec3f& bc) {
	Vec3f e1 = b - a;
	Vec3f e2 = c - a;
	Vec3f h = cross(dir, e2);
	float aa = e1 * h;

	if (aa > -0.00001 && aa < 0.00001) return false;

	float f = 1 / aa;
	Vec3f s = orig - a;
	float u = f * (s * h);

	if (u < 0.0 || u > 1.0) return false;

	Vec3f q = cross(s, e1);
	float v = f * (dir * q);

	if (v < 0.0 || (u + v) > 1.0) return false;

	t = f * (e2 * q);

	if (t > 0.00001) {
		bc.x = 1 - u - v;
		bc.y = u;
		bc.z = v;
		return true;
	}

	return false;
}

// ������������
std::pair<int, int> computeUV(float& u, float& v, TGAImage* textureTga) {
	return std::make_pair(textureTga->get_width() * u, textureTga->get_height() * v);
}

// ȡɫ�����
Vec3f computeSkyBoxColor(Vec3f dir, const TGAImage& skyBox) {
	if (skyBox.get_height() == 0 || skyBox.get_width() == 0) {
		return backgroundColor;
	}

	// ��Ϊ������Զ�����Կ��Բ�����������������Ӱ�죬����һ����ԭ�����
	float angleY = acos(Vec3f(0.0f, 1.0f, 0.0f) * dir);
	float angleX = acos(Vec3f(0.0f, 0.0f, 1.0f) * Vec3f(dir.x, 0.0f, dir.z).normalize());
	// ��-zָ�������ͼ�����У�����angleX��������Ӱ��
	angleX = dir.x >= 0 ? angleX : (2 * PI - angleX);
	TGAColor c = skyBox.get(skyBox.get_width() * (1 - angleX / (2 * PI)), skyBox.get_height() * angleY / PI);
	return Vec3f(c[2] / 255.0f, c[1] / 255.0f, c[0] / 255.0f);
}

/**
 * Brief �ж�һ�����ߣ����ߣ��볡���е������ཻ���
 * Param orig �������
 * Param dir ���߷���
 * Param objectSphere �����е�����
 * Param objModel objģ��
 * return bool �Ƿ����ཻ
 * return Vec3f hit ����
 * return Vec3f N ���㴦������
 * return Materials ���㴦����
 */
std::tuple<bool, Vec3f, Vec3f, Materials> sceneIntersect(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& objectSphere, const std::vector<Model>& objModel) {
	Vec3f hit, N;
	Materials materials;
	float minDistance = std::numeric_limits<float>::max();
	float dis = std::numeric_limits<float>::max();

	// �������ཻ
	if (std::abs(dir.y) > 1e-3) {
		dis = -(orig.y + 4) / dir.y;
		Vec3f pt = orig + dir * dis;
		if (dis > 1e-3 && dis < minDistance && std::fabs(pt.x) < 10 && pt.z < -10 && pt.z > -30) {
			minDistance = dis;
			hit = pt;
			N = Vec3f(0.0f, 1.0f, 0.0f);
			materials = Materials(Vec3f(0.0f, 0.0f, 0.0f), Vec4f(0.9f, 0.4f, 0.3f, 0.0f), 50.0f, 1.0f);
			materials.diffuseColor = (int(0.5 * hit.x + 1000) + int(0.5 * hit.z)) & 1 ? Vec3f(0.3f, 0.3f, 0.3f) : Vec3f(0.3f, 0.2f, 0.1f);
		}
	}

	// �����ཻ
	for (const Sphere& sphere : objectSphere) {
		bool intersection = sphere.rayIntersect(orig, dir, dis);
		if (!intersection || dis > minDistance) continue;
		minDistance = dis;
		hit = orig + dir * minDistance;
		N = (hit - sphere.center).normalize();
		materials = sphere.materials;
	}

	// ��obj�ཻ
	for (const Model& model : objModel) {
		for (int g = 0; g < model.getNumOfGroup(); ++g) {
			TGAImage* textureTga = model.getTexture(g);	// ĳ��ģ������������
			Materials mesh(Vec3f(model.getKa(g)),
				Vec4f(model.getKd(g), model.getKs(g), model.getToushe(g), model.getZheshe(g)),
				model.getNs(g), model.getNi(g));		// ĳ��ģ����Ĺ�������
			for (int f = 0; f < model.getNumOfGroupFace(g); ++f) {
				std::vector<int> fragment = model.getGroupFace(g, f);
				std::vector<Vec3f> triangle;
				for (int i = 0; i < 3; i++) {
					triangle.push_back(model.getVert(fragment[i * 3]));
				}

				Vec3f bc;
				if (rayIntersectsTriangle(orig, dir, triangle[0], triangle[1], triangle[2], dis, bc) && dis < minDistance) {
					materials = mesh;		
					minDistance = dis;
					hit = orig + dir * dis;
					N = model.getNorm(g, f, 0) * bc.x + model.getNorm(g, f, 1) * bc.y + model.getNorm(g, f, 2) * bc.z;
					
					
					float u = 0.0f;
					float v = 0.0f;
					for (int i = 0; i < 3; ++i) {
						u += model.getUV(fragment[i * 3 + 1]).x * bc[i];
						v += model.getUV(fragment[i * 3 + 1]).y * bc[i];
					}
					std::pair<int, int> uvxy = computeUV(u, v, textureTga);
					TGAColor coloruv = textureTga->get(uvxy.first, uvxy.second);
					Vec3f color(coloruv[2] / 255.0f, coloruv[1] / 255.0f, coloruv[0] / 255.0f);
					materials.diffuseColor = color;
				}
			}
		}
	}

	return { minDistance < 2000, hit, N, materials };
}

/**
 * Brief ��Ͷ���������õ���ɫ
 * Param orig �������
 * Param dir ���߷���
 * Param objectSphere �����е�����
 * Param lights ��Դ
 * Param skyBox �����
 * Param objModel objģ��
 * Param depth �ݹ����
 * return Vec3f ��������õ���ɫ
 */
Vec3f castRay(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& objectSphere, const std::vector<Light>& lights, const TGAImage& skyBox, const std::vector<Model>& objModel, size_t depth = 0) {
	auto [intersection, hit, N, materials] = sceneIntersect(orig, dir, objectSphere, objModel);
	
	if (depth > 4 || !intersection) {
		return computeSkyBoxColor(dir, skyBox);
	}
	depth++;
	
	// ��������
	Vec3f reflectDir = dir - N * (dir * N) * 2;
	Vec3f reflectOrig = hit + N * 1e-3;
	Vec3f reflectColor = castRay(reflectOrig, reflectDir, objectSphere, lights, skyBox, objModel, depth);

	// ��������
	Vec3f refractDir = computeRefractDir(dir, N, materials.refractiveIndex).normalize();
	// ����������ͳ�����
	Vec3f refractOrig = refractDir * N < 0 ? hit - N * 1e-3 : hit + N * 1e-3;
	Vec3f refractColor = castRay(refractOrig, refractDir, objectSphere, lights, skyBox, objModel, depth);

	//Vec3f reflectColor = Vec3f(0.0f, 0.0f, 0.0f);
	//Vec3f refractColor = Vec3f(0.0f, 0.0f, 0.0f);
	Vec3f diffuseColor = Vec3f(0.0f, 0.0f, 0.0f);
	Vec3f specularColor = Vec3f(0.0f, 0.0f, 0.0f);
	// ��Ӱ
	for (const Light& light : lights) {
		Vec3f shadowOrig = hit + N * 1e-3;
		Vec3f shadowDir = light.position - shadowOrig;
		shadowDir.normalize();

		auto [shadowIntersection, shadowHit, shadowN, shadowMaterials] = sceneIntersect(shadowOrig, shadowDir, objectSphere, objModel);

		if (shadowIntersection && (shadowHit - hit).norm() < (light.position - hit).norm()) continue;

		Vec3f lightDir = (light.position - hit).normalize();
		diffuseColor += materials.diffuseColor * std::max(0.0f, lightDir * N) * light.intensity;
		Vec3f lightDirOut = lightDir - N * (lightDir * N) * 2;
		specularColor += Vec3f(1.0f, 1.0f, 1.0f) * powf(std::max(0.0f, lightDirOut * dir), materials.specularExponent) * light.intensity;
	}

	return diffuseColor * materials.albedo[0] + specularColor * materials.albedo[1] + reflectColor * materials.albedo[2] + refractColor * materials.albedo[3];
}

// ���Դ��룺���ɽ���ͼƬ
void renderTest() {
	for (size_t j = 0; j < SCREEN_HEIGHT; j++) {
		for (size_t i = 0; i < SCREEN_WIDTH; i++) {
			framebuffer[i + j * SCREEN_WIDTH] = Vec3f(i / float(SCREEN_WIDTH), j / float(SCREEN_HEIGHT), 0);
		}
	}
}

// ��MSAA����������������
void computeMSAABuffer() {
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			//����
			float r = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][0];
			float g = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][1];
			float b = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][2];
			//����
			r += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][2];
			//����
			r += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][2];
			//����
			r += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][2];

			//���
			r /= 4.0f;
			g /= 4.0f;
			b /= 4.0f;

			framebuffer[x + y * SCREEN_WIDTH] = Vec3f(r, g, b);
		}
	}
}

// ���������д���ļ�
void framebufferOutPut() {
	std::ofstream ofs;
	ofs.open("out/image.ppm", std::ios::out | std::ios::binary);
	if (!ofs.is_open()) {
		std::cout << "Out File Error!" << std::endl;
		return;
	}

	ofs << "P6" << '\n'
		<< SCREEN_WIDTH << ' ' << SCREEN_HEIGHT << "\n"
		<< "255" << '\n';
	for (Vec3f& c : framebuffer) {
		// 0.0f~1.0f��Χת��Ϊchar��0~255��Χ
		// ����ֱ�ӽ�ȡ�˳������޵�ֵ��̫��ѧ��RGB���г������޵�ֱ�����ţ��о������⣿�����ǲ������ظ��ˡ�
		// �������ź󣬸߹ⲿ���б�Ե�񻯵�Ч��
		float max = std::max(c[0], std::max(c[1], c[2]));
		if (max > 1.0f) c = c * (1.0f / max);
		ofs << (char)(std::max(std::min(c[0], 1.0f), 0.0f) * 255)
			<< (char)(std::max(std::min(c[1], 1.0f), 0.0f) * 255)
			<< (char)(std::max(std::min(c[2], 1.0f), 0.0f) * 255);
	}

	ofs.close();
}

/**
 * Brief ��Ⱦ
 * Param skyBox �����
 * Param objectSphere ��
 * Param lights ��Դ
 * Param objModel objģ��
 * Param MSAAFlag ����ݱ�־
 */
void render(const TGAImage& skyBox, const std::vector<Sphere>& objectSphere, const std::vector<Light>& lights, const std::vector<Model>& objModel, bool MSAAFlag) {
	float cameraToScreenOnZ = -SCREEN_HEIGHT / (2.0f * tan(fov / 2.0f));	// ���������Ļ����

	if (MSAAFlag) {
		for (int j = 0; j < SCREEN_HEIGHT * 2; j++) {
			tbb::blocked_range<int> range(0, SCREEN_WIDTH * 2);
			tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

				for (int i = range.begin(); i != range.end(); i++) {
					// ����Ͷ�����������Ͷ��
					float x = i - SCREEN_WIDTH * 2 / 2.0f + 0.5f;
					float y = -(j - SCREEN_HEIGHT * 2 / 2.0f) - 0.5f;
					x /= 2.0f;
					y /= 2.0f;
					Vec3f dir = Vec3f(x, y, cameraToScreenOnZ) - cameraOrig;
					dir.normalize();
					framebufferMSAA[j * SCREEN_WIDTH * 2 + i] = castRay(cameraOrig, dir, objectSphere, lights, skyBox, objModel);
				}

				});
			std::cout << "��" << j << "����ɼ���..." << std::endl;
		}
		computeMSAABuffer();
	}
	else {
		for (size_t j = 0; j < SCREEN_HEIGHT; j++) {
			tbb::blocked_range<int> range(0, SCREEN_WIDTH);
			tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

				for (int i = range.begin(); i != range.end(); i++) {
					float x = i - SCREEN_WIDTH / 2.0f + 0.5f;
					float y = -(j - SCREEN_HEIGHT / 2.0f) - 0.5f;
					Vec3f dir = Vec3f(x, y, cameraToScreenOnZ) - cameraOrig;
					dir.normalize();
					framebuffer[j * SCREEN_WIDTH + i] = castRay(cameraOrig, dir, objectSphere, lights, skyBox, objModel);
				}

				});
			std::cout << "��" << j << "����ɼ���..." << std::endl;
		}
	}

	framebufferOutPut();
}

int main(int argc, char* argv[]) {
	// ���������
	auto skyBoxResult = skyBoxInput();
	if (!skyBoxResult.has_value()) {
		std::cout << "SkyBox File Open Error!" << std::endl;
	}
	TGAImage skyBox = skyBoxResult.value_or(TGAImage());

	// ��������
	Materials yellow_material(Vec3f{ 0.68, 0.74, 0.16 }, Vec4f{ 0.4, 0.3, 0.1, 0.0 }, 50, 1.0);
	Materials red_material(Vec3f{ 0.8, 0.32, 0.32 }, Vec4f{ 0.3, 0.1, 0.0, 0.0 }, 10, 1.0);
	Materials mirror(Vec3f{ 1.0, 1.0, 1.0 }, Vec4f{ 0.0, 10.0, 0.8, 0.0 }, 1425, 1.0);
	Materials glass(Vec3f{ 0.6, 0.7, 0.8 }, Vec4f{ 0.0, 0.5, 0.1, 0.8 }, 125, 1.5);
	
	std::vector<Sphere> objectSphere;	// ����Ⱦ�������
	std::vector<Light> lights;			// ��Դ
	std::vector<Model> objModel;		// objģ��

	// ������
	objectSphere.push_back(Sphere(Vec3f{ -3, 0, -16 }, 2, yellow_material));
	objectSphere.push_back(Sphere(Vec3f{ -1.0, -1.5, -12 }, 2, glass));
	objectSphere.push_back(Sphere(Vec3f{ 1.5, -0.5, -18 }, 3, red_material));
	objectSphere.push_back(Sphere(Vec3f{ 7, 5, -18 }, 4, mirror));

	// ������Դ
	lights.push_back(Light(Vec3f{ -20, 20, 20 }, 1.3));
	lights.push_back(Light(Vec3f{ 30, 50, -25 }, 1.3));
	lights.push_back(Light(Vec3f{ 30, 20, 30 }, 1.9));
	lights.push_back(Light(Vec3f{ 0, 0, 30 }, 1.7));

	// ����objģ��
	//objModel.push_back(std::move(Model("resources/obj/xiao.obj", "resources/obj/xiao.mtl")));
	objModel.push_back(std::move(Model("resources/obj/cube.obj", "resources/obj/cube.mtl")));

	bool MSAAFlag = true;	// �Ƿ��������
	render(skyBox, objectSphere, lights, objModel, MSAAFlag);

	return 0;
}