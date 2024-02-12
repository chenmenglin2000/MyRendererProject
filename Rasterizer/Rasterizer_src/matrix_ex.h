#pragma once

#include "geometry_ssloy.h"
#include "global.h"

//3ά����ת��Ϊ4ά����
Matrix v2m(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.0f;
	return m;
}

//3ά����ת��Ϊ4ά����
Matrix v2mn(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 0.0f;
	return m;
}

//4ά����ת��Ϊ3ά����
Vec3f m2vn(Matrix m) {
	return Vec3f(m[0][0], m[1][0], m[2][0]);
}

//4ά����ת��Ϊ3ά���꣬���Ե��ķ���
Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

//�ӿڱ任����
Matrix viewport(float x, float y) {
	//�ӿڱ任����
	//�任֮��z��ԽС��Խ�������½�Ϊԭ�㣬�Ҳ�Ϊ��x�ᣬ�ϲ�Ϊ��y��
	//���ǵ��������������룬�������z��ı任�ƺ�ûʲô����
	Matrix m = Matrix::identity(4);
	m[0][3] = SCREEN_WIDTH / 2.0f;
	m[1][3] = SCREEN_HEIGHT / 2.0f;
	m[2][3] = 1.0f;
	//m[2][3] = ZBUFFER_DEPTH / 2.0f;

	//m[0][0] = 2 * w / 3.0f;// ������ w / 2.0f������Ļ��3/4���ڻ��ƣ��������������ã������������Χ�ǲ��ɼ���
	//m[1][1] = 2 * h / 3.0f;// ������ w / 2.0f������Ļ��3/4���ڻ��ƣ��������������ã������������Χ�ǲ��ɼ���
	m[0][0] = SCREEN_WIDTH * x;
	m[1][1] = SCREEN_HEIGHT * y;
	//m[2][2] = ZBUFFER_DEPTH / 2.0f;
	m[2][2] = 1.0f;
	return m;
}

//ģ����ͼ�任����
Matrix lookat(Vec3f cameraEye, Vec3f cameraCenter, Vec3f cameraUp) {
	//�����������Ϊԭ�㣬����z��ĵ����������ϵ���������������z�������������ķ���
	//�����þ���任���������������任���۲����꣬��������������ϵ����z�ᳯ����Ļ���
	Vec3f z = (cameraEye - cameraCenter).normalize();//ע�ⷽ��
	Vec3f x = (cameraUp ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix res = Matrix::identity(4);

	//���� ������������������ϵ����ת 
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

	//����
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

//͸��ͶӰ�任����
Matrix setFrustum(float fovy, float aspect, float near, float far)
{
	//͸��ͶӰ���󣬽��۲�����任�ڲü��ռ��У�͸�ӳ���֮��Ϊ��׼���豸���ꡣ�任֮��Ϊ��������ϵ����z�ᳯ����Ļ��
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