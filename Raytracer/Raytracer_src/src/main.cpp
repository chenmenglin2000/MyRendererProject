/****************************************
 * Copyright:	Hinata
 * Author:		Hinata
 * Date:		2023-04-06
 * Project:		TinySoftRaytracer
 * Description:	1. 简单的光线追踪渲染器
				2. 支持球、地平面、天空盒、多点光源、obj模型的渲染
 * Unfinished:	1. 优化
				2. obj模型的阴影检测
				3. model类的重写
 * Known Issue: 1. 棋盘生成程序的理解
				2. 折射向量的推导
				3. 光线与三角形面片的求交
				4. 颜色溢出的处理方法
				5. 最终的光照计算和软光栅渲染器不一样
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

const int SCREEN_WIDTH = 500;											// 图片宽度
const int SCREEN_HEIGHT = 500;											// 图片高度
std::vector<Vec3f> framebuffer(SCREEN_WIDTH * SCREEN_HEIGHT);			// 图片缓冲
std::vector<Vec3f> framebufferMSAA(SCREEN_WIDTH * SCREEN_HEIGHT * 4);	// 抗锯齿缓冲
const Vec3f backgroundColor(0.0f, 0.0f, 0.0f);							// 世界背景颜色
const float fov = PI / 3.0;												// 60度视角？
const Vec3f cameraOrig(0.0f, 0.0f, 6.0f);								// 摄像机坐标

// 读入天空球图片
std::optional<TGAImage> skyBoxInput() {
	TGAImage skyBox;
	if (skyBox.read_tga_file("resources/miku.tga"))
		return skyBox;
	return {};
}

// 计算折射向量，默认只在物体与空气之间折射
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
 * Brief 判断三角形面片与光线相交
 * Param orig 光线起点
 * Param dir 光线方向
 * Param a b c 三角形面片顶点
 * t 相交时光线到交点前进距离
 * bc 重心坐标
 * return false 不相交
 * return true 相交
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

// 计算纹理坐标
std::pair<int, int> computeUV(float& u, float& v, TGAImage* textureTga) {
	return std::make_pair(textureTga->get_width() * u, textureTga->get_height() * v);
}

// 取色天空球
Vec3f computeSkyBoxColor(Vec3f dir, const TGAImage& skyBox) {
	if (skyBox.get_height() == 0 || skyBox.get_width() == 0) {
		return backgroundColor;
	}

	// 因为天空球很远，所以可以不受摄像机世界坐标的影响，这里一律用原点计算
	float angleY = acos(Vec3f(0.0f, 1.0f, 0.0f) * dir);
	float angleX = acos(Vec3f(0.0f, 0.0f, 1.0f) * Vec3f(dir.x, 0.0f, dir.z).normalize());
	// 让-z指向天空球图像正中，并且angleX的正负有影响
	angleX = dir.x >= 0 ? angleX : (2 * PI - angleX);
	TGAColor c = skyBox.get(skyBox.get_width() * (1 - angleX / (2 * PI)), skyBox.get_height() * angleY / PI);
	return Vec3f(c[2] / 255.0f, c[1] / 255.0f, c[0] / 255.0f);
}

/**
 * Brief 判断一个射线（光线）与场景中的物体相交情况
 * Param orig 射线起点
 * Param dir 射线方向
 * Param objectSphere 场景中的球体
 * Param objModel obj模型
 * return bool 是否有相交
 * return Vec3f hit 交点
 * return Vec3f N 交点处法向量
 * return Materials 交点处材质
 */
std::tuple<bool, Vec3f, Vec3f, Materials> sceneIntersect(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& objectSphere, const std::vector<Model>& objModel) {
	Vec3f hit, N;
	Materials materials;
	float minDistance = std::numeric_limits<float>::max();
	float dis = std::numeric_limits<float>::max();

	// 与棋盘相交
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

	// 与球相交
	for (const Sphere& sphere : objectSphere) {
		bool intersection = sphere.rayIntersect(orig, dir, dis);
		if (!intersection || dis > minDistance) continue;
		minDistance = dis;
		hit = orig + dir * minDistance;
		N = (hit - sphere.center).normalize();
		materials = sphere.materials;
	}

	// 与obj相交
	for (const Model& model : objModel) {
		for (int g = 0; g < model.getNumOfGroup(); ++g) {
			TGAImage* textureTga = model.getTexture(g);	// 某个模型组的纹理对象
			Materials mesh(Vec3f(model.getKa(g)),
				Vec4f(model.getKd(g), model.getKs(g), model.getToushe(g), model.getZheshe(g)),
				model.getNs(g), model.getNi(g));		// 某个模型组的光照数据
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
 * Brief 求投射光线所获得的颜色
 * Param orig 射线起点
 * Param dir 射线方向
 * Param objectSphere 场景中的球体
 * Param lights 光源
 * Param skyBox 天空球
 * Param objModel obj模型
 * Param depth 递归深度
 * return Vec3f 光线所获得的颜色
 */
Vec3f castRay(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& objectSphere, const std::vector<Light>& lights, const TGAImage& skyBox, const std::vector<Model>& objModel, size_t depth = 0) {
	auto [intersection, hit, N, materials] = sceneIntersect(orig, dir, objectSphere, objModel);
	
	if (depth > 4 || !intersection) {
		return computeSkyBoxColor(dir, skyBox);
	}
	depth++;
	
	// 反射光计算
	Vec3f reflectDir = dir - N * (dir * N) * 2;
	Vec3f reflectOrig = hit + N * 1e-3;
	Vec3f reflectColor = castRay(reflectOrig, reflectDir, objectSphere, lights, skyBox, objModel, depth);

	// 折射光计算
	Vec3f refractDir = computeRefractDir(dir, N, materials.refractiveIndex).normalize();
	// 折射入物体和出物体
	Vec3f refractOrig = refractDir * N < 0 ? hit - N * 1e-3 : hit + N * 1e-3;
	Vec3f refractColor = castRay(refractOrig, refractDir, objectSphere, lights, skyBox, objModel, depth);

	//Vec3f reflectColor = Vec3f(0.0f, 0.0f, 0.0f);
	//Vec3f refractColor = Vec3f(0.0f, 0.0f, 0.0f);
	Vec3f diffuseColor = Vec3f(0.0f, 0.0f, 0.0f);
	Vec3f specularColor = Vec3f(0.0f, 0.0f, 0.0f);
	// 阴影
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

// 测试代码：生成渐变图片
void renderTest() {
	for (size_t j = 0; j < SCREEN_HEIGHT; j++) {
		for (size_t i = 0; i < SCREEN_WIDTH; i++) {
			framebuffer[i + j * SCREEN_WIDTH] = Vec3f(i / float(SCREEN_WIDTH), j / float(SCREEN_HEIGHT), 0);
		}
	}
}

// 将MSAA缓冲求均到输出缓冲
void computeMSAABuffer() {
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			//左上
			float r = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][0];
			float g = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][1];
			float b = framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x][2];
			//右上
			r += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][2];
			//左下
			r += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][2];
			//右下
			r += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][0];
			g += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][1];
			b += framebufferMSAA[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][2];

			//求均
			r /= 4.0f;
			g /= 4.0f;
			b /= 4.0f;

			framebuffer[x + y * SCREEN_WIDTH] = Vec3f(r, g, b);
		}
	}
}

// 将输出缓冲写入文件
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
		// 0.0f~1.0f范围转换为char的0~255范围
		// 看似直接截取了超过上限的值不太数学？RGB中有超过上限的直接缩放，感觉有问题？而且是不是做重复了。
		// 做完缩放后，高光部分有边缘锐化的效果
		float max = std::max(c[0], std::max(c[1], c[2]));
		if (max > 1.0f) c = c * (1.0f / max);
		ofs << (char)(std::max(std::min(c[0], 1.0f), 0.0f) * 255)
			<< (char)(std::max(std::min(c[1], 1.0f), 0.0f) * 255)
			<< (char)(std::max(std::min(c[2], 1.0f), 0.0f) * 255);
	}

	ofs.close();
}

/**
 * Brief 渲染
 * Param skyBox 天空球
 * Param objectSphere 球
 * Param lights 光源
 * Param objModel obj模型
 * Param MSAAFlag 抗锯齿标志
 */
void render(const TGAImage& skyBox, const std::vector<Sphere>& objectSphere, const std::vector<Light>& lights, const std::vector<Model>& objModel, bool MSAAFlag) {
	float cameraToScreenOnZ = -SCREEN_HEIGHT / (2.0f * tan(fov / 2.0f));	// 摄像机到屏幕距离

	if (MSAAFlag) {
		for (int j = 0; j < SCREEN_HEIGHT * 2; j++) {
			tbb::blocked_range<int> range(0, SCREEN_WIDTH * 2);
			tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

				for (int i = range.begin(); i != range.end(); i++) {
					// 计算投射光线向量并投射
					float x = i - SCREEN_WIDTH * 2 / 2.0f + 0.5f;
					float y = -(j - SCREEN_HEIGHT * 2 / 2.0f) - 0.5f;
					x /= 2.0f;
					y /= 2.0f;
					Vec3f dir = Vec3f(x, y, cameraToScreenOnZ) - cameraOrig;
					dir.normalize();
					framebufferMSAA[j * SCREEN_WIDTH * 2 + i] = castRay(cameraOrig, dir, objectSphere, lights, skyBox, objModel);
				}

				});
			std::cout << "第" << j << "行完成计算..." << std::endl;
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
			std::cout << "第" << j << "行完成计算..." << std::endl;
		}
	}

	framebufferOutPut();
}

int main(int argc, char* argv[]) {
	// 创建天空球
	auto skyBoxResult = skyBoxInput();
	if (!skyBoxResult.has_value()) {
		std::cout << "SkyBox File Open Error!" << std::endl;
	}
	TGAImage skyBox = skyBoxResult.value_or(TGAImage());

	// 创建材质
	Materials yellow_material(Vec3f{ 0.68, 0.74, 0.16 }, Vec4f{ 0.4, 0.3, 0.1, 0.0 }, 50, 1.0);
	Materials red_material(Vec3f{ 0.8, 0.32, 0.32 }, Vec4f{ 0.3, 0.1, 0.0, 0.0 }, 10, 1.0);
	Materials mirror(Vec3f{ 1.0, 1.0, 1.0 }, Vec4f{ 0.0, 10.0, 0.8, 0.0 }, 1425, 1.0);
	Materials glass(Vec3f{ 0.6, 0.7, 0.8 }, Vec4f{ 0.0, 0.5, 0.1, 0.8 }, 125, 1.5);
	
	std::vector<Sphere> objectSphere;	// 被渲染的球对象
	std::vector<Light> lights;			// 光源
	std::vector<Model> objModel;		// obj模型

	// 创建球
	objectSphere.push_back(Sphere(Vec3f{ -3, 0, -16 }, 2, yellow_material));
	objectSphere.push_back(Sphere(Vec3f{ -1.0, -1.5, -12 }, 2, glass));
	objectSphere.push_back(Sphere(Vec3f{ 1.5, -0.5, -18 }, 3, red_material));
	objectSphere.push_back(Sphere(Vec3f{ 7, 5, -18 }, 4, mirror));

	// 创建光源
	lights.push_back(Light(Vec3f{ -20, 20, 20 }, 1.3));
	lights.push_back(Light(Vec3f{ 30, 50, -25 }, 1.3));
	lights.push_back(Light(Vec3f{ 30, 20, 30 }, 1.9));
	lights.push_back(Light(Vec3f{ 0, 0, 30 }, 1.7));

	// 创建obj模型
	//objModel.push_back(std::move(Model("resources/obj/xiao.obj", "resources/obj/xiao.mtl")));
	objModel.push_back(std::move(Model("resources/obj/cube.obj", "resources/obj/cube.mtl")));

	bool MSAAFlag = true;	// 是否开启抗锯齿
	render(skyBox, objectSphere, lights, objModel, MSAAFlag);

	return 0;
}