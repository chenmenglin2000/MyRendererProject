/************************************************************
 * Copyright:	Hinata
 * Author:		Hinata
 * Date:		2022-05-28
 * Description:	TinySoftRenderer
 * Comment:		1. Blinn-Phong光照模型（基本实现）（mtl库中的参数是否正确使用？）
				2. GUI，用户与摄像机的交互（基本实现）
				3. MSAA抗锯齿（基本实现）
				4. 贴图透视插值（基本实现）
				5. ShadowMap硬阴影（基本实现）（思考精度问题）（平行光需要使用正交投影计算ShadowMap）（需要保证物体在视锥体间，不然不会产生阴影，甚至出错）
				6. 剪裁（实现了简单的NDC空间剔除）
				7. 点光源（基本实现）（未考虑摄像机和物体距离的衰减）
				8. ...
 * Unfinished:	1. 优化
				2. oneTBB并行化
				3. 可以先判断深度值，再进行其他计算
				4. 待实现：屏幕空间环境光遮蔽算法（Screen space ambient occlusion，SSAO）
				5. 摄像机类的封装
				6. 键盘鼠标控制类的封装
 * Known Issue:	1. 背面剔除后轻微的不正常剔除（精度问题？）
				2. ShadowMap的acne问题
				3. float数值比较问题
				4. 并行化后，像素点闪烁问题
				5. 并行化后，面片边缘随机覆盖问题（已进行深度值相同求均尝试）
************************************************************/

#define _CRT_SECURE_NO_DEPRECATE

#include <tbb/parallel_for.h>
#include <mutex>
#include "button.h"
#include "model_obj.h"
#include "global.h"
#include "matrix_ex.h"
#include "timer.h"

//初始化程序开启SDL
bool init()
{
	bool success = true;//初始化标志

	//初始化SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//将纹理过滤设置为线性
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//创建窗口
		g_window = SDL_CreateWindow("Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (g_window == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//给窗口创建渲染
			g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
			if (g_renderer == NULL)
			{
				SDL_DestroyWindow(g_window);
				g_window = NULL;
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
		}
		//创建TTF
		if (TTF_Init()) {
			SDL_DestroyRenderer(g_renderer);
			SDL_DestroyWindow(g_window);
			g_window = NULL;
			g_renderer = NULL;
			success = false;
		}
		g_font = TTF_OpenFont("simhei.ttf", 64);
		if (g_font == NULL) {
			SDL_DestroyRenderer(g_renderer);
			SDL_DestroyWindow(g_window);
			g_window = NULL;
			g_renderer = NULL;
			success = false;
		}
	}

	return success;
}

//释放资源关闭SDL
void close()
{
	for (int i = 0; i < g_renderControlList.size(); i++) {
		delete g_renderControlList[i];
	}
	for (int i = 0; i < g_modelControlList.size(); i++) {
		delete g_modelControlList[i];
	}
	for (int i = 0; i < g_windowControlList.size(); i++) {
		delete g_windowControlList[i];
	}
	for (int i = 0; i < g_modelList.size(); i++) {
		delete g_modelList[i];
	}
	TTF_CloseFont(g_font);
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	g_window = NULL;
	g_renderer = NULL;
	g_font = NULL;

	//多次delete报错
	delete btn10;
	delete btn11;
	delete btn18;
	delete[] g_smallColorCount;
	delete[] g_MSAAZBuffer;
	delete[] g_NoneMSAAZBuffer;
	delete[] g_ModelshadowMapBuffer0;
	delete[] g_ModelshadowMapBuffer1;
	delete[] g_ModelshadowMapBuffer2;
	delete[] g_ModelshadowMapBuffer3;
	delete[] g_ModelshadowMapBuffer4;

	SDL_Quit();
}

//创建按钮
void createButton() {
	//纹理预览渲染模式控制按钮
	ButtonTextureRender* btn1 = new ButtonTextureRender(SCREEN_WIDTH / 30, 23 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "Material");
	g_renderControlList.push_back(btn1);
	//渲染模式控制按钮
	ButtonRender* btn2 = new ButtonRender(SCREEN_WIDTH / 30, 27 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "Rendered");
	g_renderControlList.push_back(btn2);
	//不包括阴影渲染模式控制按钮
	ButtonMSAARender* btn3 = new ButtonMSAARender(SCREEN_WIDTH / 30, 35 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, " 4xMSAA ");
	g_renderControlList.push_back(btn3);
	//包括阴影渲染模式控制按钮
	ButtonShadowRender* btn4 = new ButtonShadowRender(SCREEN_WIDTH / 30, 39 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "ShadowAM");
	g_renderControlList.push_back(btn4);
	//线框渲染模式控制按钮
	ButtonFrameRender* btn5 = new ButtonFrameRender(SCREEN_WIDTH / 30, 11 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "Wireframe");
	g_renderControlList.push_back(btn5);
	//材质预览渲染模式控制按钮
	ButtonMaterialRender* btn6 = new ButtonMaterialRender(SCREEN_WIDTH / 30, 19 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "  Solid  ");
	g_renderControlList.push_back(btn6);
	//flat预览渲染模式控制按钮
	ButtonFlatRender* btn66 = new ButtonFlatRender(SCREEN_WIDTH / 30, 15 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "  Flat  ");
	g_renderControlList.push_back(btn66);
	//无抗锯齿阴影渲染模式控制按钮
	ButtonShadowNMRender* btn666 = new ButtonShadowNMRender(SCREEN_WIDTH / 30, 31 * SCREEN_HEIGHT / 75, SCREEN_WIDTH / 9, SCREEN_HEIGHT / 22, "ShadowNM");
	g_renderControlList.push_back(btn666);

	//模型1控制按钮
	ButtonModel0* btn7 = new ButtonModel0(SCREEN_WIDTH / 30, 53 * SCREEN_HEIGHT / 90, SCREEN_WIDTH / 7, SCREEN_HEIGHT / 15, "Model1");
	g_modelControlList.push_back(btn7);
	//模型2控制按钮
	ButtonModel1* btn8 = new ButtonModel1(SCREEN_WIDTH / 30, 60 * SCREEN_HEIGHT / 90, SCREEN_WIDTH / 7, SCREEN_HEIGHT / 15, "Model2");
	g_modelControlList.push_back(btn8);
	//模型3控制按钮
	ButtonModel2* btn9 = new ButtonModel2(SCREEN_WIDTH / 30, 67 * SCREEN_HEIGHT / 90, SCREEN_WIDTH / 7, SCREEN_HEIGHT / 15, "Model3");
	g_modelControlList.push_back(btn9);
	//模型4控制按钮
	ButtonModel3* btn12 = new ButtonModel3(SCREEN_WIDTH / 30, 74 * SCREEN_HEIGHT / 90, SCREEN_WIDTH / 7, SCREEN_HEIGHT / 15, "Model4");
	g_modelControlList.push_back(btn12);
	//模型5控制按钮
	ButtonModel4* btn133 = new ButtonModel4(SCREEN_WIDTH / 30, 81 * SCREEN_HEIGHT / 90, SCREEN_WIDTH / 7, SCREEN_HEIGHT / 15, "Model5");
	g_modelControlList.push_back(btn133);

	//Small窗口控制按钮
	ButtonChangeWindowSmall* btn13 = new ButtonChangeWindowSmall(138 * SCREEN_WIDTH / 240, 11 * SCREEN_HEIGHT / 12, SCREEN_WIDTH / 8, SCREEN_HEIGHT / 20, "SamllWin");
	g_windowControlList.push_back(btn13);
	//Midium窗口控制按钮
	ButtonChangeWindowMidium* btn14 = new ButtonChangeWindowMidium(170 * SCREEN_WIDTH / 240, 11 * SCREEN_HEIGHT / 12, SCREEN_WIDTH / 8, SCREEN_HEIGHT / 20, "MediumWin");
	g_windowControlList.push_back(btn14);
	//Large窗口控制按钮
	ButtonChangeWindowLarge* btn15 = new ButtonChangeWindowLarge(202 * SCREEN_WIDTH / 240, 11 * SCREEN_HEIGHT / 12, SCREEN_WIDTH / 8, SCREEN_HEIGHT / 20, "LargeWin");
	g_windowControlList.push_back(btn15);

	//坐标轴渲染控制按钮
	ButtonCoordinatesRender* btn16 = new ButtonCoordinatesRender(30 * SCREEN_WIDTH / 40, SCREEN_HEIGHT / 30, SCREEN_WIDTH / 16, SCREEN_HEIGHT / 20, "CSYS");
	g_renderControlList.push_back(btn16);
	//点光源位置渲染控制按钮
	ButtonLightPosRender* btn17 = new ButtonLightPosRender(33 * SCREEN_WIDTH / 40, SCREEN_HEIGHT / 30, SCREEN_WIDTH / 16, SCREEN_HEIGHT / 20, "LSYS");
	g_renderControlList.push_back(btn17);

	//FPS文本显示按钮
	btn10 = new Button(SCREEN_WIDTH / 30, SCREEN_HEIGHT / 16, SCREEN_WIDTH / 6, SCREEN_HEIGHT / 15, "FPScontent");
	//FPS内容显示按钮
	btn11 = new Button(SCREEN_WIDTH / 30, SCREEN_HEIGHT / 27, SCREEN_WIDTH / 6, SCREEN_HEIGHT / 30, "FPS");
	//按钮显隐控制按钮
	btn18 = new ButtonControl(36 * SCREEN_WIDTH / 40, SCREEN_HEIGHT / 30, SCREEN_WIDTH / 16, SCREEN_HEIGHT / 20, "BHID");
}

//绘制所有按钮
void drawAllButton(std::string stringFPS) {
	if (g_buttonControlFlag) {
		for (int i = 0; i < g_renderControlList.size(); i++) {
			g_renderControlList[i]->drawButton(g_font, g_renderer);
		}
		for (int i = 0; i < g_modelControlList.size(); i++) {
			g_modelControlList[i]->drawButton(g_font, g_renderer);
		}
		for (int i = 0; i < g_windowControlList.size(); i++) {
			g_windowControlList[i]->drawButton(g_font, g_renderer);
		}
	}
	btn10->drawTextFps(g_font, g_renderer, stringFPS);
	btn11->drawTextNormal(g_font, g_renderer);
	btn18->drawButton(g_font, g_renderer);
}

//设置坐标轴、点光源显示的向量
void setGUIvector() {
	//定义坐标轴向量
	Vec3f x = { 2.0f, 0.0f, 0.0f };
	Vec3f y = { 0.0f, 2.0f, 0.0f };
	Vec3f z = { 0.0f, 0.0f, 2.0f };
	Vec3f o = { 0.0f, 0.0f, 0.0f };
	g_coordinatesVector.push_back(x);
	g_coordinatesVector.push_back(y);
	g_coordinatesVector.push_back(z);
	g_coordinatesVector.push_back(o);
	//定义点光源坐标轴向量
	Vec3f lx = Vec3f{ 0.3f, 0.0f, 0.0f } + g_lightPos;
	Vec3f ly = Vec3f{ 0.0f, 0.3f, 0.0f } + g_lightPos;
	Vec3f lz = Vec3f{ 0.0f, 0.0f, 0.3f } + g_lightPos;
	Vec3f lxf = lx;
	lxf.x -= 0.6f;
	Vec3f lyf = ly;
	lyf.y -= 0.6f;
	Vec3f lzf = lz;
	lzf.z -= 0.6f;
	g_lightPosVector.push_back(lx);
	g_lightPosVector.push_back(ly);
	g_lightPosVector.push_back(lz);
	g_lightPosVector.push_back(lxf);
	g_lightPosVector.push_back(lyf);
	g_lightPosVector.push_back(lzf);
	g_lightPosVector.push_back(g_lightPos);
}

//加载模型
void loadModel() {
	Model* m0 = new Model("obj/african_head.obj", "obj/african_head.mtl");
	g_modelList.push_back(m0);
	Model* m1 = new Model("obj/cube.obj", "obj/cube.mtl");
	g_modelList.push_back(m1);
	Model* m2 = new Model("obj/xiao.obj", "obj/xiao.mtl");
	g_modelList.push_back(m2);
	Model* m3 = new Model("obj/cat.obj", "obj/cat.mtl");
	g_modelList.push_back(m3);
	Model* m4 = new Model("obj/floor.obj", "obj/floor.mtl");
	g_modelList.push_back(m4);
}

//初始化buffer
void initializeBuffer() {
	g_MSAAZBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT * 4];
	//一维和二维影响很小？
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 4; i++) {
		g_colorBuffer.push_back(TGAColor(0, 0, 0, 0));
		g_MSAAZBuffer[i] = -9999999.0f;
	}
	g_NoneMSAAZBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_smallColorCount = new int[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_ModelshadowMapBuffer0 = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_ModelshadowMapBuffer1 = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_ModelshadowMapBuffer2 = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_ModelshadowMapBuffer3 = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_ModelshadowMapBuffer4 = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
	g_8xMSAAZBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT * 16];
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 16; ++i) {
		g_colorOpt8xMSAABuffer.push_back(Color(0, 0, 0));
		g_8xMSAAZBuffer[i] = 9999999.0f;
	}
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		g_ModelshadowMapBuffer0[i] = 9999999.0f;
		g_ModelshadowMapBuffer1[i] = 9999999.0f;
		g_ModelshadowMapBuffer2[i] = 9999999.0f;
		g_ModelshadowMapBuffer3[i] = 9999999.0f;
		g_ModelshadowMapBuffer4[i] = 9999999.0f;
		g_smallColorCount[i] = 1;
		g_smallColorBuffer.push_back(TGAColor(0, 0, 0, 0));
		g_colorOptBuffer.push_back(Color(0, 0, 0));
	}
}

//初始化运行过程中的buffer
void initializeRunBuffer() {
	if (g_MSAARenderFlag || g_shadowRenderFlag) {
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 4; i++) {
			g_colorBuffer[i] = TGAColor(0, 0, 0, 0);
			g_MSAAZBuffer[i] = -9999999.0f;	// 未初始化，默认值0
		}
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 9; ++i) {
			g_8xMSAAZBuffer[i] = -9999999.0f;
			g_colorOpt8xMSAABuffer[i] = Color(0, 0, 0);
		}
	}
	else {
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
			g_NoneMSAAZBuffer[i] = -9999999.0f;
			//g_smallColorBuffer[i] = TGAColor(0, 0, 0, 0);//已被g_colorOptBuffer替代
			g_smallColorCount[i] = 1;
			g_colorOptBuffer[i] = Color(0, 0, 0);
		}
	}
}

//设置程序运行中默认参数
void setDefaultParameter() {
	//默认模型
	g_model = g_modelList[0];

	//默认按钮激活
	g_modelControlList[0]->m_buttonStatus = true;
	if (SCREEN_WIDTH == 1000)
		g_windowControlList[2]->m_buttonStatus = true;
	else if (SCREEN_WIDTH == 600)
		g_windowControlList[1]->m_buttonStatus = true;
	else if (SCREEN_WIDTH == 500)
		g_windowControlList[0]->m_buttonStatus = true;

	//默认模型shadowMapVertexShader索引
	g_bufferIndex = 0;
}

//将float转换为string并保留两位小数
std::string floatToString(float f) {
	char* chCode;
	chCode = new(std::nothrow)char[20];
	sprintf(chCode, "%.2lf", f);
	std::string strCode(chCode);
	delete[] chCode;
	return strCode;
}

//检测鼠标是否在按钮上方
void detectMouseOnButton(SDL_Event& event) {
	//按钮如果都被隐藏，则不检测
	if (g_buttonControlFlag) {
		//判断鼠标是否在按钮内，如果在，则执行相应操作
		for (int i = 0; i < g_renderControlList.size(); i++)
		{
			SDL_Rect rect = g_renderControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					g_renderControlList[i]->m_buttonMouseOn = true;
					//一般只会在一个按钮内
					//不能break，可能后面还有需要初始化的按钮
					continue;
				}
			}
			//初始化之前按钮的状态
			g_renderControlList[i]->m_buttonMouseOn = false;
		}
		for (int i = 0; i < g_modelControlList.size(); i++)
		{
			SDL_Rect rect = g_modelControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					g_modelControlList[i]->m_buttonMouseOn = true;
					continue;
				}
			}
			g_modelControlList[i]->m_buttonMouseOn = false;
		}
		for (int i = 0; i < g_windowControlList.size(); i++)
		{
			SDL_Rect rect = g_windowControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					g_windowControlList[i]->m_buttonMouseOn = true;
					continue;
				}
			}
			g_windowControlList[i]->m_buttonMouseOn = false;
		}
	}

	//单独检测控制按钮显隐的按钮
	SDL_Rect rect = btn18->getRect();
	btn18->m_buttonMouseOn = false;
	if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
	{
		if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
		{
			btn18->m_buttonMouseOn = true;
		}
	}
}

//鼠标左键点击后的操作
void mouseLeftUpControl(SDL_Event& event) {
	//鼠标左键如果按起，且在某个按钮上，执行点击后的操作
	if (g_buttonControlFlag) {
		for (int i = 0; i < g_renderControlList.size(); i++)
		{
			SDL_Rect rect = g_renderControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					//坐标轴与光源位置控制按钮不影响其他按钮
					if (i != 8 && i != 9) {
						//这种情况下点击了，又在按钮上，其余按钮肯定变化，索性提前将其余的全部初始化
						for (int j = 0; j < g_renderControlList.size(); j++) {
							if (i != j && j != 8 && j != 9) g_renderControlList[j]->m_buttonStatus = false;
						}
					}

					//点击后的操作
					//如果是未激活状态，则变为激活状态
					if (g_renderControlList[i]->m_buttonStatus == false) {
						g_renderControlList[i]->m_buttonStatus = true;
						g_renderControlList[i]->onClicked();
						continue;
					}
					else {
						//如果是激活状态，则变为未激活状态
						g_renderControlList[i]->m_buttonStatus = false;
						g_renderControlList[i]->onClickedSecond();
					}
				}
			}
		}
		for (int i = 0; i < g_modelControlList.size(); i++)
		{
			SDL_Rect rect = g_modelControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					for (int j = 0; j < g_modelControlList.size(); j++) {
						if (i != j) g_modelControlList[j]->m_buttonStatus = false;
					}

					if (g_modelControlList[i]->m_buttonStatus == false) {
						g_modelControlList[i]->m_buttonStatus = true;
						g_modelControlList[i]->onClicked();
						continue;
					}
				}
			}
		}
		for (int i = 0; i < g_windowControlList.size(); i++)
		{
			SDL_Rect rect = g_windowControlList[i]->getRect();
			if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
			{
				if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
				{
					for (int j = 0; j < g_windowControlList.size(); j++) {
						if (i != j) g_windowControlList[j]->m_buttonStatus = false;
					}

					if (g_windowControlList[i]->m_buttonStatus == false) {
						g_windowControlList[i]->m_buttonStatus = true;
						g_windowControlList[i]->onClicked();
						//点击改变窗口大小按钮
						//需要先销毁、初始化一些东西
						g_MSAAZBuffer = NULL;
						g_NoneMSAAZBuffer = NULL;
						g_smallColorCount = NULL;
						g_ModelshadowMapBuffer0 = NULL;
						g_ModelshadowMapBuffer1 = NULL;
						g_ModelshadowMapBuffer2 = NULL;
						g_ModelshadowMapBuffer3 = NULL;
						g_ModelshadowMapBuffer4 = NULL;
						g_bufferIndex = 0;
						g_model = NULL;
						g_textureTga = NULL;
						g_window = NULL;
						g_renderer = NULL;
						g_font = NULL;
						g_frameRenderFlag = false;
						g_materialRenderFlag = false;
						g_textureRenderFlag = false;
						g_renderFlag = false;
						g_MSAARenderFlag = false;
						g_shadowRenderFlag = false;
						g_coordinatesRenderFlag = false;
						g_lightPosRenderFlag = false;
						g_buttonControlFlag = true;
						btn10 = NULL;
						btn11 = NULL;
						btn18 = NULL;
						g_perOpt = 0.0f;
						g_faceFragmentCount = 0;
						for (int i = 0; i < g_renderControlList.size(); i++) {
							g_renderControlList[i];
						}
						g_renderControlList.clear();
						for (int i = 0; i < g_modelControlList.size(); i++) {
							delete g_modelControlList[i];
						}
						g_modelControlList.clear();
						for (int i = 0; i < g_windowControlList.size(); i++) {
							delete g_windowControlList[i];
						}
						g_windowControlList.clear();
						for (int i = 0; i < g_modelList.size(); i++) {
							delete g_modelList[i];
						}
						g_modelList.clear();
						g_window = NULL;
						g_renderer = NULL;
						g_font = NULL;

						//跳转标志
						g_gotoFlag = true;
						SDL_Quit();
						return;
					}
				}
			}
		}
	}
	SDL_Rect rect = btn18->getRect();
	if (event.button.x > rect.x && event.button.x < rect.x + rect.w)
	{
		if (event.button.y > rect.y && event.button.y < rect.y + rect.h)
		{
			//点击后的操作
			//如果是未激活状态，则变为激活状态
			if (btn18->m_buttonStatus == false) {
				btn18->m_buttonStatus = true;
				btn18->onClicked();
			}
			else {
				//如果是激活状态，则变为未激活状态
				btn18->m_buttonStatus = false;
				btn18->onClickedSecond();
			}
		}
	}
}

//用SDL绘制像素点
void SDLDrawPixel(int x, int y)
{
	//SDL绘图，左上角为原点，右侧为正z轴，下侧为正y轴
	//将原点转换为左下角绘制
	SDL_RenderDrawPoint(g_renderer, x, SCREEN_HEIGHT - 1 - y);
}

//Bresenham画线
void drawLineBresenham(int x1, int y1, int x2, int y2) {
	//斜率绝对值大于1的直线，对称变换为小于1
	bool steep = false;
	if (std::abs(x1 - x2) < std::abs(y1 - y2)) {
		std::swap(x1, y1);
		std::swap(x2, y2);
		steep = true;
	}
	//交换起点终点坐标
	if (x1 > x2) {
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	int y = y1;
	int eps = 0;
	int dx = x2 - x1;
	int dy = y2 - y1;
	int yi = 1;

	//处理[-1, 0]范围内的斜率
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}

	for (int x = x1; x <= x2; x++) {
		if (steep) {
			SDLDrawPixel(y, x);
		}
		else {
			SDLDrawPixel(x, y);
		}

		eps += dy;
		//这里用位运算<< 1代替* 2
		if ((eps << 1) >= dx) {
			y = y + yi;
			eps -= dx;
		}
	}
}

//Bresenham画圆
void drawCircleBresenham(int x, int y, int r)
{
	int tx = 0, ty = r, d = 3 - 2 * r;

	while (tx <= ty)
	{
		//利用圆的八分对称性画点
		SDLDrawPixel(x + tx, y + ty);
		SDLDrawPixel(x + tx, y - ty);
		SDLDrawPixel(x - tx, y + ty);
		SDLDrawPixel(x - tx, y - ty);
		SDLDrawPixel(x + ty, y + tx);
		SDLDrawPixel(x + ty, y - tx);
		SDLDrawPixel(x - ty, y + tx);
		SDLDrawPixel(x - ty, y - tx);

		if (d < 0)	
			// 取上面的点
			d += 4 * tx + 6;
		else			
			// 取下面的点
			d += 4 * (tx - ty) + 10, ty--;

		tx++;
	}
}

//绘制剪裁框
void printClipWin() {
	//MSAA后可以提前将值写入着色缓冲区
	SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
	drawLineBresenham(100, 100, 700, 100);
	drawLineBresenham(100, 100, 100, 700);
	drawLineBresenham(700, 100, 700, 700);
	drawLineBresenham(100, 700, 700, 700);
}

//求重心坐标
Vec3f baryCentric(Vec3f* pts, Vec3f P) {
	Vec3f x(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x);
	Vec3f y(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);

	Vec3f u = x ^ y;

	// 由于 A, B, C, P 的坐标都是 int 类型，所以 u.z 必定是 int 类型，取值范围为 ..., -2, -1, 0, 1, 2, ...
	// 所以 u.z 绝对值小于 1 意味着三角形退化了，需要舍弃
	if (std::abs(u.z) < 1) {
		//传入一个不满足检测的即可舍弃
		return Vec3f(-1, 1, 1);
	}
	return Vec3f(1.0f - (u.x + u.y) / (float)u.z, u.x / (float)u.z, u.y / (float)u.z);
}

//将UV[0, 1]坐标转换为图片里的坐标
std::pair<int, int> computeUV(float u, float v, TGAImage* g_textureTga) {
	return std::make_pair(u * g_textureTga->get_width(), v * g_textureTga->get_height());
}

//将MSAA后得到的颜色缓冲区绘制
void drawMSAAColorBuffer() {
	for (int y = (SCREEN_HEIGHT * 1 / 5); y < (SCREEN_HEIGHT * 4 / 5); y++) {
		for (int x = (SCREEN_WIDTH * 1 / 5); x < (SCREEN_WIDTH * 4 / 5); x++) {
			//左上
			int r = (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x][0];
			int g = (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x][1];
			int b = (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x][2];
			int a = (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x][3];
			//右上
			r += (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][0];
			g += (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][1];
			b += (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][2];
			a += (int)g_colorBuffer[2 * SCREEN_WIDTH * 2 * y + 2 * x + 1][3];
			//左下
			r += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][0];
			g += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][1];
			b += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][2];
			a += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x][3];
			//右下
			r += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][0];
			g += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][1];
			b += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][2];
			a += (int)g_colorBuffer[2 * SCREEN_WIDTH * (2 * y + 1) + 2 * x + 1][3];

			//求均
			r /= 4;
			g /= 4;
			b /= 4;
			a /= 4;

			SDL_SetRenderDrawColor(g_renderer, b, g, r, a);
			SDLDrawPixel(x, y);
		}
	}
}

//计算阴影用片段着色器
void shadowMapFragmentShader(Vec3f pts[3], Vec3f seebc_coords[3]) {
	Vec2f boxmin(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	Vec2f boxmax(0, 0);
	Vec2f clamp(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			boxmin.x = std::max(0.0f, std::min(boxmin.x, pts[i].x));
			boxmin.y = std::max(0.0f, std::min(boxmin.y, pts[i].y));
			boxmax.x = std::min(clamp.x, std::max(boxmax.x, pts[i].x));
			boxmax.y = std::min(clamp.y, std::max(boxmax.y, pts[i].y));
		}
	}
	Vec3f P;
	for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
		for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
			Vec3f bc_screenn = baryCentric(pts, P);
			if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
				continue;
			}
			P.z = 0;
			for (int i = 0; i < 3; i++) { 
				P.z += pts[i][2] * bc_screenn[i];
			}
			if (g_bufferIndex == 0) {
				if (g_ModelshadowMapBuffer0[int(P.x + P.y * SCREEN_WIDTH)] > P.z) {
					g_ModelshadowMapBuffer0[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
				}
			}
			else if (g_bufferIndex == 1) {
				if (g_ModelshadowMapBuffer1[int(P.x + P.y * SCREEN_WIDTH)] > P.z) {
					g_ModelshadowMapBuffer1[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
				}
			}
			else if (g_bufferIndex == 2) {
				if (g_ModelshadowMapBuffer2[int(P.x + P.y * SCREEN_WIDTH)] > P.z) {
					g_ModelshadowMapBuffer2[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
				}
			}
			else if (g_bufferIndex == 3) {
				if (g_ModelshadowMapBuffer3[int(P.x + P.y * SCREEN_WIDTH)] > P.z) {
					g_ModelshadowMapBuffer3[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
				}
			}
			else if (g_bufferIndex == 4) {
				if (g_ModelshadowMapBuffer4[int(P.x + P.y * SCREEN_WIDTH)] > P.z) {
					g_ModelshadowMapBuffer4[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
				}
			}
		}
	}
}

//计算阴影用顶点着色器
void shadowMapVertexShader() {
	for (int g = 0; g < g_model->getNumOfGroup(); g++) {
		for (int i = g_model->getNumOfGroupFace(g) - 1; i >= 0; i--) {
			std::vector<int> face = g_model->getGroupFace(g, i);
			Vec3f worldCoords[3];
			Vec3f cameraCoords[3];
			Vec3f perspectiveCoords[3];
			Vec3f screenCoords[3];
			float wComponent = 1.0f;
			bool flag = false;
			bool flagw = true;
			for (int j = 0; j < 3; j++) {
				worldCoords[j] = g_model->getVert(face[j * 3]);
				cameraCoords[j] = m2vn(LModelView * v2m(worldCoords[j]));
				if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
					flagw = false;
					break;
				}
				wComponent = -cameraCoords[j].z;
				perspectiveCoords[j] = m2vn(LProjectionMatrix * v2m(cameraCoords[j]));
				perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
				perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
				perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
			}
			if (flagw) {
				for (int j = 0; j < 3; j++) {
					if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
						if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
							flag = true;
							break;
						}
					}
				}
				if (flag) {
					Vec3f c(0.0f, 0.0f, 1.0f);
					Vec3f n = (perspectiveCoords[1] - perspectiveCoords[0]) ^ (perspectiveCoords[2] - perspectiveCoords[1]);
					float intensity = n * c;
					if (intensity > 0.00f) continue;//正面剔除用>
					for (int j = 0; j < 3; j++) {
						screenCoords[j] = m2vn(LViewPort * v2m(perspectiveCoords[j]));
						screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
					}
					shadowMapFragmentShader(screenCoords, cameraCoords);
				}
			}
		}
	}
}

//创建ShadowMap
void createShadowMap() {
	//以光源为摄像机，构造的MVP矩阵
	LModelView = lookat(g_lightPos, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
	LProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);
	LViewPort = viewport(1.0f / 2.0f, 1.0f / 2.0f);
	LMVP = LViewPort * LProjectionMatrix * LModelView;

	//只用运行一次，光源不动的情况下
	g_bufferIndex = 0;
	g_model = g_modelList[0];
	shadowMapVertexShader();
	g_bufferIndex = 1;
	g_model = g_modelList[1];
	shadowMapVertexShader();
	g_bufferIndex = 2;
	g_model = g_modelList[2];
	shadowMapVertexShader();
	g_bufferIndex = 3;
	g_model = g_modelList[3];
	shadowMapVertexShader();
	g_bufferIndex = 4;
	g_model = g_modelList[4];
	shadowMapVertexShader();
}

/**
 * Brief 片段着色器
 * Param screenCoords 3个顶点屏幕坐标
 * Param face 面片数据
 * Param diffuseIntensity 漫反射强度
 * Param specularIntensity 镜面反射强度
 * Param KaKdKsNs 光照参数 
 * Param cameraCoords 3个顶点观察坐标
 * Param uniformMShadow 阴影映射矩阵
 * Return
 */
void fragmentShader(Vec3f screenCoords[3], std::vector<int>& face, float diffuseIntensity[3], float specularIntensity[3], std::vector<float>& KaKdKsNs, Vec3f cameraCoords[3], Matrix& uniformMShadow) {
	//GouraudShading顶点光照插值，非法线插值

	//查找三角形包围盒边界，有更优解？
	Vec2f boxmin(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	Vec2f boxmax(0.0f, 0.0f);
	Vec2f clamp(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	for (int i = 0; i < 3; i++) {
		//第一层循环，遍历三角形的三个顶点
		for (int j = 0; j < 2; j++) {
			//第二层循环，根据顶点数值缩小包围盒的范围
			boxmin.x = std::max(0.0f, std::min(boxmin.x, screenCoords[i].x));
			boxmin.y = std::max(0.0f, std::min(boxmin.y, screenCoords[i].y));

			boxmax.x = std::min(clamp.x, std::max(boxmax.x, screenCoords[i].x));
			boxmax.y = std::min(clamp.y, std::max(boxmax.y, screenCoords[i].y));
		}
	}

	Vec3f P;//包围盒内的每一像素
	//包围盒内的每一像素根据重心坐标判断是否渲染
	for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
		for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
			//定义每个子采样点
			Vec3f msaapoint[4];//子采样点
			msaapoint[0].x = P.x - 0.25;//左下
			msaapoint[0].y = P.y - 0.25;
			msaapoint[1].x = P.x - 0.25;//左上
			msaapoint[1].y = P.y + 0.25;
			msaapoint[2].x = P.x + 0.25;//右下
			msaapoint[2].y = P.y - 0.25;
			msaapoint[3].x = P.x + 0.25;//右上
			msaapoint[3].y = P.y + 0.25;
			msaapoint[0].z = 0.0f;
			msaapoint[1].z = 0.0f;
			msaapoint[2].z = 0.0f;
			msaapoint[3].z = 0.0f;

			int count = 0;			//ShadowMap卷积数
			float shadow = 1.0f;	//阴影值，默认为1
			float x, y;				//子采样点对应像素坐标，映射颜色缓冲
			bool colorFlag = false;	//如果像素已经着色则为true
			TGAColor colorAfteruv;	//着色完成之后的值

			//对每一个子采样点循环
			for (int i = 0; i < 4; i++) {
				Vec3f bc_screenn = baryCentric(screenCoords, msaapoint[i]);
				//若重心坐标某分量小于0则表示此点在三角形外（在边上和顶点上认为是三角形内部）
				if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
					continue;
				}

				//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
				msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);

				//计算对应着色缓冲区坐标
				if (i == 0) {
					//左下
					x = P.x * 2.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 1) {
					//左上
					x = P.x * 2.0f;
					y = P.y * 2.0f;
				}
				else if (i == 2) {
					//右下
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 3) {
					//右上
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f;
				}

				//需要先上锁，再判断深度缓冲
				//如果判断深度缓冲后上锁，会出现深度值已经被刷新，反而继续着色
				std::lock_guard<std::mutex> lck(mtx);

				//z值不用在视口变换中变换？
				//这里存储的是观察坐标下的z值，越大越近
				//如果是NDC空间的z值，因为是左手坐标系，所以是越小越近
				if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] <= msaapoint[i].z) {
					//未着色则计算着色值
					//可能有采样点在三角形内，像素却在三角形外的情况，这时要用采样点的着色值，比较麻烦
					//给所有子采样点同一个着色值，可以取其父像素中心的着色值，比较麻烦
					//这里用第一个在面片内部的子采样点的着色值
					if (!colorFlag) {
						//计算shadow
						if (g_shadowRenderFlag) {
							msaapoint[i].z = 0;
							//依旧需要重心坐标插值z值？
							for (int k = 0; k < 3; k++) {
								msaapoint[i].z += screenCoords[k][2] * bc_screenn[k];
							}

							//为什么这里不用考虑透视除法的问题，为什么可以直接结果用w值归一化，屏幕坐标向上取整了的，为什么算出来的误差挺小
							//当前采样点屏幕坐标转换到以光源为摄像机点的坐标系下的屏幕坐标
							//Vec3f sbP = m2v(uniformMShadow * v2m(msaapoint[i]));

							//上面的矩阵运算太慢了。
							Vec3f sbP = { 0.0f, 0.0f, 0.0f };
							std::vector<float> v4 = { msaapoint[i].x, msaapoint[i].y, msaapoint[i].z, 1.0f };
							float four = 0.0f;
							for (int k = 0; k < 4; k++) {
								sbP.x += uniformMShadow.m[0][k] * v4[k];
								sbP.y += uniformMShadow.m[1][k] * v4[k];
								sbP.z += uniformMShadow.m[2][k] * v4[k];
								four += uniformMShadow.m[3][k] * v4[k];
							}
							sbP.x /= four;
							sbP.y /= four;
							sbP.z /= four;

							//简单的解决zfighting，使用一个bias偏移量
							float bias = 0.00015f;
							//可以动态调节bias，但是需要每个点的法向
							//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

							//zbuffer的值是当z值越大，离摄像机越近，观察坐标系下的z值
							//shadowMapVertexShader和sbP的z值是屏幕坐标系下的值，当其越小，离摄像机越近，左手坐标系
							sbP.z -= bias;

							//shadowMapVertexShader求均，求得shadowmap9个点中有阴影的数量，根据这个获得shadow值
							//如果求均后，似乎不用bias也能得到不错的效果？
							for (int x = -1; x <= 1; ++x)
							{
								for (int y = -1; y <= 1; ++y)
								{
									if (g_bufferIndex == 0) {
										if (g_ModelshadowMapBuffer0[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 1) {
										if (g_ModelshadowMapBuffer1[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 2) {
										if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 3) {
										if (g_ModelshadowMapBuffer3[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else {
										if (g_ModelshadowMapBuffer4[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
								}
							}
							shadow = 1.0f - .7 * count / 9.0f;

							//不优化的阴影
							/*if (g_bufferIndex == 0) {
								if (g_ModelshadowMapBuffer0[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH] < sbP.z)
									shadow = 0.3f;
							}
							else if (g_bufferIndex == 1) {
								if (g_ModelshadowMapBuffer1[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH] < sbP.z)
									shadow = 0.3f;
							}
							else if (g_bufferIndex == 2) {
								if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH] < sbP.z)
									shadow = 0.3f;
							}
							else if (g_bufferIndex == 3) {
								if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH] < sbP.z)
									shadow = 0.3f;
							}
							else {
								if (g_ModelshadowMapBuffer4[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH] < sbP.z)
									shadow = 0.3f;
							}*/
						}

						float u = 0.0f, v = 0.0f;				//uv坐标
						TGAColor coloruv;						//uv颜色值
						float intensityAfterDiffuse = 0.0f;		//插值得到的漫反射强度
						float intensityAfterSpecular = 0.0f;	//插值得到的镜面反射强度

						//重心坐标，计算当前像素的zbuffer，顶点uv坐标插值
						for (int i = 0; i < 3; i++) {
							//未透视插值的UV
							//u += g_model->uv(face[i * 3 + 1]).x * bc_screenn[i];
							//v += g_model->uv(face[i * 3 + 1]).y * bc_screenn[i];

							//透视插值的UV
							u += g_model->getUV(face[i * 3 + 1]).x * bc_screenn[i] / cameraCoords[i].z;
							v += g_model->getUV(face[i * 3 + 1]).y * bc_screenn[i] / cameraCoords[i].z;
							intensityAfterDiffuse += diffuseIntensity[i] * bc_screenn[i];
							intensityAfterSpecular += specularIntensity[i] * bc_screenn[i];
						}

						//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
						msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);
						u *= msaapoint[i].z;
						v *= msaapoint[i].z;

						//此时的uv坐标要变换到真实图片坐标
						std::pair<int, int> uvxy = computeUV(u, v, g_textureTga);
						coloruv = g_textureTga->get(uvxy.first, uvxy.second);

						//阴影值不会影响环境光量
						//for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (0.2 + 1.0 * intensityAfterDiffuse + 0.1 * intensityAfterSpecular), 255);
						for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (KaKdKsNs[0] + shadow * (KaKdKsNs[1] * intensityAfterDiffuse + KaKdKsNs[2] * intensityAfterSpecular)), 255);

						colorFlag = true;
					}

					//如果深度值相同，则求均
					int r = colorAfteruv[0];
					int g = colorAfteruv[1];
					int b = colorAfteruv[2];
					if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] == msaapoint[i].z) {
						r += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0];
						g += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1];
						b += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2];
						r /= 2;
						g /= 2;
						b /= 2;
					}

					g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] = msaapoint[i].z;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0] = r;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1] = g;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2] = b;
				}
			}
		}
	}
}

//改进后的片段着色器
void fragmentShaderOptimize(Vec3f screenCoords[3], std::vector<int>& face, float diffuseIntensity[3], float specularIntensity[3], std::vector<float>& KaKdKsNs, Vec3f cameraCoords[3], Matrix& uniformMShadow) {
	Vec2f boxmin(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	Vec2f boxmax(0.0f, 0.0f);
	Vec2f clamp(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			boxmin.x = std::max(0.0f, std::min(boxmin.x, screenCoords[i].x));
			boxmin.y = std::max(0.0f, std::min(boxmin.y, screenCoords[i].y));

			boxmax.x = std::min(clamp.x, std::max(boxmax.x, screenCoords[i].x));
			boxmax.y = std::min(clamp.y, std::max(boxmax.y, screenCoords[i].y));
		}
	}

	Vec3f screenCoordsOpt[3];//扩大一个像素后的三角形屏幕坐标

	//事实上不需要用扩大的包围盒，也不能简单扩大一个像素
	//Vec2f boxminOpt(boxmin.x - 1.0f, boxmin.y - 1.0f);//扩大一个像素后的包围盒
	//Vec2f boxmaxOpt(boxmax.x + 1.0f, boxmax.y + 1.0f);//扩大一个像素后的包围盒

	//顶点最值扩大
	/*for (int i = 0; i < 3; i++) {
		//如果三角形某点x等于boxmin.x，说明其最小，要减一个像素位置
		screenCoordsOpt[i].x = screenCoords[i].x == boxmin.x ? screenCoords[i].x - 1.0f : screenCoords[i].x;
		//如果三角形某点x等于boxmax.x，说明其最大，要加一个像素位置
		screenCoordsOpt[i].x = screenCoords[i].x == boxmax.x ? screenCoords[i].x + 1.0f : screenCoordsOpt[i].x;
		//如果三角形某点y等于boxmin.y，说明其最小，要减一个像素位置
		screenCoordsOpt[i].y = screenCoords[i].y == boxmin.y ? screenCoords[i].y - 1.0f : screenCoords[i].y;
		//如果三角形某点y等于boxmax.y，说明其最大，要加一个像素位置
		screenCoordsOpt[i].y = screenCoords[i].y == boxmax.y ? screenCoords[i].y + 1.0f : screenCoordsOpt[i].y;

		screenCoordsOpt[i].z = screenCoords[i].z;

		//std::cout << screenCoords[i].x << "*" << screenCoords[i].y << '\n';
		//std::cout << screenCoordsOpt[i].x << "**" << screenCoordsOpt[i].y << '\n';
	}*/

	//矩阵缩放扩大
	/*//获取三角形内随意一点
	Vec2f point;
	point.x = screenCoords[1].x - 2 * screenCoords[0].x + screenCoords[2].x;
	point.y = screenCoords[1].y - 2 * screenCoords[0].y + screenCoords[2].y;
	point.x = point.x / 3.0f + screenCoords[0].x;
	point.y = point.y / 3.0f + screenCoords[0].y;
	for (int i = 0; i < 3; i++) {
		screenCoordsOpt[i] = screenCoords[i];
		//将三角形平移到原点在三角形内部
		screenCoordsOpt[i].x -= point.x;
		screenCoordsOpt[i].y -= point.y;
		//将三角形放大
		screenCoordsOpt[i].x *= 1.3f;
		screenCoordsOpt[i].y *= 1.3f;
		//将三角形平移到原位
		screenCoordsOpt[i].x += point.x;
		screenCoordsOpt[i].y += point.y;
	}*/

	//向量缩放
	for (int i = 0; i < 3; i++) {
		Vec2f fangxiang1;
		Vec2f fangxiang2;
		float sin = 0.0f;
		fangxiang1.x = screenCoords[i].x - screenCoords[(i + 1) % 3].x;
		fangxiang1.y = screenCoords[i].y - screenCoords[(i + 1) % 3].y;
		fangxiang2.x = screenCoords[i].x - screenCoords[(i + 2) % 3].x;
		fangxiang2.y = screenCoords[i].y - screenCoords[(i + 2) % 3].y;
		sin = std::abs(fangxiang1 ^ fangxiang2) / (std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y) * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));

		fangxiang1 = fangxiang1 / (sin * std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y));
		fangxiang2 = fangxiang2 / (sin * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));
		fangxiang1 = fangxiang1 / 2.5f;
		fangxiang2 = fangxiang2 / 2.5f;
		screenCoordsOpt[i] = Vec3f(fangxiang1.x + fangxiang2.x, fangxiang1.y + fangxiang2.y, 0.0f) + screenCoords[i];
	}

	Vec3f P;
	//包围盒内的每一像素根据重心坐标判断是否渲染
	for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
		for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
			P.z = 0.0f;

			//用扩大后的三角形求重心坐标
			Vec3f bc_screen = baryCentric(screenCoordsOpt, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
				continue;
			}

			//如果扩大的包围盒里的某个像素在扩大的三角形内部，则将该像素内的四个采样点再进行操作
			Vec3f msaapoint[4];//子采样点
			msaapoint[0].x = P.x - 0.25;//左下
			msaapoint[0].y = P.y - 0.25;
			msaapoint[1].x = P.x - 0.25;//左上
			msaapoint[1].y = P.y + 0.25;
			msaapoint[2].x = P.x + 0.25;//右下
			msaapoint[2].y = P.y - 0.25;
			msaapoint[3].x = P.x + 0.25;//右上
			msaapoint[3].y = P.y + 0.25;
			msaapoint[0].z = 0.0f;
			msaapoint[1].z = 0.0f;
			msaapoint[2].z = 0.0f;
			msaapoint[3].z = 0.0f;

			int count = 0;			//ShadowMap卷积数
			float shadow = 1.0f;	//阴影值，默认为1
			float x, y;				//子采样点对应像素坐标，映射颜色缓冲
			bool colorFlag = false;	//如果像素已经着色则为true
			TGAColor colorAfteruv;	//着色完成之后的值

			//对每一个子采样点循环
			for (int i = 0; i < 4; i++) {
				Vec3f bc_screenn = baryCentric(screenCoords, msaapoint[i]);
				//若重心坐标某分量小于0则表示此点在三角形外（在边上和顶点上认为是三角形内部）
				if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
					continue;
				}
				//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
				msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);

				//计算对应着色缓冲区坐标
				if (i == 0) {
					//左下
					x = P.x * 2.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 1) {
					//左上
					x = P.x * 2.0f;
					y = P.y * 2.0f;
				}
				else if (i == 2) {
					//右下
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 3) {
					//右上
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f;
				}

				std::lock_guard<std::mutex> lck(mtx);

				if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] <= msaapoint[i].z) {
					if (!colorFlag) {
						//计算shadow
						if (g_shadowRenderFlag) {
							msaapoint[i].z = 0;
							for (int k = 0; k < 3; k++) {
								msaapoint[i].z += screenCoords[k][2] * bc_screenn[k];
							}

							Vec3f sbP = { 0.0f, 0.0f, 0.0f };
							std::vector<float> v4 = { msaapoint[i].x, msaapoint[i].y, msaapoint[i].z, 1.0f };
							float four = 0.0f;
							for (int k = 0; k < 4; k++) {
								sbP.x += uniformMShadow.m[0][k] * v4[k];
								sbP.y += uniformMShadow.m[1][k] * v4[k];
								sbP.z += uniformMShadow.m[2][k] * v4[k];
								four += uniformMShadow.m[3][k] * v4[k];
							}
							sbP.x /= four;
							sbP.y /= four;
							sbP.z /= four;

							float bias = 0.00015f;						
							sbP.z -= bias;

							for (int x = -1; x <= 1; ++x)
							{
								for (int y = -1; y <= 1; ++y)
								{
									if (g_bufferIndex == 0) {
										if (g_ModelshadowMapBuffer0[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 1) {
										if (g_ModelshadowMapBuffer1[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 2) {
										if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 3) {
										if (g_ModelshadowMapBuffer3[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else {
										if (g_ModelshadowMapBuffer4[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
								}
							}
							shadow = 1.0f - .7 * count / 9.0f;
						}

						float u = 0.0f, v = 0.0f;				//uv坐标
						TGAColor coloruv;						//uv颜色值
						float intensityAfterDiffuse = 0.0f;		//插值得到的漫反射强度
						float intensityAfterSpecular = 0.0f;	//插值得到的镜面反射强度

						//重心坐标，计算当前像素的zbuffer，顶点uv坐标插值
						for (int i = 0; i < 3; i++) {
							//透视插值的UV
							u += g_model->getUV(face[i * 3 + 1]).x * bc_screenn[i] / cameraCoords[i].z;
							v += g_model->getUV(face[i * 3 + 1]).y * bc_screenn[i] / cameraCoords[i].z;
							intensityAfterDiffuse += diffuseIntensity[i] * bc_screenn[i];
							intensityAfterSpecular += specularIntensity[i] * bc_screenn[i];
						}

						//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
						msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);
						u *= msaapoint[i].z;
						v *= msaapoint[i].z;

						//此时的uv坐标要变换到真实图片坐标
						std::pair<int, int> uvxy = computeUV(u, v, g_textureTga);
						coloruv = g_textureTga->get(uvxy.first, uvxy.second);

						//阴影值不会影响环境光量
						//for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (0.2 + 1.0 * intensityAfterDiffuse + 0.1 * intensityAfterSpecular), 255);
						for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (KaKdKsNs[0] + shadow * (KaKdKsNs[1] * intensityAfterDiffuse + KaKdKsNs[2] * intensityAfterSpecular)), 255);

						colorFlag = true;
					}

					//如果深度值相同，则求均
					int r = colorAfteruv[0];
					int g = colorAfteruv[1];
					int b = colorAfteruv[2];
					if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] == msaapoint[i].z) {
						r += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0];
						g += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1];
						b += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2];
						r /= 2;
						g /= 2;
						b /= 2;
					}

					g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] = msaapoint[i].z;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0] = r;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1] = g;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2] = b;
				}
			}
		}
	}
}

void fragmentShader8xMSAAOptimize(Vec3f screenCoords[3], std::vector<int>& face, float diffuseIntensity[3], float specularIntensity[3], std::vector<float>& KaKdKsNs, Vec3f cameraCoords[3], Matrix& uniformMShadow) {
	Vec2f boxmin(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	Vec2f boxmax(0.0f, 0.0f);
	Vec2f clamp(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			boxmin.x = std::max(0.0f, std::min(boxmin.x, screenCoords[i].x));
			boxmin.y = std::max(0.0f, std::min(boxmin.y, screenCoords[i].y));

			boxmax.x = std::min(clamp.x, std::max(boxmax.x, screenCoords[i].x));
			boxmax.y = std::min(clamp.y, std::max(boxmax.y, screenCoords[i].y));
		}
	}

	Vec3f screenCoordsOpt[3];//扩大一个像素后的三角形屏幕坐标

	//向量缩放
	for (int i = 0; i < 3; i++) {
		Vec2f fangxiang1;
		Vec2f fangxiang2;
		float sin = 0.0f;
		fangxiang1.x = screenCoords[i].x - screenCoords[(i + 1) % 3].x;
		fangxiang1.y = screenCoords[i].y - screenCoords[(i + 1) % 3].y;
		fangxiang2.x = screenCoords[i].x - screenCoords[(i + 2) % 3].x;
		fangxiang2.y = screenCoords[i].y - screenCoords[(i + 2) % 3].y;
		sin = std::abs(fangxiang1 ^ fangxiang2) / (std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y) * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));

		fangxiang1 = fangxiang1 / (sin * std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y));
		fangxiang2 = fangxiang2 / (sin * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));
		fangxiang1 = fangxiang1 / 2.5f;
		fangxiang2 = fangxiang2 / 2.5f;
		screenCoordsOpt[i] = Vec3f(fangxiang1.x + fangxiang2.x, fangxiang1.y + fangxiang2.y, 0.0f) + screenCoords[i];
	}

	Vec3f P;
	//包围盒内的每一像素根据重心坐标判断是否渲染
	for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
		for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
			P.z = 0.0f;

			//用扩大后的三角形求重心坐标
			Vec3f bc_screen = baryCentric(screenCoordsOpt, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
				continue;
			}

			//如果扩大的包围盒里的某个像素在扩大的三角形内部，则将该像素内的四个采样点再进行操作
			Vec3f msaapoint[8];//子采样点
			msaapoint[0].x = P.x - 0.25;//左下
			msaapoint[0].y = P.y - 0.25;
			msaapoint[1].x = P.x - 0.25;//左上
			msaapoint[1].y = P.y + 0.25;
			msaapoint[2].x = P.x + 0.25;//右下
			msaapoint[2].y = P.y - 0.25;
			msaapoint[3].x = P.x + 0.25;//右上
			msaapoint[3].y = P.y + 0.25;
			msaapoint[4].x = P.x - 0.25;//左
			msaapoint[4].y = P.y;
			msaapoint[5].x = P.x + 0.25;//右
			msaapoint[5].y = P.y;
			msaapoint[6].x = P.x;		//上
			msaapoint[6].y = P.y - 0.25;
			msaapoint[7].x = P.x;		//下
			msaapoint[7].y = P.y + 0.25;
			msaapoint[8].x = P.x;		//中
			msaapoint[8].y = P.y;
			msaapoint[0].z = 0.0f;
			msaapoint[1].z = 0.0f;
			msaapoint[2].z = 0.0f;
			msaapoint[3].z = 0.0f;
			msaapoint[4].z = 0.0f;
			msaapoint[5].z = 0.0f;
			msaapoint[6].z = 0.0f;
			msaapoint[7].z = 0.0f;
			msaapoint[8].z = 0.0f;

			int count = 0;			//ShadowMap卷积数
			float shadow = 1.0f;	//阴影值，默认为1
			float x, y;				//子采样点对应像素坐标，映射颜色缓冲
			bool colorFlag = false;	//如果像素已经着色则为true
			TGAColor colorAfteruv;	//着色完成之后的值

			//对每一个子采样点循环
			for (int i = 0; i < 9; i++) {
				Vec3f bc_screenn = baryCentric(screenCoords, msaapoint[i]);
				//若重心坐标某分量小于0则表示此点在三角形外（在边上和顶点上认为是三角形内部）
				if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
					continue;
				}
				//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
				msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);

				//计算对应着色缓冲区坐标
				if (i == 0) {
					//左下
					x = P.x * 2.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 1) {
					//左上
					x = P.x * 2.0f;
					y = P.y * 2.0f;
				}
				else if (i == 2) {
					//右下
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f + 1.0f;
				}
				else if (i == 3) {
					//右上
					x = P.x * 2.0f + 1.0f;
					y = P.y * 2.0f;
				}
				else if (i == 4) {
					//左
					//x = P.x * 
				}
				std::lock_guard<std::mutex> lck(mtx);

				if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] <= msaapoint[i].z) {
					if (!colorFlag) {
						//计算shadow
						if (g_shadowRenderFlag) {
							msaapoint[i].z = 0;
							for (int k = 0; k < 3; k++) {
								msaapoint[i].z += screenCoords[k][2] * bc_screenn[k];
							}

							Vec3f sbP = { 0.0f, 0.0f, 0.0f };
							std::vector<float> v4 = { msaapoint[i].x, msaapoint[i].y, msaapoint[i].z, 1.0f };
							float four = 0.0f;
							for (int k = 0; k < 4; k++) {
								sbP.x += uniformMShadow.m[0][k] * v4[k];
								sbP.y += uniformMShadow.m[1][k] * v4[k];
								sbP.z += uniformMShadow.m[2][k] * v4[k];
								four += uniformMShadow.m[3][k] * v4[k];
							}
							sbP.x /= four;
							sbP.y /= four;
							sbP.z /= four;

							float bias = 0.00015f;
							sbP.z -= bias;

							for (int x = -1; x <= 1; ++x)
							{
								for (int y = -1; y <= 1; ++y)
								{
									if (g_bufferIndex == 0) {
										if (g_ModelshadowMapBuffer0[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 1) {
										if (g_ModelshadowMapBuffer1[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 2) {
										if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else if (g_bufferIndex == 3) {
										if (g_ModelshadowMapBuffer3[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
									else {
										if (g_ModelshadowMapBuffer4[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
											count++;
									}
								}
							}
							shadow = 1.0f - .7 * count / 9.0f;
						}

						float u = 0.0f, v = 0.0f;				//uv坐标
						TGAColor coloruv;						//uv颜色值
						float intensityAfterDiffuse = 0.0f;		//插值得到的漫反射强度
						float intensityAfterSpecular = 0.0f;	//插值得到的镜面反射强度

						//重心坐标，计算当前像素的zbuffer，顶点uv坐标插值
						for (int i = 0; i < 3; i++) {
							//透视插值的UV
							u += g_model->getUV(face[i * 3 + 1]).x * bc_screenn[i] / cameraCoords[i].z;
							v += g_model->getUV(face[i * 3 + 1]).y * bc_screenn[i] / cameraCoords[i].z;
							intensityAfterDiffuse += diffuseIntensity[i] * bc_screenn[i];
							intensityAfterSpecular += specularIntensity[i] * bc_screenn[i];
						}

						//该采样点深度的插值，即msaapoint[i].z，这个插值是基于观察坐标的
						msaapoint[i].z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);
						u *= msaapoint[i].z;
						v *= msaapoint[i].z;

						//此时的uv坐标要变换到真实图片坐标
						std::pair<int, int> uvxy = computeUV(u, v, g_textureTga);
						coloruv = g_textureTga->get(uvxy.first, uvxy.second);

						//阴影值不会影响环境光量
						//for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (0.2 + 1.0 * intensityAfterDiffuse + 0.1 * intensityAfterSpecular), 255);
						for (int i = 0; i < 3; i++) colorAfteruv[i] = std::min<float>(coloruv[i] * (KaKdKsNs[0] + shadow * (KaKdKsNs[1] * intensityAfterDiffuse + KaKdKsNs[2] * intensityAfterSpecular)), 255);

						colorFlag = true;
					}

					//如果深度值相同，则求均
					int r = colorAfteruv[0];
					int g = colorAfteruv[1];
					int b = colorAfteruv[2];
					if (g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] == msaapoint[i].z) {
						r += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0];
						g += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1];
						b += g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2];
						r /= 2;
						g /= 2;
						b /= 2;
					}

					g_MSAAZBuffer[int(x + (2 * y * SCREEN_WIDTH))] = msaapoint[i].z;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][0] = r;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][1] = g;
					g_colorBuffer[int(x + (2 * y * SCREEN_WIDTH))][2] = b;
				}
			}
		}
	}
}

//测试优化效率
void speedTestOptimize(Vec3f screenCoords[3], std::vector<int>& face, float diffuseIntensity[3], float specularIntensity[3], std::vector<float>& KaKdKsNs, Vec3f cameraCoords[3], Matrix& uniformMShadow) {
	//查找三角形包围盒边界，有更优解？
	Vec2f boxmin(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	Vec2f boxmax(0.0f, 0.0f);
	Vec2f clamp(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
	for (int i = 0; i < 3; i++) {
		//第一层循环，遍历三角形的三个顶点
		for (int j = 0; j < 2; j++) {
			//第二层循环，根据顶点数值缩小包围盒的范围
			boxmin.x = std::max(0.0f, std::min(boxmin.x, screenCoords[i].x));
			boxmin.y = std::max(0.0f, std::min(boxmin.y, screenCoords[i].y));

			boxmax.x = std::min(clamp.x, std::max(boxmax.x, screenCoords[i].x));
			boxmax.y = std::min(clamp.y, std::max(boxmax.y, screenCoords[i].y));
		}
	}

	Vec3f screenCoordsOpt[3];//扩大一个像素后的三角形屏幕坐标

	clock_t startExtra, finishExtra;
	long ms1 = 0;//扩大三角形的计算耗时
	startExtra = clock();
	//循环10000次，计算耗时
	{
		Timer();
		for (int j = 0; j < 10000; j++) {
			//向量缩放
			for (int i = 0; i < 3; i++) {
				Vec2f fangxiang1;
				Vec2f fangxiang2;
				float sin = 0.0f;
				fangxiang1.x = screenCoords[i].x - screenCoords[(i + 1) % 3].x;
				fangxiang1.y = screenCoords[i].y - screenCoords[(i + 1) % 3].y;
				fangxiang2.x = screenCoords[i].x - screenCoords[(i + 2) % 3].x;
				fangxiang2.y = screenCoords[i].y - screenCoords[(i + 2) % 3].y;
				sin = std::abs(fangxiang1 ^ fangxiang2) / (std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y) * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));

				fangxiang1 = fangxiang1 / (sin * std::sqrt(fangxiang1.x * fangxiang1.x + fangxiang1.y * fangxiang1.y));
				fangxiang2 = fangxiang2 / (sin * std::sqrt(fangxiang2.x * fangxiang2.x + fangxiang2.y * fangxiang2.y));
				fangxiang1 = fangxiang1 / 2.5f;
				fangxiang2 = fangxiang2 / 2.5f;
				screenCoordsOpt[i] = Vec3f(fangxiang1.x + fangxiang2.x, fangxiang1.y + fangxiang2.y, 0.0f) + screenCoords[i];
			}
		}
	}
	finishExtra = clock();
	ms1 = (int)(finishExtra - startExtra);
	//std::cout << startExtra << "*" << finishExtra << "*" << ms << '\n';

	int countBeforeOpt = 0;		//优化前检测次数
	int countAfterOpt = 0;		//优化后检测次数
	float percentOpt = 0.0f;	//该面片优化效率
	long ms2 = 0;				//子采样点重心坐标检测时间

	//计算优化后的检测次数
	Vec3f P;//包围盒内的每一像素
	for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
		for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
			countAfterOpt++;

			P.z = 0.0f;
			//用扩大后的三角形求重心坐标
			Vec3f bc_screen = baryCentric(screenCoordsOpt, P);
			//若重心坐标某分量小于0则表示此点在三角形外（在边上和顶点上认为是三角形内部）
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
				continue;
			}

			//如果扩大的包围盒里的某个像素在扩大的三角形内部，则将该像素内的四个采样点再进行操作
			Vec3f msaapoint[4];//子采样点
			msaapoint[0].x = P.x - 0.25;//左下
			msaapoint[0].y = P.y - 0.25;
			msaapoint[1].x = P.x - 0.25;//左上
			msaapoint[1].y = P.y + 0.25;
			msaapoint[2].x = P.x + 0.25;//右下
			msaapoint[2].y = P.y - 0.25;
			msaapoint[3].x = P.x + 0.25;//右上
			msaapoint[3].y = P.y + 0.25;
			msaapoint[0].z = 0.0f;
			msaapoint[1].z = 0.0f;
			msaapoint[2].z = 0.0f;
			msaapoint[3].z = 0.0f;

			//对每一个子采样点循环
			for (int i = 0; i < 4; i++) {
				countAfterOpt++;

				clock_t start, finish;
				start = clock();
				//用原来的三角形坐标求重心坐标
				Vec3f bc_screenn = baryCentric(screenCoords, msaapoint[i]);
				//若重心坐标某分量小于0则表示此点在三角形外（在边上和顶点上认为是三角形内部）
				if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
					continue;
				}
				finish = clock();
				ms2 = finish - start;

				//求重心坐标等
				//计算着色、修改颜色缓冲区
			}
		}
	}

	//计算优化前的检测次数
	Vec3f PP;//包围盒内的每一像素
	for (PP.x = boxmin.x; PP.x <= boxmax.x; PP.x++) {
		for (PP.y = boxmin.y; PP.y <= boxmax.y; PP.y++) {
			//定义每个子采样点
			Vec3f msaapoint[4];//子采样点
			msaapoint[0].x = PP.x - 0.25;//左下
			msaapoint[0].y = PP.y - 0.25;
			msaapoint[1].x = PP.x - 0.25;//左上
			msaapoint[1].y = PP.y + 0.25;
			msaapoint[2].x = PP.x + 0.25;//右下
			msaapoint[2].y = PP.y - 0.25;
			msaapoint[3].x = PP.x + 0.25;//右上
			msaapoint[3].y = PP.y + 0.25;
			msaapoint[0].z = 0.0f;
			msaapoint[1].z = 0.0f;
			msaapoint[2].z = 0.0f;
			msaapoint[3].z = 0.0f;

			//对每一个子采样点循环
			for (int i = 0; i < 4; i++) {
				countBeforeOpt++;

				//用原来的三角形坐标求重心坐标
				Vec3f bc_screenn = baryCentric(screenCoords, msaapoint[i]);
				if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
					continue;
				}

				//求重心坐标等
				//着色、写入颜色缓冲区等
			}
		}
	}

	percentOpt = (float)(countBeforeOpt - countAfterOpt) / (float)countBeforeOpt;
	g_perOpt += percentOpt;
	g_faceFragmentCount++;

	//std::cout << ms << '\n';
	//std::cout << countBeforeOpt << "*" << countAfterOpt  << "*" << percentOpt << "*" << g_perOpt  << "*" << g_faceFragmentCount << '\n';

	//当三角形面片在屏幕上大时，优化最好，如果缩小模型、改变窗口导致面片变小，则优化降低
	//虽然在额外开销（计算三角形大小）上花费不大，但是在采样点处计算重心坐标判断是否在三角形内的花销也是十分小的
}

//顶点着色器（模拟）
void vertexShader() {
	Matrix ModelView = lookat(cameraEye, cameraCenter, cameraUp);				//模型视图矩阵
	Matrix ProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);	//投影矩阵
	Matrix ViewPort = viewport(3.0f / 8.0f, 3.0f / 8.0f);						//视口矩阵
	Matrix MVP = ViewPort * ProjectionMatrix * ModelView;						//MVP矩阵
	Matrix uniformMShadow = LMVP * MVP.inverse();								//阴影映射矩阵

	//每帧都清0
	//g_perOpt = 0.0f;
	//g_faceFragmentCount = 0;

	//依次绘制模型的每个模型组
	for (int g = 0; g < g_model->getNumOfGroup(); g++) {
		g_textureTga = g_model->getTexture(g);							//某个模型组的纹理对象
		std::vector<float> KaKdKsNs = g_model->getKaKdKsNs(g);			//某个模型组的光照数据

		tbb::blocked_range<int> range(0, g_model->getNumOfGroupFace(g));
		tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

			//如果反向顺序渲染，可以解决特定情况下面部两个面片重叠问题
			for (int i = range.begin(); i != range.end(); i++) {
				float diffuseIntensity[3] = { 0.0f, 0.0f, 0.0f };		//漫反射光强度
				float specularIntensity[3] = { 0.0f, 0.0f, 0.0f };		//镜面反射光强度

				std::vector<int> face = g_model->getGroupFace(g, i);	//某个模型组下的面片数据
				Vec3f worldCoords[3];									//面片世界坐标
				Vec3f cameraCoords[3];									//面片摄像机坐标
				Vec3f perspectiveCoords[3];								//面片NDC空间坐标
				Vec3f screenCoords[3];									//面片屏幕坐标
				Vec3f seeCoords[3];										//面片摄像机观察/坐标反向
				Vec3f cosineHalfWay;									//半程向量计算得到的角度余弦值
				Vec3f normAfter;										//变换后的正确法向
				float wComponent = 1.0f;								//透视除法w值
				bool viewPortCullingFlag = false;						//视口剔除，如果至少一个点在NDC空间内则为true
				bool frustumtCullingFlag = true;						//视锥剔除，如果在摄像机后为false

				for (int j = 0; j < 3; j++) {
					worldCoords[j] = g_model->getVert(face[j * 3]);
					cameraCoords[j] = m2vn(ModelView * v2m(worldCoords[j]));

					//视锥剔除
					//如果此时z值大于等于0则在摄像机背后，应该舍弃
					if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
						frustumtCullingFlag = false;
						break;
					}
					seeCoords[j].x = -cameraCoords[j].x;
					seeCoords[j].y = -cameraCoords[j].y;
					seeCoords[j].z = -cameraCoords[j].z;
					seeCoords[j].normalize();

					//存储w值
					wComponent = -cameraCoords[j].z;
					perspectiveCoords[j] = m2vn(ProjectionMatrix * v2m(cameraCoords[j]));
					//先进行透视除法再进行视口变换，除数为能使w为1的值
					//剪裁在透视除法之前
					perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
					perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
					perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
				}

				if (frustumtCullingFlag) {
					for (int j = 0; j < 3; j++) {
						//视口剔除（未剪裁，保留至少一个点在NDC空间的三角形）
						//这种方法依旧有问题，三角形三个顶点都在外但是与屏幕相交的情况
						//可以用很大的视口比映射优化，不过严格意义上依然有错误
						//可以取等
						if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
							if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
								viewPortCullingFlag = true;
								break;
							}
						}
					}

					if (viewPortCullingFlag) {
						Vec3f c(0.0f, 0.0f, 1.0f);	//正z轴的向量
						Vec3f n;					//面片法线
						float intensity;			//视线（反）和三角形方向点乘得到cos值

						n = (perspectiveCoords[1] - perspectiveCoords[0]) ^ (perspectiveCoords[2] - perspectiveCoords[1]);
						intensity = n * c;

						//背面剔除，有问题，有些角度，某些地方有轻微的异常，精度问题？NDC空间中进行
						//如果是精度问题，稍微扩大容错范围，为什么精度问题能造成这种现象？
						//值为负舍弃（背对）
						if (intensity < -0.001) continue;

						for (int j = 0; j < 3; j++) {
							Vec3f PointToLightVec;									//点到光源向量
							float lightToPointDisSq;								//点到光源距离平方
							Vec3f PointToLightVecSee = PointToLightVec.normalize();	//变换过后摄像机坐标下点到光源向量
							PointToLightVec = g_lightPos - worldCoords[j];
							lightToPointDisSq = PointToLightVec.x * PointToLightVec.x + PointToLightVec.y * PointToLightVec.y + PointToLightVec.z * PointToLightVec.z;
							PointToLightVecSee = PointToLightVec.normalize();

							screenCoords[j] = m2vn(ViewPort * v2m(perspectiveCoords[j]));
							//屏幕坐标在这里必须取整，后面使用的都是基于屏幕的计算
							//截尾取整
							screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
							//四舍五入
							//screenCoords[j] = Vec3f(int(screenCoords[j].x + 0.5), int(screenCoords[j].y + 0.5), screenCoords[j].z);

							//根据顶点法线计算三个顶点光照
							//如果只是计算漫反射光照，光照方向是基于原来的世界坐标，法线也是，所以这里不需要对法线进行变换
							diffuseIntensity[j] = std::max(0.0f, g_illum * (g_model->getNorm(g, i, j) * PointToLightVec.normalize()) / lightToPointDisSq);
							//这里的法线变换不太严谨，严格来说法线向量的变换矩阵是顶点变换矩阵的逆转置矩阵
							normAfter = m2vn(ModelView * v2mn(g_model->getNorm(g, i, j))).normalize();
							//为了计算高光，光线也要变换
							PointToLightVecSee = m2vn(ModelView * v2mn(PointToLightVecSee)).normalize();

							//半程向量计算高光
							Vec3f vectorHalfWay = (PointToLightVecSee + seeCoords[j]).normalize();//半程向量
							cosineHalfWay[j] = vectorHalfWay * normAfter;

							//KaKdKsNs可以自己变化，因为不同渲染器都有不同的实现方法
							//specularIntensity[i] = pow(std::max(cosineHalfWay[i], 0.0f), 16);
							specularIntensity[j] = pow(std::max(cosineHalfWay[j], 0.0f), KaKdKsNs[3]);
							//考虑物体距离光源的衰减
							specularIntensity[j] = g_illum * specularIntensity[j] / lightToPointDisSq;
							//严格意义上来说，还要考虑摄像机和物体的距离，考虑这部分的光的衰减
						}

						//送入片段着色器
						//fragmentShader(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);
						fragmentShaderOptimize(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);
						//fragmentShader8xMSAAOptimize(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);

						/*long msFragmentShader, msFragmentShaderOptimize;
						clock_t start, finish;
						start = clock();
						fragmentShader(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);
						finish = clock();
						msFragmentShader = finish - start;
						start = clock();
						fragmentShaderOptimize(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);
						finish = clock();
						msFragmentShaderOptimize = finish - start;
						std::cout << msFragmentShader << "*" << msFragmentShaderOptimize << '\n';*/

						//优化效率测试
						//speedTestOptimize(screenCoords, face, diffuseIntensity, specularIntensity, KaKdKsNs, cameraCoords, uniformMShadow);
					}
				}
			}

			});
	}

	//计算平均提升百分比
	/*g_perOpt /= g_faceFragmentCount;
	std::cout << g_faceFragmentCount << '\n';
	std::cout << g_perOpt << '\n';*/

	//绘制颜色缓冲区
	drawMSAAColorBuffer();
}

//线框渲染管线
void frameRender() {
	Matrix ModelView = lookat(cameraEye, cameraCenter, cameraUp);
	Matrix ProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);
	Matrix ViewPort = viewport(3.0f / 8.0f, 3.0f / 8.0f);
	SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);

	tbb::blocked_range<int> range1(0, g_model->getNumOfGroup());
	tbb::parallel_for(range1, [&](const tbb::blocked_range<int>& range1) {

		for (int g = range1.begin(); g != range1.end(); g++) {

			tbb::blocked_range<int> range(0, g_model->getNumOfGroupFace(g));
			tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

				for (int i = range.begin(); i != range.end(); i++) {
					std::vector<int> face = g_model->getGroupFace(g, i);
					Vec3f worldCoords[3];
					Vec3f cameraCoords[3];
					Vec3f perspectiveCoords[3];
					Vec3f screenCoords[3];
					float wComponent = 1.0f;
					bool flag = false;
					bool flagw = true;
					for (int j = 0; j < 3; j++) {
						worldCoords[j] = g_model->getVert(face[j * 3]);
						cameraCoords[j] = m2vn(ModelView * v2m(worldCoords[j]));
						if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
							flagw = false;
							break;
						}
						wComponent = -cameraCoords[j].z;
						perspectiveCoords[j] = m2vn(ProjectionMatrix * v2m(cameraCoords[j]));
						perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
						perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
						perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
					}
					if (flagw) {
						for (int j = 0; j < 3; j++) {
							if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
								if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
									flag = true;
									break;
								}
							}
						}
						if (flag) {
							for (int j = 0; j < 3; j++) {
								screenCoords[j] = m2vn(ViewPort * v2m(perspectiveCoords[j]));
								screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
							}

							std::lock_guard<std::mutex> lck(mtx);
							drawLineBresenham(screenCoords[0].x, screenCoords[0].y, screenCoords[1].x, screenCoords[1].y);
							drawLineBresenham(screenCoords[1].x, screenCoords[1].y, screenCoords[2].x, screenCoords[2].y);
							drawLineBresenham(screenCoords[2].x, screenCoords[2].y, screenCoords[0].x, screenCoords[0].y);
						}
					}
				}

				});
		}

		});
}

//坐标轴渲染管线
void coordinatesRender() {
	Matrix ModelView = lookat(cameraEye, cameraCenter, cameraUp);
	Matrix ProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);
	Matrix ViewPort = viewport(3.0f / 8.0f, 3.0f / 8.0f);
	Vec3f worldCoords[4];
	Vec3f cameraCoords[4];
	Vec3f perspectiveCoords[4];
	Vec3f screenCoords[4];
	float wComponent = 1.0f;
	bool flag = false;
	bool flagw = true;
	for (int j = 0; j < 4; j++) {
		worldCoords[j] = g_coordinatesVector[j];
		cameraCoords[j] = m2vn(ModelView * v2m(worldCoords[j]));
		if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
			flagw = false;
			break;
		}
		wComponent = -cameraCoords[j].z;
		perspectiveCoords[j] = m2vn(ProjectionMatrix * v2m(cameraCoords[j]));
		perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
		perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
		perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
	}
	if (flagw) {
		for (int j = 0; j < 4; j++) {
			if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
				if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
					flag = true;
					break;
				}
			}
		}
		if (flag) {
			for (int j = 0; j < 4; j++) {
				screenCoords[j] = m2vn(ViewPort * v2m(perspectiveCoords[j]));
				screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
			}

			SDL_SetRenderDrawColor(g_renderer, 0, 255, 0, 255);
			drawLineBresenham(screenCoords[3].x, screenCoords[3].y, screenCoords[1].x, screenCoords[1].y);
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 255, 255);
			drawLineBresenham(screenCoords[3].x, screenCoords[3].y, screenCoords[2].x, screenCoords[2].y);
			SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 255);
			drawLineBresenham(screenCoords[3].x, screenCoords[3].y, screenCoords[0].x, screenCoords[0].y);
		}
	}
}

//点光源位置渲染管线
void lightPosRender() {
	Matrix ModelView = lookat(cameraEye, cameraCenter, cameraUp);
	Matrix ProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);
	Matrix ViewPort = viewport(3.0f / 8.0f, 3.0f / 8.0f);
	Vec3f worldCoords[7];
	Vec3f cameraCoords[7];
	Vec3f perspectiveCoords[7];
	Vec3f screenCoords[7];
	float wComponent = 1.0f;
	bool flag = false;
	bool flagw = true;
	for (int j = 0; j < 7; j++) {
		worldCoords[j] = g_lightPosVector[j];
		cameraCoords[j] = m2vn(ModelView * v2m(worldCoords[j]));
		if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
			flagw = false;
			break;
		}
		wComponent = -cameraCoords[j].z;
		perspectiveCoords[j] = m2vn(ProjectionMatrix * v2m(cameraCoords[j]));
		perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
		perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
		perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
	}
	if (flagw) {
		for (int j = 0; j < 7; j++) {
			if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
				if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
					flag = true;
					break;
				}
			}
		}
		if (flag) {
			for (int j = 0; j < 7; j++) {
				screenCoords[j] = m2vn(ViewPort * v2m(perspectiveCoords[j]));
				screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
			}
			SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[1].x, screenCoords[1].y);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[2].x, screenCoords[2].y);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[0].x, screenCoords[0].y);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[4].x, screenCoords[4].y);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[5].x, screenCoords[5].y);
			drawLineBresenham(screenCoords[6].x, screenCoords[6].y, screenCoords[3].x, screenCoords[3].y);
		}
	}
}

//flat、材质、纹理、材质加纹理、shadowNM渲染管线
void texAndMtlAndRender() {
	Matrix ModelView = lookat(cameraEye, cameraCenter, cameraUp);
	Matrix ProjectionMatrix = setFrustum(FOVY * DegToRad, ASPECT, NEAR, FAR);
	Matrix ViewPort = viewport(3.0f / 8.0f, 3.0f / 8.0f);
	Matrix MVP = ViewPort * ProjectionMatrix * ModelView;
	Matrix uniformMShadow = LMVP * MVP.inverse();

	for (int g = 0; g < g_model->getNumOfGroup(); g++) {
		g_textureTga = g_model->getTexture(g);
		std::vector<float> KaKdKsNs = g_model->getKaKdKsNs(g);

		tbb::blocked_range<int> range(0, g_model->getNumOfGroupFace(g));
		tbb::parallel_for(range, [&](const tbb::blocked_range<int>& range) {

			for (int i = range.begin(); i != range.end(); i++) {
				float diffuseIntensity[3] = { 0., 0., 0. };
				float specularIntensity[3] = { 0., 0., 0. };
				float flatIntensity = 0.0f;
				std::vector<int> face = g_model->getGroupFace(g, i);
				Vec3f worldCoords[3];
				Vec3f cameraCoords[3];
				Vec3f perspectiveCoords[3];
				Vec3f screenCoords[3];
				Vec3f seeCoords[3];
				Vec3f cosineHalfWay;
				Vec3f normAfter;
				float wComponent = 1.0f;
				bool flag = false;
				bool flagw = true;
				for (int j = 0; j < 3; j++) {
					worldCoords[j] = g_model->getVert(face[j * 3]);
					cameraCoords[j] = m2vn(ModelView * v2m(worldCoords[j]));
					if (cameraCoords[j].z > 0. || cameraCoords[j].z == 0.) {
						flagw = false;
						break;
					}
					seeCoords[j].x = -cameraCoords[j].x;
					seeCoords[j].y = -cameraCoords[j].y;
					seeCoords[j].z = -cameraCoords[j].z;
					seeCoords[j].normalize();
					wComponent = -cameraCoords[j].z;
					perspectiveCoords[j] = m2vn(ProjectionMatrix * v2m(cameraCoords[j]));
					perspectiveCoords[j].x = perspectiveCoords[j].x / wComponent;
					perspectiveCoords[j].y = perspectiveCoords[j].y / wComponent;
					perspectiveCoords[j].z = perspectiveCoords[j].z / wComponent;
				}
				if (flagw) {
					for (int j = 0; j < 3; j++) {
						if (perspectiveCoords[j].x > -1.0 && perspectiveCoords[j].x < 1.0) {
							if (perspectiveCoords[j].y > -1.0 && perspectiveCoords[j].y < 1.0) {
								flag = true;
								break;
							}
						}
					}
					if (flag) {
						Vec3f c(0.0f, 0.0f, 1.0f);
						Vec3f n = (perspectiveCoords[1] - perspectiveCoords[0]) ^ (perspectiveCoords[2] - perspectiveCoords[1]);
						float intensity = n * c;
						if (intensity < -0.001) continue;
						for (int j = 0; j < 3; j++) {
							Vec3f light_dirnp = g_lightPos - worldCoords[j];
							float lightpointd = light_dirnp.x * light_dirnp.x + light_dirnp.y * light_dirnp.y + light_dirnp.z * light_dirnp.z;
							Vec3f light_dirnmw = light_dirnp.normalize();
							screenCoords[j] = m2vn(ViewPort * v2m(perspectiveCoords[j]));
							screenCoords[j] = Vec3f(int(screenCoords[j].x), int(screenCoords[j].y), screenCoords[j].z);
							diffuseIntensity[j] = std::max(0.0f, g_illum * (g_model->getNorm(g, i, j) * light_dirnp.normalize()) / lightpointd);
							normAfter = m2vn(ModelView * v2mn(g_model->getNorm(g, i, j))).normalize();
							light_dirnmw = m2vn(ModelView * v2mn(light_dirnmw)).normalize();
							Vec3f ban = (light_dirnmw + seeCoords[j]).normalize();
							cosineHalfWay[j] = ban * normAfter;
							specularIntensity[j] = pow(std::max(cosineHalfWay[j], 0.0f), KaKdKsNs[3]);
							specularIntensity[j] = g_illum * specularIntensity[j] / lightpointd;
						}

						//片段着色器
						Vec2f boxmin(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
						Vec2f boxmax(0.0f, 0.0f);
						Vec2f clamp(SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f);
						for (int ii = 0; ii < 3; ii++) {
							for (int j = 0; j < 2; j++) {
								boxmin.x = std::max(0.0f, std::min(boxmin.x, screenCoords[ii].x));
								boxmin.y = std::max(0.0f, std::min(boxmin.y, screenCoords[ii].y));
								boxmax.x = std::min(clamp.x, std::max(boxmax.x, screenCoords[ii].x));
								boxmax.y = std::min(clamp.y, std::max(boxmax.y, screenCoords[ii].y));
							}
						}
						Vec3f P;
						for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
							for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
								P.z = 0.0f;
								Vec3f bc_screenn = baryCentric(screenCoords, P);
								if (bc_screenn.x < 0 || bc_screenn.y < 0 || bc_screenn.z < 0) {
									continue;
								}
								P.z = 1.0f / (bc_screenn[0] / cameraCoords[0].z + bc_screenn[1] / cameraCoords[1].z + bc_screenn[2] / cameraCoords[2].z);

								//需要先上锁，再判断深度缓冲
								//如果判断深度缓冲后上锁，会出现深度值已经被刷新，反而继续着色
								std::lock_guard<std::mutex> lck(mtx);

								if (g_NoneMSAAZBuffer[int(P.x + P.y * SCREEN_WIDTH)] <= P.z) {
									float u = 0.0f, v = 0.0f;
									TGAColor coloruv;
									TGAColor colorAfteruv;
									float intensityGPAfter = 0.0f;
									float intensityGPAfterspec = 0.0f;
									for (int ii = 0; ii < 3; ii++) {
										//u += g_model->uv(face[ii * 3 + 1]).x * bc_screenn[ii];
										//v += g_model->uv(face[ii * 3 + 1]).y * bc_screenn[ii];
										u += g_model->getUV(face[ii * 3 + 1]).x * bc_screenn[ii] / cameraCoords[ii].z;
										v += g_model->getUV(face[ii * 3 + 1]).y * bc_screenn[ii] / cameraCoords[ii].z;
										intensityGPAfter += diffuseIntensity[ii] * bc_screenn[ii];
										intensityGPAfterspec += specularIntensity[ii] * bc_screenn[ii];
									}
									u *= P.z;
									v *= P.z;
									std::pair<int, int> uvxy = computeUV(u, v, g_textureTga);
									coloruv = g_textureTga->get(uvxy.first, uvxy.second);

									if (g_textureRenderFlag) {
										for (int ii = 0; ii < 3; ii++) colorAfteruv[ii] = coloruv[ii];
									}
									else if (g_materialRenderFlag) {
										for (int ii = 0; ii < 3; ii++) colorAfteruv[ii] = std::min<float>(255.0f * (KaKdKsNs[0] + KaKdKsNs[1] * intensityGPAfter + KaKdKsNs[2] * intensityGPAfterspec), 255.0f);
									}
									else if (g_renderFlag) {
										for (int ii = 0; ii < 3; ii++) colorAfteruv[ii] = std::min<float>(coloruv[ii] * (KaKdKsNs[0] + KaKdKsNs[1] * intensityGPAfter + KaKdKsNs[2] * intensityGPAfterspec), 255.0f);
									}
									else if (g_flatRenderFlag) {
										float flat = (diffuseIntensity[0] + diffuseIntensity[1] + diffuseIntensity[2]) / 3.0f;
										for (int ii = 0; ii < 3; ii++) colorAfteruv[ii] = std::min<float>(255.0f * flat, 255.0f);
									}
									else if (g_shadowNMRenderFlag) {
										float shadow = 1.0f;
										int count = 0;
										float Pz = 0;
										for (int k = 0; k < 3; k++) {
											Pz += screenCoords[k][2] * bc_screenn[k];
										}

										Vec3f sbP = { 0.0f, 0.0f, 0.0f };
										std::vector<float> v4 = { P.x, P.y, Pz, 1.0f };
										float four = 0.0f;
										for (int k = 0; k < 4; k++) {
											sbP.x += uniformMShadow.m[0][k] * v4[k];
											sbP.y += uniformMShadow.m[1][k] * v4[k];
											sbP.z += uniformMShadow.m[2][k] * v4[k];
											four += uniformMShadow.m[3][k] * v4[k];
										}
										sbP.x /= four;
										sbP.y /= four;
										sbP.z /= four;

										float bias = 0.00015f;
										sbP.z -= bias;

										for (int x = -1; x <= 1; ++x)
										{
											for (int y = -1; y <= 1; ++y)
											{
												if (g_bufferIndex == 0) {
													if (g_ModelshadowMapBuffer0[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
														count++;
												}
												else if (g_bufferIndex == 1) {
													if (g_ModelshadowMapBuffer1[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
														count++;
												}
												else if (g_bufferIndex == 2) {
													if (g_ModelshadowMapBuffer2[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
														count++;
												}
												else if (g_bufferIndex == 3) {
													if (g_ModelshadowMapBuffer3[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
														count++;
												}
												else {
													if (g_ModelshadowMapBuffer4[int(sbP.x) + int(sbP.y) * SCREEN_WIDTH + x + y * SCREEN_WIDTH] < sbP.z)
														count++;
												}
											}
										}
										shadow = 1.0f - .7 * count / 9.0f;

										for (int ii = 0; ii < 3; ii++) colorAfteruv[ii] = std::min<float>(coloruv[ii] * (KaKdKsNs[0] + shadow * (KaKdKsNs[1] * intensityGPAfter + KaKdKsNs[2] * intensityGPAfterspec)), 255.0f);
									}

									int r = colorAfteruv[0];
									int g = colorAfteruv[1];
									int b = colorAfteruv[2];
									/*if (g_NoneMSAAZBuffer[int(P.x + P.y * SCREEN_WIDTH)] == P.z) {
										r += g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][0];
										g += g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][1];
										b += g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][2];
										r /= 2;
										g /= 2;
										b /= 2;
									}

									g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][0] = r;
									g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][1] = g;
									g_smallColorBuffer[int(P.x + P.y * SCREEN_WIDTH)][2] = b;
									g_NoneMSAAZBuffer[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
									SDL_SetRenderDrawColor(g_renderer, b, g, r, 255);
									SDLDrawPixel(P.x, P.y);*/

									//顶点可能归属多个面片，会被多次渲染，不能和边一样简单除2
									if (g_NoneMSAAZBuffer[int(P.x + P.y * SCREEN_WIDTH)] == P.z) {
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].r += r;
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].g += g;
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].b += b;
										g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)]++;
										//这样可以输出顶点，好玩
										/*if (g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)] > 2) {
											SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
											SDLDrawPixel(P.x, P.y);
										}*/
										r = g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].r / g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)];
										g = g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].g / g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)];
										b = g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].b / g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)];
									}
									else {
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].r = r;
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].g = g;
										g_colorOptBuffer[int(P.x + P.y * SCREEN_WIDTH)].b = b;
										g_smallColorCount[int(P.x + P.y * SCREEN_WIDTH)] = 1;
									}							

									g_NoneMSAAZBuffer[int(P.x + P.y * SCREEN_WIDTH)] = P.z;
									SDL_SetRenderDrawColor(g_renderer, b, g, r, 255);
									SDLDrawPixel(P.x, P.y);
								}
							}
						}
					}
				}
			}

			});
	}
}

//根据Flag开启渲染管线
void startRenderPipeline() {
	if (g_MSAARenderFlag)
		vertexShader();
	if (g_frameRenderFlag)
		frameRender();
	if (g_materialRenderFlag || g_textureRenderFlag || g_renderFlag || g_flatRenderFlag || g_shadowNMRenderFlag)
		texAndMtlAndRender();
	if (g_coordinatesRenderFlag)
		coordinatesRender();
	if (g_lightPosRenderFlag)
		lightPosRender();
}

int main(int argc, char* args[])
{
	//当点击按钮改变窗口大小后，转到程序入口处重新执行
begin:

	if (!init())
	{
		printf("Failed to initialize!\n");
		SDL_Quit();
	}
	else
	{
		//创建在按钮列表中的按钮
		createButton();

		//设置点光源、坐标轴GUI向量
		setGUIvector();

		//加载模型
		loadModel();

		//初始化buffer
		initializeBuffer();

		//得到阴影贴图
		createShadowMap();

		//设置默认参数
		setDefaultParameter();

		double xpAfterx = 0.0;					//鼠标右键按下后鼠标横向偏移值
		double xpAftery = 0.0;					//鼠标右键按下后鼠标纵向偏移值
		Vec2f mousePosNow = Vec2f(0.0f, 0.0f);	//鼠标前一刻位置
		Vec2f mousePosPre = Vec2f(0.0f, 0.0f);	//鼠标后一刻位置
		bool rightMouseKeyPress = false;		//鼠标右键按下标志
		bool leftMouseKeyPress = false;			//鼠标左键按下标志
		int mouseWheelAmount = 0;				//鼠标滚轮滑动数

		//摄像机到视点位置的长度
		float cameraEyeToCenterLength = sqrt((cameraEye[0] - cameraCenter[0]) * (cameraEye[0] - cameraCenter[0]) + (cameraEye[1] - cameraCenter[1]) * (cameraEye[1] - cameraCenter[1]) + (cameraEye[2] - cameraCenter[2]) * (cameraEye[2] - cameraCenter[2]));
		//初始视点到摄像机位置向量与Y轴弧度值
		float cameraEyeToCenterYRadian = asin(fabs(cameraEye[1] - cameraCenter[1]) / cameraEyeToCenterLength);
		//初始视点到摄像机位置向量投影在XZ平面的Z轴弧度值
		float cameraEyeToCenterZRadian = asin((cameraEye[0] - cameraCenter[0]) / cameraEyeToCenterLength);

		Vec3f zChange;						//摄像机z轴偏移量
		Vec3f xChange;						//摄像机x轴偏移量
		Vec3f yChange;						//摄像机y轴偏移量

		SDL_Event event;					//用户控制事件
		bool keysEvent[1024] = { false };	//键盘事件缓冲区

		clock_t lastFrame = 0;				//计时器
		clock_t currentFrame = 0;			//计时器
		float deltaTime = 0;				//帧间隔时间
		int timer = 0;						//帧计数器
		float floatFPS = 0.0f;				//FPS
		std::string stringFPS;				//FPS文本

		bool quit = false;					//主循环标志
		while (!quit)
		{
			if (g_model == NULL) continue;

			//计算deltaTime与FPS
			timer++;
			currentFrame = clock();
			deltaTime = float((float)currentFrame - lastFrame) / CLOCKS_PER_SEC;
			lastFrame = currentFrame;
			floatFPS = 1 / deltaTime;

			//控制FPS显示速率
			if (timer >= 1 / deltaTime) {
				//std::cout << "Current Frames Per Second:" << floatFPS << std::endl;
				timer = 0;
				stringFPS = floatToString(floatFPS);
			}

			//用户控制速度补偿，好像没什么用，不如直接算
			float cameraSpeed = 5 * deltaTime;

			//处理队列中的事件
			if (SDL_PollEvent(&event) != 0)
			{
				//检测鼠标是否在按钮上方
				detectMouseOnButton(event);

				//根据事件类型响应
				switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym < 1024)
						keysEvent[event.key.keysym.sym] = true;
					break;
				case SDL_KEYUP:
					if (event.key.keysym.sym < 1024)
						keysEvent[event.key.keysym.sym] = false;
					break;
				case SDL_MOUSEMOTION:
					mousePosNow.x = (float)event.motion.x;
					mousePosNow.y = SCREEN_HEIGHT - (float)event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
					case SDL_BUTTON_LEFT:
						leftMouseKeyPress = true;
						break;
					case SDL_BUTTON_RIGHT:
						mousePosPre = mousePosNow;
						rightMouseKeyPress = true;
						break;
					default:
						break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch (event.button.button)
					{
					case SDL_BUTTON_LEFT:
						//检测鼠标点击时，是否在按钮上方，并响应事件
						mouseLeftUpControl(event);
						if (g_gotoFlag) {
							g_gotoFlag = false;
							goto begin;
						}
						leftMouseKeyPress = false;
						break;
					case SDL_BUTTON_RIGHT:
						rightMouseKeyPress = false;
						break;
					default:
						break;
					}
					break;
				case SDL_MOUSEWHEEL:
					mouseWheelAmount = event.wheel.y;
					break;
				case SDL_QUIT:
					quit = true;
					break;
				default:
					break;
				}

				//处理鼠标响应	
				if (rightMouseKeyPress || leftMouseKeyPress) {
					Vec2f offset = mousePosNow - mousePosPre;
					mousePosPre = mousePosNow;

					/*float sensitivity = 1;
					offset.x *= sensitivity;
					offset.y *= sensitivity;*/

					if (rightMouseKeyPress) {
						if (offset.x != 0 || offset.y != 0) {
							//xpAfterx += -offset.x * cameraSpeed;
							//xpAftery += -offset.y * cameraSpeed;
							xpAfterx += -offset.x;
							xpAftery += -offset.y;
						}
					}
					if (leftMouseKeyPress) {
					}
				}
				if (mouseWheelAmount != 0) {
					//改变相机视野
					FOVY -= mouseWheelAmount;
					FOVY = std::max(0.0f, std::min(90.0f, FOVY));
					mouseWheelAmount = 0;
				}
				//处理键盘响应，这里需要加入用户控制速度补偿，帧数高的时候会高频触发
				if (keysEvent[SDLK_w]) {
					zChange = (cameraCenter - cameraEye).normalize();
					zChange = zChange * cameraSpeed;
					cameraEye = cameraEye + zChange;
					cameraCenter = cameraCenter + zChange;
					cameraEyeToCenterLength = sqrt((cameraEye[0] - cameraCenter[0]) * (cameraEye[0] - cameraCenter[0]) + (cameraEye[1] - cameraCenter[1]) * (cameraEye[1] - cameraCenter[1]) + (cameraEye[2] - cameraCenter[2]) * (cameraEye[2] - cameraCenter[2]));
				}
				if (keysEvent[SDLK_a]) {
					xChange = cameraUp ^ (cameraCenter - cameraEye);
					xChange.normalize();
					xChange = xChange * cameraSpeed;
					cameraEye = cameraEye + xChange;
					cameraCenter = cameraCenter + xChange;
					float cameraEyeToCenterYRadian = asin(fabs(cameraEye[1] - cameraCenter[1]) / cameraEyeToCenterLength);//初始y轴弧度值
					float cameraEyeToCenterZRadian = asin((cameraEye[0] - cameraCenter[0]) / cameraEyeToCenterLength);//初始投影在XZ平面的z轴弧度值
				}
				if (keysEvent[SDLK_s]) {
					zChange = (cameraCenter - cameraEye).normalize();
					zChange = zChange * cameraSpeed;
					cameraEye = cameraEye - zChange;
					cameraCenter = cameraCenter - zChange;
					cameraEyeToCenterLength = sqrt((cameraEye[0] - cameraCenter[0]) * (cameraEye[0] - cameraCenter[0]) + (cameraEye[1] - cameraCenter[1]) * (cameraEye[1] - cameraCenter[1]) + (cameraEye[2] - cameraCenter[2]) * (cameraEye[2] - cameraCenter[2]));
				}
				if (keysEvent[SDLK_d]) {
					xChange = cameraUp ^ (cameraCenter - cameraEye);
					xChange.normalize();
					xChange = xChange * cameraSpeed;
					cameraEye = cameraEye - xChange;
					cameraCenter = cameraCenter - xChange;
					float cameraEyeToCenterYRadian = asin(fabs(cameraEye[1] - cameraCenter[1]) / cameraEyeToCenterLength);//初始y轴弧度值
					float cameraEyeToCenterZRadian = asin((cameraEye[0] - cameraCenter[0]) / cameraEyeToCenterLength);//初始投影在XZ平面的z轴弧度值
				}
				if (keysEvent[SDLK_q]) {
					yChange = cameraUp ^ (cameraCenter - cameraEye);
					yChange = (cameraCenter - cameraEye) ^ yChange;
					yChange.normalize();
					yChange = yChange * cameraSpeed;
					cameraEye = cameraEye + yChange;
					cameraCenter = cameraCenter + yChange;
					float cameraEyeToCenterYRadian = asin(fabs(cameraEye[1] - cameraCenter[1]) / cameraEyeToCenterLength);//初始y轴弧度值
					float cameraEyeToCenterZRadian = asin((cameraEye[0] - cameraCenter[0]) / cameraEyeToCenterLength);//初始投影在XZ平面的z轴弧度值
				}
				if (keysEvent[SDLK_e]) {
					yChange = cameraUp ^ (cameraCenter - cameraEye);
					yChange = (cameraCenter - cameraEye) ^ yChange;
					yChange.normalize();
					yChange = yChange * cameraSpeed;
					cameraEye = cameraEye - yChange;
					cameraCenter = cameraCenter - yChange;
					float cameraEyeToCenterYRadian = asin(fabs(cameraEye[1] - cameraCenter[1]) / cameraEyeToCenterLength);//初始y轴弧度值
					float cameraEyeToCenterZRadian = asin((cameraEye[0] - cameraCenter[0]) / cameraEyeToCenterLength);//初始投影在XZ平面的z轴弧度值
				}
			}			

			//清空屏幕
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0xFF);
			SDL_RenderClear(g_renderer);

			//绘制剪裁框
			//void printClipWin();

			//初始化运行过程中的buffer
			initializeRunBuffer();

			//对摄像机位置进行计算
			//上下移动，鼠标y变化，cameraEyeToCenterYRadian变化，x、y、z变化
			//左右移动，鼠标x变化，cameraEyeToCenterZRadian变化，x、z变化
			float radianYChange = xpAftery / (SCREEN_HEIGHT / 2) + cameraEyeToCenterYRadian;
			float radianZChange = xpAfterx / (SCREEN_WIDTH / 2) + cameraEyeToCenterZRadian;
			cameraUp[1] = 1;
			//当上下移动角度大于90度小于270度需要将cameraUp向量取反
			if (fmod(radianYChange, 2 * PI) > (0.5 * PI) && fmod(radianYChange, 2 * PI) < (1.5 * PI)) {
				cameraUp[1] = -1;
			}
			cameraEye[0] = cameraEyeToCenterLength * cos(radianYChange) * sin(radianZChange) + cameraCenter[0];
			cameraEye[1] = cameraEyeToCenterLength * sin(radianYChange) + cameraCenter[1];
			cameraEye[2] = cameraEyeToCenterLength * cos(radianYChange) * cos(radianZChange) + cameraCenter[2];

			//根据不同Flag开启不同渲染管线
			startRenderPipeline();

			//绘制所有按钮
			drawAllButton(stringFPS);

			//更新屏幕
			SDL_RenderPresent(g_renderer);
		}
	}

	//释放资源关闭SDL
	close();

	return 0;
}