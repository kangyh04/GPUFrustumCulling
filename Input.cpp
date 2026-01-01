#include "Input.h"
#include <iostream>

bool Input::Initialize(HWND hwnd)
{
	windowHandle = hwnd;
	RAWINPUTDEVICE rid[2];

	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x06;
	rid[0].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY;
	rid[0].hwndTarget = hwnd;

	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x02;
	rid[1].dwFlags = RIDEV_INPUTSINK;
	rid[1].hwndTarget = hwnd;

	if (!RegisterRawInputDevices(rid, _countof(rid), sizeof(rid[0])))
	{
		return false;
	}

	return true;
}

void Input::Update()
{
	keyPrevStates = keyStates;
	mousePrevState = mouseState;

	mouseState.DeltaPos = POINT{ 0, 0 };
	mouseState.WheelDelta = 0;

	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(windowHandle, &cursorPos);
	mouseState.MousePos = cursorPos;
}

bool Input::IsKeyPressed(int key)
{
	if (key < 0 || 256 < key)
	{
		return false;
	}
	return keyStates[key] && !keyPrevStates[key];
}

bool Input::IsKeyDown(int key)
{
	if (key < 0 || 256 < key)
	{
		return false;
	}
	return keyStates[key];
}

bool Input::IsKeyReleased(int key)
{
	if (key < 0 || 256 < key)
	{
		return false;
	}
	return !keyStates[key] && keyPrevStates[key];
}

bool Input::IsMouseButtonPressed(int button)
{
	auto curr = IsMouseButtonDown(button);
	auto previous = false;
	switch (button)
	{
	case 0:
		previous = mousePrevState.LeftButton;
		break;
	case 1:
		previous = mousePrevState.RightButton;
		break;
	case 2:
		previous = mousePrevState.MiddleButton;
		break;
	}
	return curr && !previous;
}

bool Input::IsMouseButtonDown(int button)
{
	switch (button)
	{
	case 0:
		return mouseState.LeftButton;
	case 1:
		return mouseState.RightButton;
	case 2:
		return mouseState.MiddleButton;
	}
	return false;
}

bool Input::IsMouseButtonReleased(int button)
{
	auto curr = IsMouseButtonDown(button);
	auto previous = false;
	switch (button)
	{
	case 0:
		previous = mousePrevState.LeftButton;
		break;
	case 1:
		previous = mousePrevState.RightButton;
		break;
	case 2:
		previous = mousePrevState.MiddleButton;
		break;
	}
	return !curr && previous;
}

MouseState Input::GetMouseState()
{
	return mouseState;
}

void Input::ShowMouseCursor(bool show)
{
	ShowCursor(show);
}

void Input::ProcessInput(LPARAM lParam)
{
	UINT dwSize;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

	if (dwSize == 0)
	{
		return;
	}

	vector<BYTE> buffer(dwSize);
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
	{
		return;
	}

	auto raw = reinterpret_cast<RAWINPUT*>(buffer.data());

	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		ProcessKeyboard(raw->data.keyboard);
	}
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		ProcessMouse(raw->data.mouse);
	}
}

void Input::ProcessKeyboard(const RAWKEYBOARD& keyboard)
{
	auto vKey = keyboard.VKey;
	auto isDown = !(keyboard.Flags & RI_KEY_BREAK);

	if (vKey < 265)
	{
		keyStates[vKey] = isDown;
	}
}

void Input::ProcessMouse(const RAWMOUSE& mouse)
{
	if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE) {
		mouseState.DeltaPos.x = mouse.lLastX - mouseState.MousePos.x;
		mouseState.DeltaPos.y = mouse.lLastY - mouseState.MousePos.y;
	}
	else {
		mouseState.DeltaPos.x += mouse.lLastX;
		mouseState.DeltaPos.y += mouse.lLastY;
	}

	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
	{
		mouseState.LeftButton = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
	{
		mouseState.LeftButton = false;
	}

	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
	{
		mouseState.RightButton = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
	{
		mouseState.RightButton = false;
	}

	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
	{
		mouseState.MiddleButton = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
	{
		mouseState.MiddleButton = false;
	}

	if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
	{
		mouseState.WheelDelta = static_cast<short>(mouse.usButtonData);
	}
}
