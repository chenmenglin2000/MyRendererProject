#ifndef __MODEL_OBJ_H__
#define __MODEL_OBJ_H__

#include "geometry.h"
#include "tgaimage.h"

struct Material
{
    char groupName[256];    //模型组的名字
    char mtlName[256];      //该模型组对应材质库的名字

    float Ns;               //镜面反射权重，反射范围？幂
    float Ka[3];            //环境光参数
    float Kd[3];            //漫反射参数
    float Ks[3];            //镜面反射参数
    double Ke[3];           //自发光参数
    double Ni;              //折射值
    double d;               //渐隐系数
    int illum;              //光照模型

    // 追加
    float Kzheshe;          //折射参数
    float Ktoushe;          //投射参数

    char map_Kd[256];                       //纹理贴图路径
    std::vector<std::vector<Vec3i>> faces;  //片元索引，每个片元存储9个int值，包含3个顶点的3个属性
    TGAImage* textureTga = new TGAImage;                    //纹理对象
};

class Model {
private:
    std::vector<Vec3f> m_verts;      //3个浮点数形成的顶点坐标
    std::vector<Vec2f> m_uvs;        //2个浮点数形成的纹理坐标
    std::vector<Vec3f> m_norms;      //3个浮点数形成的顶点法线向量
    std::vector<Material> m_groups;  //MTL库，模型每一组对应一个材质库，额外还存储了片元索引
    int m_groupCounts;               //模型组的数量，需要加1

public:
    Model();
    Model(const char* objFileName, const char* mtlFileName);
    Model(const Model& model);
    Model(Model&& model) noexcept;
    ~Model();

    Model& operator=(Model&& model) noexcept;
    Model& operator=(const Model& model);

    //获取顶点数量
    int getNumOfVert() const;
    //获取纹理坐标数量
    int getNumOfUV() const;
    //获取法向数量
    int getNumOfNorm() const;
    //获取片元数量
    int getNumOfFace() const;
    //获取模型组数量
    int getNumOfGroup() const;
    //获取指定模型组片元数量
    int getNumOfGroupFace(int groupIndex) const;

    //获取指定顶点坐标
    Vec3f getVert(int vertIndex) const;
    //获取指定UV坐标
    Vec2f getUV(int uvIndex) const;
    //获取指定法线向量
    Vec3f getNorm(int groupIndex, int faceIndex, int vertIndex) const;
    //获取指定片元
    std::vector<int> getGroupFace(int groupIndex, int faceIndex) const;

    //读取mtl文件
    void readMtlFile(const char* mtlFileName);
    //设置指定模型组纹理信息
    void setGroupTexture(int groupIndex);
    //获取模型组指定位置UV颜色
    TGAColor getGroupUVColor(int groupIndex, int x, int y) const;
    //获取指定模型组纹理对象
    TGAImage* getTexture(int groupIndex) const;
    //获取指定模型组光照参数
    Vec3f getKa(int groupIndex) const;
    float getKd(int groupIndex) const;
    float getKs(int groupIndex) const;
    float getToushe(int groupIndex) const;
    float getZheshe(int groupIndex) const;
    float getNs(int groupIndex) const;
    float getNi(int groupIndex) const;

};

#endif //__MODEL_OBJ_H__