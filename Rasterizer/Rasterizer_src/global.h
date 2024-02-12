#pragma once

#define PI float(2 * acos(0.0))
#define DegToRad PI / 180.0f

std::mutex mtx;

int SCREEN_WIDTH = 600;						//屏幕宽度
int SCREEN_HEIGHT = 600;					//屏幕高度
const int ZBUFFER_DEPTH = 255;				//深度缓冲区深度最大值

struct Color {
public:
	Color(int rr, int gg, int bb) : r(rr), g(gg), b(bb) {};
	int r;
	int g;
	int b;
};
std::vector<TGAColor> g_colorBuffer;		//颜色缓冲区
std::vector<TGAColor> g_smallColorBuffer;	//普通颜色缓冲区
std::vector<Color> g_colorOptBuffer;		//优化颜色缓冲区
std::vector<Color> g_colorOpt8xMSAABuffer;	//优化的8xMSAA颜色缓冲区
int* g_smallColorCount;						//优化颜色刷新次数
float* g_MSAAZBuffer = NULL;				//MSAA对应深度缓冲区，是正常深度缓冲区的4倍大小
float* g_8xMSAAZBuffer = NULL;				//8xMSAA对应深度缓冲区，是正常深度缓冲区的16倍大小
float* g_NoneMSAAZBuffer = NULL;			//深度缓冲区
float* g_ModelshadowMapBuffer0 = NULL;		//模型1的阴影缓冲区
float* g_ModelshadowMapBuffer1 = NULL;		//模型2的阴影颜色缓冲区
float* g_ModelshadowMapBuffer2 = NULL;		//模型3的阴影颜色缓冲区
float* g_ModelshadowMapBuffer3 = NULL;		//模型4的阴影颜色缓冲区
float* g_ModelshadowMapBuffer4 = NULL;		//模型5的阴影颜色缓冲区
int g_bufferIndex;							//阴影缓冲区索引

std::vector<Model*> g_modelList;			//模型列表
Model* g_model = NULL;						//当前渲染模型指针
TGAImage* g_textureTga = NULL;				//当前渲染模型贴图指针
std::vector<Button*> g_renderControlList;	//渲染模式控制按钮列表
std::vector<Button*> g_modelControlList;	//模型控制按钮列表
std::vector<Button*> g_windowControlList;	//窗口控制按钮列表
Button* btn10 = NULL;						//FPS文本显示按钮
Button* btn11 = NULL;						//FPS内容显示按钮
Button* btn18 = NULL;						//按钮显隐控制按钮
bool g_gotoFlag = false;					//跳转控制

Vec3f g_lightDir = Vec3f(-2.0f, -0.5f, -3.0f).normalize();	//平行光的方向
Vec3f g_lightDirn = Vec3f(2.0f, 0.5f, 3.0f).normalize();	//以表面为基点入射平行光的方向（反向）
Vec3f g_lightPos(4.0f, 1.0f, 6.0f);							//点光源位置
float g_illum = 39.0f;										//点光源在距离为1的球体的单位面积上强度

std::vector<Vec3f> g_coordinatesVector;		//坐标轴向量，用于GUI
std::vector<Vec3f> g_lightPosVector;		//点光源向量，用于GUI

Vec3f cameraEye(0.0f, 0.0f, 5.0f);			//摄像机位置
Vec3f cameraCenter(0.0f, 0.0f, 0.0f);		//摄像机视点，摄像机看的位置
Vec3f cameraUp(0.0f, 1.0f, 0.0f);			//摄像机正上方向
float ASPECT = 1.0f;						//视锥体宽高比
float FAR = 10.0f;							//视锥体远平面
float NEAR = 0.1f;							//视锥体近平面
float FOVY = 60.0f;							//视锥体角度

Matrix LModelView;							//以光源为摄像机，构造的ModelView矩阵
Matrix LProjectionMatrix;					//以光源为摄像机，构造的ProjectionMatrix矩阵
Matrix LViewPort;							//以光源为摄像机，构造的ViewPort矩阵
Matrix LMVP;								//以光源为摄像机，构造的MVP矩阵

SDL_Window* g_window = NULL;				//SDL窗口指针
SDL_Renderer* g_renderer = NULL;			//SDL窗口渲染指针
TTF_Font* g_font = NULL;					//TTF指针

bool g_frameRenderFlag = false;				//线框模式渲染控制标识
bool g_materialRenderFlag = false;			//材质预览模式渲染控制标识
bool g_textureRenderFlag = false;			//纹理预览模式渲染控制标识
bool g_flatRenderFlag = false;				//flat预览模式渲染控制标识
bool g_renderFlag = false;					//不包括抗锯齿和阴影的渲染控制标识
bool g_shadowNMRenderFlag = false;			//不包括抗锯齿的阴影渲染标识
bool g_MSAARenderFlag = false;				//不包括阴影的渲染控制标识
bool g_shadowRenderFlag = false;			//包括阴影的渲染控制标识
bool g_coordinatesRenderFlag = false;		//坐标轴渲染控制标识
bool g_lightPosRenderFlag = false;			//光源位置渲染控制标识
bool g_buttonControlFlag = true;			//按钮显隐控制标识

float g_perOpt = 0.0f;						//平均优化效率
int g_faceFragmentCount = 0;				//当前模型送入片段着色面片总数量

//以下为按钮的子类，可以实现不同按钮行为和样式

//控制线框渲染的按钮
class ButtonFrameRender : public Button {
public:
	ButtonFrameRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_frameRenderFlag = true;

		//将其他标志初始化
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

//控制flat渲染的按钮
class ButtonFlatRender : public Button {
public:
	ButtonFlatRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_flatRenderFlag = true;

		//将其他标志初始化
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

//控制材质预览渲染的按钮
class ButtonMaterialRender : public Button {
public:
	ButtonMaterialRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_materialRenderFlag = true;

		//将其他标志初始化
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

//控制纹理预览渲染的按钮
class ButtonTextureRender : public Button {
public:
	ButtonTextureRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_textureRenderFlag = true;

		//将其他标志初始化
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

//控制不包括抗锯齿和阴影渲染的按钮
class ButtonRender : public Button {
public:
	ButtonRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_renderFlag = true;

		//将其他标志初始化
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

//控制不包括抗锯齿的阴影渲染的按钮
class ButtonShadowNMRender : public Button {
public:
	ButtonShadowNMRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_shadowNMRenderFlag = true;

		//将其他标志初始化
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

//控制不包括阴影渲染的按钮
class ButtonMSAARender : public Button {
public:
	ButtonMSAARender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_MSAARenderFlag = true;

		//将其他标志初始化
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

//控制包括阴影渲染的按钮
class ButtonShadowRender : public Button {
public:
	ButtonShadowRender(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_MSAARenderFlag = true;
		g_shadowRenderFlag = true;

		//将其他标志初始化
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

//控制模型1渲染显示的按钮
class ButtonModel0 : public Button {
public:
	ButtonModel0(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[0];
		g_bufferIndex = 0;
	};
};

//控制模型2渲染显示的按钮
class ButtonModel1 : public Button {
public:
	ButtonModel1(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[1];
		g_bufferIndex = 1;
	};
};

//控制模型3渲染显示的按钮
class ButtonModel2 : public Button {
public:
	ButtonModel2(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[2];
		g_bufferIndex = 2;
	};
};

//控制模型4渲染显示的按钮
class ButtonModel3 : public Button {
public:
	ButtonModel3(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[3];
		g_bufferIndex = 3;
	};
};

//控制模型5渲染显示的按钮
class ButtonModel4 : public Button {
public:
	ButtonModel4(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		g_model = g_modelList[4];
		g_bufferIndex = 4;
	};
};

//控制窗口为Large的按钮
class ButtonChangeWindowLarge : public Button {
public:
	ButtonChangeWindowLarge(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 1000;
		SCREEN_HEIGHT = 1000;
	};
};

//控制窗口为Midium的按钮
class ButtonChangeWindowMidium : public Button {
public:
	ButtonChangeWindowMidium(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 600;
		SCREEN_HEIGHT = 600;
	};
};

//控制窗口为Small的按钮
class ButtonChangeWindowSmall : public Button {
public:
	ButtonChangeWindowSmall(int x, int y, int w, int h, std::string _title) :
		Button(x, y, w, h, _title) {};

	virtual void onClicked() {
		SCREEN_WIDTH = 500;
		SCREEN_HEIGHT = 500;
	};
};

//控制坐标轴显示的按钮
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

//控制光源位置显示的按钮
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

//控制按钮显隐的按钮
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