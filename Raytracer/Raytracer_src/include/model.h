#ifndef __MODEL_OBJ_H__
#define __MODEL_OBJ_H__

#include "geometry.h"
#include "tgaimage.h"

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

    // ׷��
    float Kzheshe;          //�������
    float Ktoushe;          //Ͷ�����

    char map_Kd[256];                       //������ͼ·��
    std::vector<std::vector<Vec3i>> faces;  //ƬԪ������ÿ��ƬԪ�洢9��intֵ������3�������3������
    TGAImage* textureTga = new TGAImage;                    //�������
};

class Model {
private:
    std::vector<Vec3f> m_verts;      //3���������γɵĶ�������
    std::vector<Vec2f> m_uvs;        //2���������γɵ���������
    std::vector<Vec3f> m_norms;      //3���������γɵĶ��㷨������
    std::vector<Material> m_groups;  //MTL�⣬ģ��ÿһ���Ӧһ�����ʿ⣬���⻹�洢��ƬԪ����
    int m_groupCounts;               //ģ�������������Ҫ��1

public:
    Model();
    Model(const char* objFileName, const char* mtlFileName);
    Model(const Model& model);
    Model(Model&& model) noexcept;
    ~Model();

    Model& operator=(Model&& model) noexcept;
    Model& operator=(const Model& model);

    //��ȡ��������
    int getNumOfVert() const;
    //��ȡ������������
    int getNumOfUV() const;
    //��ȡ��������
    int getNumOfNorm() const;
    //��ȡƬԪ����
    int getNumOfFace() const;
    //��ȡģ��������
    int getNumOfGroup() const;
    //��ȡָ��ģ����ƬԪ����
    int getNumOfGroupFace(int groupIndex) const;

    //��ȡָ����������
    Vec3f getVert(int vertIndex) const;
    //��ȡָ��UV����
    Vec2f getUV(int uvIndex) const;
    //��ȡָ����������
    Vec3f getNorm(int groupIndex, int faceIndex, int vertIndex) const;
    //��ȡָ��ƬԪ
    std::vector<int> getGroupFace(int groupIndex, int faceIndex) const;

    //��ȡmtl�ļ�
    void readMtlFile(const char* mtlFileName);
    //����ָ��ģ����������Ϣ
    void setGroupTexture(int groupIndex);
    //��ȡģ����ָ��λ��UV��ɫ
    TGAColor getGroupUVColor(int groupIndex, int x, int y) const;
    //��ȡָ��ģ�����������
    TGAImage* getTexture(int groupIndex) const;
    //��ȡָ��ģ������ղ���
    Vec3f getKa(int groupIndex) const;
    float getKd(int groupIndex) const;
    float getKs(int groupIndex) const;
    float getToushe(int groupIndex) const;
    float getZheshe(int groupIndex) const;
    float getNs(int groupIndex) const;
    float getNi(int groupIndex) const;

};

#endif //__MODEL_OBJ_H__