#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <string>
#include "SDL.h"
#include "SDL_ttf.h"

struct TextTexture {
	std::string content;	//�ı�����
	SDL_Color color;		//�ı���ɫ
	SDL_Texture* ptr;		//�ı�����ָ��
	int width;				//�ı�����������
	int height;				//�ı���������߶�
};

class Button
{
public:
	Button(int x, int y, int w, int h, std::string title);
	~Button() {
		SDL_DestroyTexture(m_textTexture.ptr);
	};

	bool m_buttonStatus;	//��ť״̬��ʶ��FALSE������TRUE����
	bool m_buttonMouseOn;	//��ť״̬��ʶ��FALSEδѡ�У�TRUEѡ��

	//�����������״̬�µİ�ť
	virtual void onClicked();
	//������ڼ���״̬�µİ�ť
	virtual void onClickedSecond();
	//��ȡ��ť���ζ���
	virtual SDL_Rect getRect();

	//���ư�ť�����������ı�
	virtual void drawButton(TTF_Font* font, SDL_Renderer* ren);
	//�����ı���������
	virtual void drawText(TTF_Font* font, SDL_Renderer* ren);
	//����FPS�ı���ÿ�ζ�Ҫ�����ı�����
	void drawTextFps(TTF_Font* font, SDL_Renderer* ren, std::string fpsText);
	//���������ı���û������
	void drawTextNormal(TTF_Font* font, SDL_Renderer* ren);

protected:
	SDL_Rect m_buttonRect;		//��ť�ľ���
	std::string m_buttonText;	//��ť���ı�
	TextTexture m_textTexture;	//�ı�����ṹ��
};

#endif //__BUTTON_H__