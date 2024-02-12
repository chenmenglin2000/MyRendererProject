#pragma once

#define PI float(2 * acos(0.0))
#define DegToRad PI / 180.0f

std::mutex mtx;

int SCREEN_WIDTH = 600;						//��Ļ���
int SCREEN_HEIGHT = 600;					//��Ļ�߶�
const int ZBUFFER_DEPTH = 255;				//��Ȼ�����������ֵ

struct Color {
public:
	Color(int rr, int gg, int bb) : r(rr), g(gg), b(bb) {};
	int r;
	int g;
	int b;
};
std::vector<TGAColor> g_colorBuffer;		//��ɫ������
std::vector<TGAColor> g_smallColorBuffer;	//��ͨ��ɫ������
std::vector<Color> g_colorOptBuffer;		//�Ż���ɫ������
std::vector<Color> g_colorOpt8xMSAABuffer;	//�Ż���8xMSAA��ɫ������
int* g_smallColorCount;						//�Ż���ɫˢ�´���
float* g_MSAAZBuffer = NULL;				//MSAA��Ӧ��Ȼ���������������Ȼ�������4����С
float* g_8xMSAAZBuffer = NULL;				//8xMSAA��Ӧ��Ȼ���������������Ȼ�������16����С
float* g_NoneMSAAZBuffer = NULL;			//��Ȼ�����
float* g_ModelshadowMapBuffer0 = NULL;		//ģ��1����Ӱ������
float* g_ModelshadowMapBuffer1 = NULL;		//ģ��2����Ӱ��ɫ������
float* g_ModelshadowMapBuffer2 = NULL;		//ģ��3����Ӱ��ɫ������
float* g_ModelshadowMapBuffer3 = NULL;		//ģ��4����Ӱ��ɫ������
float* g_ModelshadowMapBuffer4 = NULL;		//ģ��5����Ӱ��ɫ������
int g_bufferIndex;							//��Ӱ����������

std::vector<Model*> g_modelList;			//ģ���б�
Model* g_model = NULL;						//��ǰ��Ⱦģ��ָ��
TGAImage* g_textureTga = NULL;				//��ǰ��Ⱦģ����ͼָ��
std::vector<Button*> g_renderControlList;	//��Ⱦģʽ���ư�ť�б�
std::vector<Button*> g_modelControlList;	//ģ�Ϳ��ư�ť�б�
std::vector<Button*> g_windowControlList;	//���ڿ��ư�ť�б�
Button* btn10 = NULL;						//FPS�ı���ʾ��ť
Button* btn11 = NULL;						//FPS������ʾ��ť
Button* btn18 = NULL;						//��ť�������ư�ť
bool g_gotoFlag = false;					//��ת����

Vec3f g_lightDir = Vec3f(-2.0f, -0.5f, -3.0f).normalize();	//ƽ�й�ķ���
Vec3f g_lightDirn = Vec3f(2.0f, 0.5f, 3.0f).normalize();	//�Ա���Ϊ��������ƽ�й�ķ��򣨷���
Vec3f g_lightPos(4.0f, 1.0f, 6.0f);							//���Դλ��
float g_illum = 39.0f;										//���Դ�ھ���Ϊ1������ĵ�λ�����ǿ��

std::vector<Vec3f> g_coordinatesVector;		//����������������GUI
std::vector<Vec3f> g_lightPosVector;		//���Դ����������GUI

Vec3f cameraEye(0.0f, 0.0f, 5.0f);			//�����λ��
Vec3f cameraCenter(0.0f, 0.0f, 0.0f);		//������ӵ㣬���������λ��
Vec3f cameraUp(0.0f, 1.0f, 0.0f);			//��������Ϸ���
float ASPECT = 1.0f;						//��׶���߱�
float FAR = 10.0f;							//��׶��Զƽ��
float NEAR = 0.1f;							//��׶���ƽ��
float FOVY = 60.0f;							//��׶��Ƕ�

Matrix LModelView;							//�Թ�ԴΪ������������ModelView����
Matrix LProjectionMatrix;					//�Թ�ԴΪ������������ProjectionMatrix����
Matrix LViewPort;							//�Թ�ԴΪ������������ViewPort����
Matrix LMVP;								//�Թ�ԴΪ������������MVP����

SDL_Window* g_window = NULL;				//SDL����ָ��
SDL_Renderer* g_renderer = NULL;			//SDL������Ⱦָ��
TTF_Font* g_font = NULL;					//TTFָ��

bool g_frameRenderFlag = false;				//�߿�ģʽ��Ⱦ���Ʊ�ʶ
bool g_materialRenderFlag = false;			//����Ԥ��ģʽ��Ⱦ���Ʊ�ʶ
bool g_textureRenderFlag = false;			//����Ԥ��ģʽ��Ⱦ���Ʊ�ʶ
bool g_flatRenderFlag = false;				//flatԤ��ģʽ��Ⱦ���Ʊ�ʶ
bool g_renderFlag = false;					//����������ݺ���Ӱ����Ⱦ���Ʊ�ʶ
bool g_shadowNMRenderFlag = false;			//����������ݵ���Ӱ��Ⱦ��ʶ
bool g_MSAARenderFlag = false;				//��������Ӱ����Ⱦ���Ʊ�ʶ
bool g_shadowRenderFlag = false;			//������Ӱ����Ⱦ���Ʊ�ʶ
bool g_coordinatesRenderFlag = false;		//��������Ⱦ���Ʊ�ʶ
bool g_lightPosRenderFlag = false;			//��Դλ����Ⱦ���Ʊ�ʶ
bool g_buttonControlFlag = true;			//��ť�������Ʊ�ʶ

float g_perOpt = 0.0f;						//ƽ���Ż�Ч��
int g_faceFragmentCount = 0;				//��ǰģ������Ƭ����ɫ��Ƭ������

//����Ϊ��ť�����࣬����ʵ�ֲ�ͬ��ť��Ϊ����ʽ

//�����߿���Ⱦ�İ�ť
class ButtonFrameRender : public Button {
public:
	ButtonFrameRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_frameRenderFlag = true;

		//��������־��ʼ��
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
		g_shadowNMRenderFlag = false;
		g_flatRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_frameRenderFlag = false;
	};
};

//����flat��Ⱦ�İ�ť
class ButtonFlatRender : public Button {
public:
	ButtonFlatRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_flatRenderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_shadowNMRenderFlag = false;
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_flatRenderFlag = false;
	};
};

//���Ʋ���Ԥ����Ⱦ�İ�ť
class ButtonMaterialRender : public Button {
public:
	ButtonMaterialRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_materialRenderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
		g_flatRenderFlag = false;
		g_shadowNMRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_materialRenderFlag = false;
	};
};

//��������Ԥ����Ⱦ�İ�ť
class ButtonTextureRender : public Button {
public:
	ButtonTextureRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_textureRenderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_materialRenderFlag = false;
		g_renderFlag = false;
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
		g_flatRenderFlag = false;
		g_shadowNMRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_textureRenderFlag = false;
	};
};

//���Ʋ���������ݺ���Ӱ��Ⱦ�İ�ť
class ButtonRender : public Button {
public:
	ButtonRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_renderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
		g_flatRenderFlag = false;
		g_shadowNMRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_renderFlag = false;
	};
};

//���Ʋ���������ݵ���Ӱ��Ⱦ�İ�ť
class ButtonShadowNMRender : public Button {
public:
	ButtonShadowNMRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_shadowNMRenderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_flatRenderFlag = false;
		g_shadowRenderFlag = false;
		g_MSAARenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_shadowNMRenderFlag = false;
	};
};

//���Ʋ�������Ӱ��Ⱦ�İ�ť
class ButtonMSAARender : public Button {
public:
	ButtonMSAARender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_MSAARenderFlag = true;

		//��������־��ʼ��
		g_shadowRenderFlag = false;
		g_frameRenderFlag = false;
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_flatRenderFlag = false;
		g_shadowNMRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_MSAARenderFlag = false;
	};
};

//���ư�����Ӱ��Ⱦ�İ�ť
class ButtonShadowRender : public Button {
public:
	ButtonShadowRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_MSAARenderFlag = true;
		g_shadowRenderFlag = true;

		//��������־��ʼ��
		g_frameRenderFlag = false;
		g_materialRenderFlag = false;
		g_textureRenderFlag = false;
		g_renderFlag = false;
		g_flatRenderFlag = false;
		g_shadowNMRenderFlag = false;
	};
	virtual void onClickedSecond() {
		g_MSAARenderFlag = false;
		g_shadowRenderFlag = false;
	};
};

//����ģ��1��Ⱦ��ʾ�İ�ť
class ButtonModel0 : public Button {
public:
	ButtonModel0(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[0];
		g_bufferIndex = 0;
	};
};

//����ģ��2��Ⱦ��ʾ�İ�ť
class ButtonModel1 : public Button {
public:
	ButtonModel1(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[1];
		g_bufferIndex = 1;
	};
};

//����ģ��3��Ⱦ��ʾ�İ�ť
class ButtonModel2 : public Button {
public:
	ButtonModel2(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[2];
		g_bufferIndex = 2;
	};
};

//����ģ��4��Ⱦ��ʾ�İ�ť
class ButtonModel3 : public Button {
public:
	ButtonModel3(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[3];
		g_bufferIndex = 3;
	};
};

//����ģ��5��Ⱦ��ʾ�İ�ť
class ButtonModel4 : public Button {
public:
	ButtonModel4(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[4];
		g_bufferIndex = 4;
	};
};

//���ƴ���ΪLarge�İ�ť
class ButtonChangeWindowLarge : public Button {
public:
	ButtonChangeWindowLarge(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 1000;
		SCREEN_HEIGHT = 1000;
	};
};

//���ƴ���ΪMidium�İ�ť
class ButtonChangeWindowMidium : public Button {
public:
	ButtonChangeWindowMidium(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 600;
		SCREEN_HEIGHT = 600;
	};
};

//���ƴ���ΪSmall�İ�ť
class ButtonChangeWindowSmall : public Button {
public:
	ButtonChangeWindowSmall(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 500;
		SCREEN_HEIGHT = 500;
	};
};

//������������ʾ�İ�ť
class ButtonCoordinatesRender : public Button {
public:
	ButtonCoordinatesRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_coordinatesRenderFlag = true;
	};
	virtual void onClickedSecond() {
		g_coordinatesRenderFlag = false;
	};
};

//���ƹ�Դλ����ʾ�İ�ť
class ButtonLightPosRender : public Button {
public:
	ButtonLightPosRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_lightPosRenderFlag = true;
	};
	virtual void onClickedSecond() {
		g_lightPosRenderFlag = false;
	};
};

//���ư�ť�����İ�ť
class ButtonControl : public Button {
public:
	ButtonControl(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_buttonControlFlag = false;
	};
	virtual void onClickedSecond() {
		g_buttonControlFlag = true;
	};
};