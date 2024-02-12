#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <string>
#include "SDL.h"
#include "SDL_ttf.h"

struct TextTexture {
	std::string content;	//文本内容
	SDL_Color color;		//文本颜色
	SDL_Texture* ptr;		//文本纹理指针
	int width;				//文本纹理自身宽度
	int height;				//文本纹理自身高度
};

class Button
{
public:
	Button(int x, int y, int w, int h, std::string title);
	~Button() {
		SDL_DestroyTexture(m_textTexture.ptr);
	};

	bool m_buttonStatus;	//按钮状态标识，FALSE正常，TRUE激活
	bool m_buttonMouseOn;	//按钮状态标识，FALSE未选中，TRUE选中

	//点击处于正常状态下的按钮
	virtual void onClicked();
	//点击处于激活状态下的按钮
	virtual void onClickedSecond();
	//获取按钮矩形对象
	virtual SDL_Rect getRect();

	//绘制按钮，包括绘制文本
	virtual void drawButton(TTF_Font* font, SDL_Renderer* ren);
	//绘制文本，有拉伸
	virtual void drawText(TTF_Font* font, SDL_Renderer* ren);
	//绘制FPS文本，每次都要更新文本内容
	void drawTextFps(TTF_Font* font, SDL_Renderer* ren, std::string fpsText);
	//绘制正常文本，没有拉伸
	void drawTextNormal(TTF_Font* font, SDL_Renderer* ren);

protected:
	SDL_Rect m_buttonRect;		//按钮的矩形
	std::string m_buttonText;	//按钮的文本
	TextTexture m_textTexture;	//文本纹理结构体
};

#endif //__BUTTON_H__