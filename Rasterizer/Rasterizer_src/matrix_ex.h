#pragma once

#include "geometry_ssloy.h"
#include "global.h"

//3维坐标转换为4维矩阵
Matrix v2m(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.0f;
	return m;
}

//3维向量转换为4维矩阵
Matrix v2mn(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 0.0f;
	return m;
}

//4维矩阵转换为3维向量
Vec3f m2vn(Matrix m) {
	return Vec3f(m[0][0], m[1][0], m[2][0]);
}

//4维矩阵转换为3维坐标，除以第四分量
Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

//视口变换矩阵
Matrix viewport(float x, float y) {
	//视口变换矩阵
	//变换之后，z轴越小，越近。左下角为原点，右侧为正x轴，上侧为正y轴
	//考虑到摄像机和物体距离，这里关于z轴的变换似乎没什么意义
	Matrix m = Matrix::identity(4);
	m[0][3] = SCREEN_WIDTH / 2.0f;
	m[1][3] = SCREEN_HEIGHT / 2.0f;
	m[2][3] = 1.0f;
	//m[2][3] = ZBUFFER_DEPTH / 2.0f;

	//m[0][0] = 2 * w / 3.0f;// 可以用 w / 2.0f，即屏幕的3/4用于绘制，可以用来检查剪裁，正常情况下周围是不可见的
	//m[1][1] = 2 * h / 3.0f;// 可以用 w / 2.0f，即屏幕的3/4用于绘制，可以用来检查剪裁，正常情况下周围是不可见的
	m[0][0] = SCREEN_WIDTH * x;
	m[1][1] = SCREEN_HEIGHT * y;
	//m[2][2] = ZBUFFER_DEPTH / 2.0f;
	m[2][2] = 1.0f;
	return m;
}

//模型视图变换矩阵
Matrix lookat(Vec3f cameraEye, Vec3f cameraCenter, Vec3f cameraUp) {
	//构造以摄像机为原点，朝向负z轴的的摄像机坐标系，所以摄像机的正z轴是摄像机朝向的反向
	//经过该矩阵变换，物体从世界坐标变换到观察坐标，依旧是右手坐标系，正z轴朝向屏幕外侧
	Vec3f z = (cameraEye - cameraCenter).normalize();//注意方向
	Vec3f x = (cameraUp ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix res = Matrix::identity(4);

	//计算 摄像机相对于世界坐标系的旋转 
	Matrix rotationM = Matrix::identity(4);
	rotationM[0][0] = x[0];
	rotationM[0][1] = x[1];
	rotationM[0][2] = x[2];

	rotationM[1][0] = y[0];
	rotationM[1][1] = y[1];
	rotationM[1][2] = y[2];

	rotationM[2][0] = z[0];
	rotationM[2][1] = z[1];
	rotationM[2][2] = z[2];


	Matrix transM = Matrix::identity(4);
	transM[0][3] = -cameraEye[0];
	transM[1][3] = -cameraEye[1];
	transM[2][3] = -cameraEye[2];

	return res = rotationM * transM;

	//更快
	/*Matrix Minv;
	Minv = Matrix::identity(4);
	for (int i = 0; i < 3; i++) {
		Minv[0][i] = x[i];
		Minv[1][i] = y[i];
		Minv[2][i] = z[i];
	}
	Minv[0][3] = -1 * (x * cameraEye);
	Minv[1][3] = -1 * (y * cameraEye);
	Minv[2][3] = -1 * (z * cameraEye);

	return Minv;*/
}

//透视投影变换矩阵
Matrix setFrustum(float fovy, float aspect, float near, float far)
{
	//透视投影矩阵，将观察坐标变换在裁剪空间中，透视除法之后为标准化设备坐标。变换之后为左手坐标系，正z轴朝向屏幕内
	float z_range = far - near;
	Matrix matrix;
	matrix = Matrix::identity(4);
	matrix[1][1] = 1 / (float)tan(fovy / 2);
	matrix[0][0] = matrix[1][1] / aspect;
	matrix[2][2] = -(near + far) / z_range;
	matrix[2][3] = -2.0f * near * far / z_range;
	matrix[3][2] = -1;
	matrix[3][3] = 0;

	return matrix;
}