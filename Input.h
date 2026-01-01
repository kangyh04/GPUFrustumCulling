#pragma once

#include "D3DUtil.h"
#include "Singleton.h"

struct MouseState
{
	POINT MousePos;
	POINT DeltaPos;
	int WheelDelta;
	bool LeftButton;
	bool RightButton;
	bool MiddleButton;
};

class Input : public Singleton<Input>
{
	friend class Singleton<Input>;
public:
	bool Initialize(HWND hwnd);
	void Update();

	bool IsKeyPressed(int key);
	bool IsKeyDown(int key);
	bool IsKeyReleased(int key);
	bool IsMouseButtonPressed(int button);
	bool IsMouseButtonDown(int button);
	bool IsMouseButtonReleased(int button);
	MouseState GetMouseState();

	void ShowMouseCursor(bool show);

	void ProcessInput(LPARAM lParam);
private:
	void ProcessKeyboard(const RAWKEYBOARD& keyboard);
	void ProcessMouse(const RAWMOUSE& mouse);

private:
	array<bool, 256> keyStates{ false };
	array<bool, 256> keyPrevStates{ false };

	MouseState mouseState;
	MouseState mousePrevState;

	HWND windowHandle;
};
