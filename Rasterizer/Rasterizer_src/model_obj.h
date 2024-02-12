#ifndef __MODEL_OBJ_H__
#define __MODEL_OBJ_H__

#include "geometry_ssloy.h"
#include "tgaimage_ssloy.h"

struct Material
{
    char groupName[256];    //ģ���������
    char mtlName[256];      //��ģ�����Ӧ���ʿ������

    float Ns;               //���淴��Ȩ�أ����䷶Χ����
    float Ka[3];            //���������
    float Kd[3];            //���������
    float Ks[3];            //���淴�����
    double Ke[3];           //�Է������
    double Ni;              //����ֵ
    double d;               //����ϵ��
    int illum;              //����ģ��

    char map_Kd[256];                       //������ͼ·��
    std::vector<std::vector<Vec3i>> faces;  //ƬԪ������ÿ��ƬԪ�洢9��intֵ������3�������3������
    TGAImage* textureTga = new TGAImage;    //�������
};

class Model {
private:
    std::vector<Vec3f> m_verts;      //3���������γɵĶ�������
    std::vector<Vec2f> m_uvs;        //2���������γɵ���������
    std::vector<Vec3f> m_norms;      //3���������γɵĶ��㷨������
    std::vector<Material> m_groups;  //MTL�⣬ģ��ÿһ���Ӧһ�����ʿ⣬���⻹�洢��ƬԪ����
    int m_groupCounts;               //ģ�������������Ҫ��1

public:
    Model(const char* objFileName, const char* mtlFileName);
    ~Model();

    //��ȡ��������
    int getNumOfVert();
    //��ȡ������������
    int getNumOfUV();
    //��ȡ��������
    int getNumOfNorm();
    //��ȡƬԪ����
    int getNumOfFace();
    //��ȡģ��������
    int getNumOfGroup();
    //��ȡָ��ģ����ƬԪ����
    int getNumOfGroupFace(int groupIndex);

    //��ȡָ����������
    Vec3f getVert(int vertIndex);
    //��ȡָ��UV����
    Vec2f getUV(int uvIndex);
    //��ȡָ����������
    Vec3f getNorm(int groupIndex, int faceIndex, int vertIndex);
    //��ȡָ��ƬԪ
    std::vector<int> getGroupFace(int groupIndex, int faceIndex);

    //��ȡmtl�ļ�
    void readMtlFile(const char* mtlFileName);
    //����ָ��ģ����������Ϣ
    void setGroupTexture(int groupIndex);
    //��ȡģ����ָ��λ��UV��ɫ
    TGAColor getGroupUVColor(int groupIndex, int x, int y);
    //��ȡָ��ģ�����������
    TGAImage* getTexture(int groupIndex);
    //��ȡָ��ģ������ղ���
    std::vector<float> getKaKdKsNs(int groupIndex);
};

#endif //__MODEL_OBJ_H__