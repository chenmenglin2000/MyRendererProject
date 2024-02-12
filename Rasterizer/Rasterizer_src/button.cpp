#include "button.h"

Button::Button(int windowX, int windowY, int buttonWidth, int buttonHeight, std::string text) {
	m_buttonRect.x = windowX;
	m_buttonRect.y = windowY;
	m_buttonRect.w = buttonWidth;
	m_buttonRect.h = buttonHeight;
	m_buttonText = text;
	m_buttonStatus = false;
	m_buttonMouseOn = false;
	m_textTexture.ptr = NULL;
	m_textTexture.color = { 255, 255, 255, 255 };
	m_textTexture.ptr = NULL;
	m_textTexture.content = text;
	m_textTexture.height = 0;
}

void Button::drawButton(TTF_Font* font, SDL_Renderer* ren) {
	if (m_buttonStatus == true) {
		//������״̬��ť
		SDL_SetRenderDrawColor(ren, 119, 119, 119, 255);
	}
	else {
		if (m_buttonMouseOn == true) {
			//��ѡ��״̬��ť
			SDL_SetRenderDrawColor(ren, 69, 69, 69, 255);
		}
		else {
			//������״̬��ť
			SDL_SetRenderDrawColor(ren, 39, 39, 39, 255);
		}
	}
	SDL_RenderFillRect(ren, &m_buttonRect);

	//��ʾ����
	drawText(font, ren);
}

void Button::drawText(TTF_Font* font, SDL_Renderer* ren) {
	//���ı�����ת��Ϊ���������ڴ����У�ֻ��ת��һ��
	if (m_textTexture.ptr == NULL)
	{
		SDL_Surface* surf = TTF_RenderText_Blended(font, m_buttonText.c_str(), m_textTexture.color);
		if (surf == nullptr) {
			/*TTF_CloseFont(g_font);
			SDL_Quit();
			exit(1);*/
		}
		m_textTexture.ptr = SDL_CreateTextureFromSurface(ren, surf);
		if (m_textTexture.ptr == nullptr) {
			/*SDL_Quit();
			exit(1);*/
		}
		//�������
		SDL_FreeSurface(surf);
	}

	//������ʾ�ľ��η�Χ���Ȱ�ť�ķ�ΧС
	SDL_Rect dst;
	dst.x = m_buttonRect.x + m_buttonRect.w / 10;
	dst.y = m_buttonRect.y + m_buttonRect.w / 10;
	dst.w = m_buttonRect.w - m_buttonRect.w / 5;
	dst.h = m_buttonRect.h - m_buttonRect.w / 5;
	SDL_RenderCopy(ren, m_textTexture.ptr, NULL, &dst);
}

void Button::drawTextFps(TTF_Font* font, SDL_Renderer* ren, std::string fpsText) {
	SDL_Texture* tex = NULL;
	SDL_Surface* surf = TTF_RenderText_Blended(font, fpsText.c_str(), m_textTexture.color);
	//ÿ�ζ���Ҫһ����ָ�������ܷ���ֵ��������m_textTexture.ptr
	tex = SDL_CreateTextureFromSurface(ren, surf);
	SDL_FreeSurface(surf);

	//��ȡ����ĳ���
	SDL_QueryTexture(tex, NULL, NULL, &m_textTexture.width, &m_textTexture.height);
	m_textTexture.content = fpsText;
	//������ʾ�ľ��η�Χ
	SDL_Rect dst;
	dst.x = m_buttonRect.x;
	dst.y = m_buttonRect.y;
	//������ֳ��������SDL_QueryTexture����ʧ�ܣ�
	if (m_textTexture.width == 0 || m_textTexture.height == 0) {
		m_textTexture.width = 1;
		m_textTexture.height = 1;
	}
	dst.w = m_textTexture.width * m_buttonRect.h / m_textTexture.height;
	dst.h = m_buttonRect.h;
	SDL_RenderCopy(ren, tex, NULL, &dst);
	SDL_DestroyTexture(tex);
}

void Button::drawTextNormal(TTF_Font* font, SDL_Renderer* ren) {
	//���ı�����ת��Ϊ���������ڴ����У�ֻ��ת��һ��
	if (m_textTexture.ptr == NULL)
	{
		SDL_Surface* surf = TTF_RenderText_Blended(font, m_buttonText.c_str(), m_textTexture.color);
		if (surf == nullptr) {
			/*TTF_CloseFont(g_font);
			SDL_Quit();
			exit(1);*/
		}
		m_textTexture.ptr = SDL_CreateTextureFromSurface(ren, surf);
		//���Ի�ȡ����ĳ���
		SDL_QueryTexture(m_textTexture.ptr, NULL, NULL, &m_textTexture.width, &m_textTexture.height);
		if (m_textTexture.ptr == nullptr) {
			/*SDL_Quit();
			exit(1);*/
		}
		//Clean up the surface and font
		SDL_FreeSurface(surf);

		if (m_textTexture.width == 0 || m_textTexture.height == 0) {
			m_textTexture.width = 1;
			m_textTexture.height = 1;
		}
	}

	//������ʾ�ľ��η�Χ
	SDL_Rect dst;
	dst.x = m_buttonRect.x;
	dst.y = m_buttonRect.y;
	dst.w = m_textTexture.width * m_buttonRect.h / m_textTexture.width;
	dst.h = m_buttonRect.h;
	SDL_RenderCopy(ren, m_textTexture.ptr, NULL, &dst);
}

SDL_Rect Button::getRect() {
	return m_buttonRect;
}

void Button::onClicked() {
}

void Button::onClickedSecond() {
}