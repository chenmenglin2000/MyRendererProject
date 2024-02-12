#include "model_obj.h"

#include <string>
#include <sstream>

Model::Model(const char* objFileName, const char* mtlFileName) : m_verts(), m_groups(), m_norms(), m_uvs(), m_groupCounts() {
    std::ifstream in;
    in.open(objFileName, std::ifstream::in);
    if (in.fail()) return;
    std::string line;

    m_groupCounts = -1;

    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            m_verts.push_back(v);
        }
        else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            m_norms.push_back(n);
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            m_uvs.push_back(uv);
        }
        else if (!line.compare(0, 2, "g ")) {
            m_groupCounts++;
            Material m;
            m_groups.push_back(m);
            iss >> trash;
            iss >> m_groups[m_groupCounts].groupName;
        }
        else if (!line.compare(0, 7, "usemtl ")) {
            iss >> trash >> trash >> trash >> trash >> trash >> trash;
            iss >> m_groups[m_groupCounts].mtlName;
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                //在obj模型中，所有索引都是从1开始而不是0
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            m_groups[m_groupCounts].faces.push_back(f);
        }
    }
    in.close();
    readMtlFile(mtlFileName);
}

Model::~Model() {
    for (int i = 0; i < m_groups.size(); i++) {
        delete m_groups[i].textureTga;
    }
}

int Model::getNumOfVert() {
    return m_verts.size();
}

int Model::getNumOfUV() {
    return m_uvs.size();
}

int Model::getNumOfNorm() {
    return m_norms.size();
}

int Model::getNumOfFace() {
    int n = 0;
    for (int i = 0; i <= m_groupCounts; i++) {
        n += m_groups[m_groupCounts].faces.size();
    }
    return n;
}

int Model::getNumOfGroup() {
    return m_groupCounts + 1;
}

int Model::getNumOfGroupFace(int groupIndex) {
    return m_groups[groupIndex].faces.size();
}

Vec3f Model::getVert(int vertIndex) {
    return m_verts[vertIndex];
}

Vec2f Model::getUV(int uvIndex) {
    return m_uvs[uvIndex];
}

Vec3f Model::getNorm(int groupIndex, int faceIndex, int vertIndex) {
    int idx = m_groups[groupIndex].faces[faceIndex][vertIndex][2];
    return m_norms[idx].normalize();
}

std::vector<int> Model::getGroupFace(int groupIndex, int faceIndex) {
    std::vector<int> face;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            face.push_back(m_groups[groupIndex].faces[faceIndex][i][j]);
        }
    }
    return face;
}

void Model::readMtlFile(const char* mtlFileName) {
    std::ifstream in;
    in.open(mtlFileName, std::ifstream::in);
    if (in.fail()) return;
    std::string line;

    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 7, "newmtl ")) {
            iss >> trash >> trash >> trash >> trash >> trash >> trash;
            char name[256] = "";
            iss >> name;
            for (int i = 0; i <= m_groupCounts; i++) {
                if (strcmp(name, m_groups[i].mtlName) == 0) {
                    for (int ii = 0; ii < 9; ii++) {
                        std::getline(in, line);
                        std::istringstream isss(line.c_str());
                        if (!line.compare(0, 3, "Ns ")) {
                            isss >> trash >> trash;
                            float Ns = 0.;
                            isss >> Ns;
                            m_groups[i].Ns = Ns;
                        }
                        else if (!line.compare(0, 3, "Ka ")) {
                            isss >> trash >> trash;
                            float n[3];
                            for (int j = 0; j < 3; j++) {
                                isss >> n[j];
                                m_groups[i].Ka[j] = n[j];
                            }
                        }
                        else if (!line.compare(0, 3, "Kd ")) {
                            isss >> trash >> trash;
                            float n[3];
                            for (int j = 0; j < 3; j++) {
                                isss >> n[j];
                                m_groups[i].Kd[j] = n[j];
                            }
                        }
                        else if (!line.compare(0, 3, "Ks ")) {
                            isss >> trash >> trash;
                            float n[3];
                            for (int j = 0; j < 3; j++) {
                                isss >> n[j];
                                m_groups[i].Ks[j] = n[j];
                            }
                        }
                        else if (!line.compare(0, 3, "Ke ")) {
                            isss >> trash >> trash;
                            double n[3];
                            for (int j = 0; j < 3; j++) {
                                isss >> n[j];
                                m_groups[i].Ke[j] = n[j];
                            }
                        }
                        else if (!line.compare(0, 3, "Ni ")) {
                            isss >> trash >> trash;
                            double Ni = 0.;
                            isss >> Ni;
                            m_groups[i].Ni = Ni;
                        }
                        else if (!line.compare(0, 2, "d ")) {
                            isss >> trash;
                            double d = 0.;
                            isss >> d;
                            m_groups[i].d = d;
                        }
                        else if (!line.compare(0, 6, "illum ")) {
                            isss >> trash >> trash >> trash >> trash >> trash;
                            int n = 0;
                            isss >> n;
                            m_groups[i].illum = n;
                        }
                        else if (!line.compare(0, 7, "map_Kd ")) {
                            isss >> trash >> trash >> trash >> trash >> trash >> trash;
                            isss >> m_groups[i].map_Kd;
                            setGroupTexture(i);
                        }
                    }
                    break;
                }
            }
        }
    }

    in.close();
}

void Model::setGroupTexture(int groupIndex) {
    std::string s(&m_groups[groupIndex].map_Kd[0], &m_groups[groupIndex].map_Kd[strlen(m_groups[groupIndex].map_Kd)]);
    m_groups[groupIndex].textureTga->read_tga_file("obj/Texture/" + s);
    m_groups[groupIndex].textureTga->flip_vertically();
}

TGAColor Model::getGroupUVColor(int groupIndex, int x, int y) {
    TGAColor color = m_groups[groupIndex].textureTga->get(x, y);
    return color;
}

TGAImage* Model::getTexture(int groupIndex) {
    return m_groups[groupIndex].textureTga;
}

std::vector<float> Model::getKaKdKsNs(int groupIndex) {
    std::vector<float> KN;
    KN.push_back(m_groups[groupIndex].Ka[0]);
    KN.push_back(m_groups[groupIndex].Kd[0]);
    KN.push_back(m_groups[groupIndex].Ks[0]);
    KN.push_back(m_groups[groupIndex].Ns);
    return KN;
}