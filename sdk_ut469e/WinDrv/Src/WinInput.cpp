/*=============================================================================
	WinInput.cpp: Input processing.
	Copyright 2022 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/
#include "WinDrv.h"

/*-----------------------------------------------------------------------------
	Shared Functionality.
-----------------------------------------------------------------------------*/

// Marco: Copy from URender::Deproject, but without need for FSceneNode Frame
static void DeprojectMouse(UViewport* Viewport, INT MouseX, INT MouseY, FVector& V)
{
	FLOAT FX = (FLOAT)Viewport->SizeX;
	FLOAT FY = (FLOAT)Viewport->SizeY;
	FLOAT FX2 = FX * 0.5f;
	FLOAT FY2 = FY * 0.5f;
	FLOAT Zoom = Viewport->Actor->OrthoZoom / (FX * 15.f);

	const FVector& Origin = Viewport->Actor->Location;
	FLOAT	 SX = (FLOAT)MouseX - FX2;
	FLOAT	 SY = (FLOAT)MouseY - FY2;

	switch (Viewport->Actor->RendMap)
	{
		case REN_OrthXY:
			V.X = +SX * Zoom + Origin.X;
			V.Y = +SY * Zoom + Origin.Y;
			V.Z = 0;
			break;
		case REN_OrthXZ:
			V.X = +SX * Zoom + Origin.X;
			V.Y = 0.0;
			V.Z = -SY * Zoom + Origin.Z;
			break;
		case REN_OrthYZ:
			V.X = 0.0;
			V.Y = +SX * Zoom + Origin.Y;
			V.Z = -SY * Zoom + Origin.Z;
			break;
		default:
			V = Origin;
	}
}

INT FWindowsMouseInputHandler::GetButtonStateIndex(UINT WindowMessage, WPARAM wParam)
{
	switch (WindowMessage)
	{
		case WM_LBUTTONDOWN: case WM_LBUTTONUP: return 0;
		case WM_RBUTTONDOWN: case WM_RBUTTONUP: return 1;
		case WM_MBUTTONDOWN: case WM_MBUTTONUP: return 2;
		case WM_XBUTTONDOWN: case WM_XBUTTONUP:
		{
			const INT Button = GET_XBUTTON_WPARAM(wParam);
			if (Button == XBUTTON1)
				return 3;
			return 4;
		}
		default:
			return -1;
	}
}


void FORCEINLINE FWindowsMouseInputHandler::UpdateCumulativeMovement(const LONG Cumulative)
{
	for (INT i = 0; i < NumButtons; ++i)
		ButtonStates[i].CumulativeMovementSinceLastClick += Cumulative;
}

LRESULT FWindowsMouseInputHandler::ProcessCommonEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		{
			const INT ButtonStateIndex = GetButtonStateIndex(iMessage, wParam);
			ButtonState* State = ButtonStateIndex != -1 ? &ButtonStates[ButtonStateIndex] : nullptr;
			if (State)
			{
				ButtonsClicked = TRUE;
				State->LastClickLocation.x = GET_X_LPARAM(lParam);
				State->LastClickLocation.y = GET_Y_LPARAM(lParam);
				if (GIsEditor)
				{
					State->CumulativeMovementSinceLastClick = 0;
					State->LastClickTime = appSecondsNew();
				}
				State->HaveLastClickLocation = TRUE;
				State->IsDown = TRUE;
				State->WasDown = FALSE;
			}
			return 0;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			const INT ButtonStateIndex = GetButtonStateIndex(iMessage, wParam);
			ButtonState* State = ButtonStateIndex != -1 ? &ButtonStates[ButtonStateIndex] : nullptr;
			if (State)
			{
				ButtonsClicked = TRUE;
				State->HaveLastClickLocation = FALSE;
				State->IsDown = FALSE;
				State->WasDown = TRUE;
			}
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			if (Viewport->IgnoreMouseWheel)
			{
				Viewport->IgnoreMouseWheel = 0;
				return 0;
			}
			WheelMoved = TRUE;
			const SWORD Delta = HIWORD(wParam);
			NewWheelPos += Delta;
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			//
			// stijn: We can no longer rely on WM_MOUSEMOVE for mouse cursor updates because:
			//
			// 1) Windows sometimes generates spurious WM_MOUSEMOVE events
			// 2) Win 10 FCU and later do not generate WM_MOUSEMOVE events after calling
			// SetCursorPos. This breaks relative mouse input.
			// 3) If you manually post a WM_MOUSEMOVE message after calling SetCursorPos,
			// the message can be posted _before_ other input messages that are either
			// (i) already in the message queue at the time of the PostMessage call,
			// (ii) being processed by the kernel at the time of the PostMessage call.
			// It is impossible to tell if these messages were processed before or after
			// the SetCursorPos call.
			// 
			return DefWindowProcW(Viewport->Window->hWnd, iMessage, wParam, lParam);
		}
		default:
		{
			return 0;
		}
	}
}

void FWindowsMouseInputHandler::ProcessInputUpdates(UWindowsViewport* Viewport)
{
	UBOOL UpdateViewport = CursorMoved | WheelMoved | ButtonsClicked;
	UBOOL ShouldStartDrag = FALSE, ShouldStopDrag = FALSE, DeliveredClick = FALSE;

	// Process wheel scrolling
	if (WheelMoved)
	{
		const LONG WheelDelta = NewWheelPos - OldWheelPos;
		if (WheelDelta != 0)
		{
			const UBOOL WheelDown = WheelDelta < 0;
			Viewport->CauseInputEvent(IK_MouseW, IST_Axis, WheelDelta);
			Viewport->CauseInputEvent(WheelDown ? IK_MouseWheelDown : IK_MouseWheelUp, IST_Press);
			Viewport->CauseInputEvent(WheelDown ? IK_MouseWheelDown : IK_MouseWheelUp, IST_Release);

			if (GIsEditor)
			{
				PostMessageW(Viewport->ParentWindow, WM_VSCROLL, MAKEWPARAM((WheelDown ? SB_LINEDOWN : SB_LINEUP), 0), WM_MOUSEWHEEL); // Hack to make the scroll bar work in the Texture Browser.
				Viewport->GetOuterUWindowsClient()->Engine->MouseDelta(Viewport, WheelDown ? MOUSE_WheelDown : MOUSE_WheelUp, 0, 0); // in ortho viewports, we use MouseDelta to zoom
			}
		}

		if (RelativeMouseMotion)
			OldWheelPos = NewWheelPos = 0;
		else
			OldWheelPos = NewWheelPos;
	}

	// Process button clicks
	if (ButtonsClicked)
	{
		UBOOL LeftButtonDown = FALSE, RightButtonDown = FALSE;

		for (INT i = 0; i < NumButtons; ++i)
		{
			if (ButtonStates[i].IsDown)
			{
				if (ButtonStates[i].ViewportButtonFlag == MOUSE_Right)
					RightButtonDown = TRUE;
				if (ButtonStates[i].ViewportButtonFlag == MOUSE_Left)
					LeftButtonDown = TRUE;
			}
		}

		for (INT i = 0; i < NumButtons; ++i)
		{
			if (ButtonStates[i].IsDown && !ButtonStates[i].WasDown)
			{
				Viewport->CauseInputEvent(ButtonStates[i].InputEventKey, IST_Press);

				if (ButtonStates[i].EnablesDrag)
					ShouldStartDrag = TRUE;

				if (GIsEditor && ButtonStates[i].HaveLastClickLocation)
				{
					//
					// Doing this allows the editor to know where the mouse was last clicked in
					// world coordinates without having to do hit tests and such.
					//

					// Figure out where the mouse was clicked, in world coordinates.
					FVector V;
					DeprojectMouse(Viewport,
						ButtonStates[i].LastClickLocation.x,
						ButtonStates[i].LastClickLocation.y,
						V);

					Viewport->GetOuterUWindowsClient()->Engine->edSetClickLocation(V);

					// Send a notification message to the editor frame.  This of course relies on the 
					// window hierarchy not changing ... if it does, update this!
					const HWND hwndEditorFrame = GetParent(GetParent(GetParent(Viewport->ParentWindow)));
					SendMessageW(hwndEditorFrame, WM_COMMAND, WM_SETCURRENTVIEWPORT, (LPARAM)Viewport);

					// Allows the editor to properly initiate box selections
					if (ButtonStates[i].ViewportButtonFlag == MOUSE_Left &&
						Viewport->Input->KeyDown(IK_Ctrl) && 
						Viewport->Input->KeyDown(IK_Alt))
					{
						Viewport->GetOuterUWindowsClient()->Engine->Click(
							Viewport,
							MOUSE_Left,
							static_cast<FLOAT>(ButtonStates[i].LastClickLocation.x),
							static_cast<FLOAT>(ButtonStates[i].LastClickLocation.y));

						DeliveredClick = TRUE;
					}
				}
			}
			else if (ButtonStates[i].WasDown && !ButtonStates[i].IsDown)
			{
				Viewport->CauseInputEvent(ButtonStates[i].InputEventKey, IST_Release);

				if (ButtonStates[i].EnablesDrag)
					ShouldStopDrag = TRUE;

				if (GIsEditor)
				{
					// check if we should deliver this click
					if (!(Viewport->BlitFlags & BLIT_Fullscreen) &&
						Viewport->SizeX && 
						Viewport->SizeY &&
						ButtonStates[i].CumulativeMovementSinceLastClick <= Viewport->ClickDetectionMovementThreshold &&
						Abs(ButtonStates[i].LastClickTime - appSecondsNew()) <= Viewport->ClickDetectionTimeThreshold &&
						// Always deliver clicks of buttons other than mouse left and mouse right
						((ButtonStates[i].ViewportButtonFlag != MOUSE_Left && ButtonStates[i].ViewportButtonFlag != MOUSE_Right)
							// and also delivers clicks of left and right if the other button is not currently held
							|| (!LeftButtonDown && !RightButtonDown)))
					{
						// Get mouse cursor position.
						POINT TempPoint = { 0,0 };
						::ClientToScreen(Viewport->Window->hWnd, &TempPoint);
						INT MouseX = Viewport->CapturingMouse ? Viewport->SavedCursor.x - TempPoint.x : ButtonStates[i].LastClickLocation.x;
						INT MouseY = Viewport->CapturingMouse ? Viewport->SavedCursor.y - TempPoint.y : ButtonStates[i].LastClickLocation.y;

						Viewport->GetOuterUWindowsClient()->Engine->Click(
							Viewport, 
							ButtonStates[i].ViewportButtonFlag,
							MouseX, 
							MouseY
						);

						DeliveredClick = TRUE;
						if (GIsEditor)
						{
							ButtonStates[i].CumulativeMovementSinceLastClick = 0;
							ButtonStates[i].LastClickTime = 0.f;
						}
					}
				}
			}

			ButtonStates[i].WasDown = ButtonStates[i].IsDown;
		}
	}

	// Note: UpdateViewport is not entirely reliable because DirectInput and RawInput will set
	// CursorMoved to TRUE if they detect _any_ movement while the mouse is not captured.
	// Neither of these handlers check if the cursor moved over the Viewport client area
	if (UpdateViewport)
	{
		DWORD ViewportButtonFlags = 0;
		LONG DX = 0, DY = 0;

		// Process cursor movement
		if (CursorMoved)
		{
			DX = NewCursorPos.x - OldCursorPos.x;
			DY = NewCursorPos.y - OldCursorPos.y;

			// Relative mouse motion is always relative to 0,0
			if (RelativeMouseMotion)
				OldCursorPos = NewCursorPos = POINT{ 0,0 };
			else
				OldCursorPos = NewCursorPos;
		}

		// Filter out small movement between down and up for one click
		if (GIsEditor && CursorMoved && (DX || DY))
		{
			LONG Cumulative = Abs(DX) + Abs(DY);
			DOUBLE LastClickTime = 0;

			for (INT i = 0; i < NumButtons; ++i)
				if (ButtonStates[i].IsDown && ButtonStates[i].LastClickTime > LastClickTime)
					LastClickTime = ButtonStates[i].LastClickTime;

			if (LastIgnoredMoveTime != LastClickTime)
			{
				LastIgnoredMoveTime = LastClickTime;
				CumulativeIgnoredMovement = 0;
			}

			UBOOL IsSmallMove = Cumulative <= Max(2, Viewport->ClickDetectionMovementThreshold/10);

			// Mouse move between down and up for one click
			if (IsSmallMove && 
				appSecondsNew() - LastClickTime < Viewport->ClickDetectionTimeThreshold/2 && 
				LastNotSmallMoveTime != LastClickTime && 
				CumulativeIgnoredMovement < Viewport->ClickDetectionMovementThreshold/2)
			{
				CumulativeIgnoredMovement += Cumulative;
				DX = DY = 0;
				CursorMoved = FALSE;
			}
			else if (!IsSmallMove)
			{
				LastNotSmallMoveTime = LastClickTime;
			}
		}

		// Accumulate button flags
		for (INT i = 0; i < NumButtons; ++i)
			if (ButtonStates[i].IsDown)
				ViewportButtonFlags |= ButtonStates[i].ViewportButtonFlag;

		if (Viewport->Input->KeyDown(IK_Shift)) ViewportButtonFlags |= MOUSE_Shift;
		if (Viewport->Input->KeyDown(IK_Ctrl)) ViewportButtonFlags |= MOUSE_Ctrl;
		if (Viewport->Input->KeyDown(IK_Alt)) ViewportButtonFlags |= MOUSE_Alt;

		// If cursor isn't captured, just do MousePosition.
		if (!Viewport->CapturingMouse)
		{
			// Get window rectangle.
			RECT TempRect;
			::GetClientRect(Viewport->Window->hWnd, &TempRect);
			POINT ClientPosition;
			::GetCursorPos(&ClientPosition);
			::ScreenToClient(Viewport->Window->hWnd, &ClientPosition);

			if (ClientPosition.x >= TempRect.left && ClientPosition.x < TempRect.right &&
				ClientPosition.y >= TempRect.top && ClientPosition.y < TempRect.bottom)
			{
				// We use MousePosition to:
				// * Detect clicks in the texture browser window
				// * Update the WindowsMouse coordinates in game
				// * Animate fractal textures
				Viewport->GetOuterUWindowsClient()->Engine->MousePosition(
					Viewport, 
					ViewportButtonFlags, 
					static_cast<FLOAT>(ClientPosition.x - TempRect.left), 
					static_cast<FLOAT>(ClientPosition.y - TempRect.top)
				);
			}
			else if (!DeliveredClick && !ShouldStartDrag && !ShouldStopDrag)
			{
				// No need to repaint the viewport if we didn't actually deliver any events
				UpdateViewport = FALSE;
			}
		}
		else if (CursorMoved)
		{
			// The mouse is captured and we detected movement _within_ the viewport!

			// Pass precise deltas to MouseDelta
			Viewport->GetOuterUWindowsClient()->Engine->MouseDelta(Viewport, ViewportButtonFlags, DX, DY);

			// Send to input subsystem.
			if (DX) Viewport->CauseInputEvent(IK_MouseX, IST_Axis, +DX);
			if (DY) Viewport->CauseInputEvent(IK_MouseY, IST_Axis, -DY);
		}

		if (UpdateViewport)
		{
			if (ShouldStartDrag)
			{
				Viewport->SetDrag(TRUE);
			}
			else if (ShouldStopDrag)
			{
				if (!Viewport->Input->KeyDown(IK_LeftMouse) &&
					!Viewport->Input->KeyDown(IK_MiddleMouse) &&
					!Viewport->Input->KeyDown(IK_RightMouse) &&
					!Viewport->Input->KeyDown(IK_MouseButton4) &&
					!Viewport->Input->KeyDown(IK_MouseButton5) &&
					!(Viewport->BlitFlags & BLIT_Fullscreen))
				{
					Viewport->SetDrag(FALSE);
				}
			}

			// Viewport isn't realtime, so we must update the frame here and now.
			if ((Viewport->CapturingMouse || WheelMoved || ButtonsClicked || DeliveredClick) && !Viewport->IsRealtime())
			{
				if (Viewport->Input->KeyDown(IK_Space))
					for (INT i = 0; i < Viewport->GetOuterUWindowsClient()->Viewports.Num(); i++)
						Viewport->GetOuterUWindowsClient()->Viewports(i)->RepaintPending = TRUE;
				else
					Viewport->RepaintPending = TRUE;
			}

			// TODO: Should we cache these values so we can avoid calling SetCursor for no reason?
			if (!Viewport->CapturingMouse && Viewport->bShowWindowsMouse && Viewport->SelectedCursor <= 6)
				SetCursor(Viewport->StandardCursors[Viewport->SelectedCursor]);
		}
	}

	CursorMoved = WheelMoved = ButtonsClicked = FALSE;
}

void FWindowsMouseInputHandler::ResetMouseState()
{
	// stijn: We should _not_ reset the mouse position here because
	// non-capturing viewports such as the texture browser need the
	// CursorPos to figure out where we clicked
	CursorMoved = ButtonsClicked = WheelMoved = FALSE;

	for (INT i = 0; i < NumButtons; ++i)
	{
		ButtonState* State = &ButtonStates[i];
		if (GIsEditor)
		{
			State->CumulativeMovementSinceLastClick = 0;
			State->LastClickTime = 0.f;
		}
		State->HaveLastClickLocation = FALSE;
		State->IsDown = State->WasDown = FALSE;
	}
}

/*-----------------------------------------------------------------------------
	Win32 Cursor Input.
-----------------------------------------------------------------------------*/

UBOOL FWindowsWin32InputHandler::SetupInput()
{
	ButtonStates[NumButtons++] = ButtonState(VK_LBUTTON, 0x8000, IK_LeftMouse, MOUSE_Left, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_RBUTTON, 0x8000, IK_RightMouse, MOUSE_Right, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_MBUTTON, 0x8000, IK_MiddleMouse, MOUSE_Middle, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_XBUTTON1, 0x8000, IK_MouseButton4, MOUSE_Button4, FALSE);
	ButtonStates[NumButtons++] = ButtonState(VK_XBUTTON2, 0x8000, IK_MouseButton5, MOUSE_Button5, FALSE);
	RelativeMouseMotion = FALSE;
	return TRUE;
}

#define MOUSE_RESET_THRESHOLD_PERCENTAGE 15

//
// stijn: The Windows mouse input APIs report absolute mouse coordinates.
// Converting absolute mouse coordinates to relative coordinates is possible if:
//
// 1) We calculate the difference between the absolute mouse coordinates from
// one frame to the next.
//
// 2) We make sure that the mouse cursor does not escape the viewport that
// captures it.
//
// We achieve the latter by warping the mouse cursor back to the center of the
// viewport whenever it comes too close to the edge of the viewport window.
//
void FWindowsWin32InputHandler::WarpMouseIfNecessary(UWindowsViewport* Viewport, UBOOL Force)
{
	RECT TempRect;
	const HWND ViewportWnd = static_cast<HWND>(Viewport->GetWindow());
	::GetClientRect(ViewportWnd, &TempRect);

	const INT ClientAreaWidth = Abs(TempRect.right - TempRect.left);
	const INT ClientAreaHeight = Abs(TempRect.top - TempRect.bottom);

	// Check if we're in the threshold zone
	const INT ResetThresholdX = ClientAreaWidth * MOUSE_RESET_THRESHOLD_PERCENTAGE / 100;
	const INT ResetThresholdY = ClientAreaHeight * MOUSE_RESET_THRESHOLD_PERCENTAGE / 100;

	const BOOL ShouldReset =
		Force ||
		OldCursorPos.x < TempRect.left + ResetThresholdX ||
		OldCursorPos.x > TempRect.right - ResetThresholdX ||
		OldCursorPos.y > TempRect.bottom - ResetThresholdY ||
		OldCursorPos.y < TempRect.top + ResetThresholdY;

	if (ShouldReset)
	{
		MouseResetPending = TRUE;
		MouseResetTarget.x = (TempRect.left + TempRect.right) / 2;
		MouseResetTarget.y = (TempRect.top + TempRect.bottom) / 2;

		// Convert to screen coordinates before issuing the reset
		POINT MouseResetTargetScreenCoords = MouseResetTarget;
		::ClientToScreen(ViewportWnd, &MouseResetTargetScreenCoords);
		SetCursorPos(MouseResetTargetScreenCoords.x, MouseResetTargetScreenCoords.y);
		SetCursorPos(MouseResetTargetScreenCoords.x + 1, MouseResetTargetScreenCoords.y);
		SetCursorPos(MouseResetTargetScreenCoords.x, MouseResetTargetScreenCoords.y);

		POINT CurrentScreenCoords;
		GetCursorPos(&CurrentScreenCoords);
	}
}

void FWindowsWin32InputHandler::PollInputs(UWindowsViewport* Viewport)
{
	// Process Win32 cursor input using GetCursorPos
	GetCursorPos(&NewCursorPos);
	ScreenToClient(Viewport->Window->hWnd, &NewCursorPos);

	if (MouseResetPending)
	{
		CursorMoved = TRUE;
		MouseResetPending = FALSE;
		OldCursorPos = MouseResetTarget;
	}
	else if (NewCursorPos.x != OldCursorPos.x ||
		NewCursorPos.y != OldCursorPos.y)
	{
		CursorMoved = TRUE;
		if (GIsEditor)
			UpdateCumulativeMovement(Abs<LONG>(NewCursorPos.x - OldCursorPos.x) + Abs<LONG>(NewCursorPos.y - OldCursorPos.y));
	}

	if (Viewport->CapturingMouse)
		WarpMouseIfNecessary(Viewport, FALSE);

#if 0 // Process button clicks -- stijn: This is just here in case we want to
	  // switch from event-driven to poll-based click processing in the future
	for (INT i = 0; i < NumButtons; ++i)
	{
		ButtonStates[i].IsDown = 
			(GetAsyncKeyState(ButtonStates[i].ButtonStateSelectionMask)
			& ButtonStates[i].ButtonStateCompareMask) ? TRUE : FALSE;

		if (ButtonStates[i].IsDown != ButtonStates[i].WasDown)
			ButtonsClicked = TRUE;
	}
#endif
}

LRESULT FWindowsWin32InputHandler::ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	return ProcessCommonEvent(Viewport, iMessage, wParam, lParam);
}

void FWindowsWin32InputHandler::AcquireMouse(UWindowsViewport* Viewport)
{
	debugfSlow(NAME_DevInput, TEXT("Win32 Input: Acquired mouse for Viewport %ls"), *FObjectPathName(Viewport));
}

void FWindowsWin32InputHandler::ReleaseMouse(UWindowsViewport* Viewport)
{
	ResetMouseState();
	debugfSlow(NAME_DevInput, TEXT("Win32 Input: Released mouse from Viewport %ls"), *FObjectPathName(Viewport));
}

/*-----------------------------------------------------------------------------
	Win32 Legacy Cursor Input - This no longer works reliably since Win 10's
	Fall Creators Update
-----------------------------------------------------------------------------*/

UBOOL FWindowsLegacyWin32InputHandler::SetupInput()
{
	ButtonStates[NumButtons++] = ButtonState(VK_LBUTTON, 0x8000, IK_LeftMouse, MOUSE_Left, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_RBUTTON, 0x8000, IK_RightMouse, MOUSE_Right, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_MBUTTON, 0x8000, IK_MiddleMouse, MOUSE_Middle, TRUE);
	ButtonStates[NumButtons++] = ButtonState(VK_XBUTTON1, 0x8000, IK_MouseButton4, MOUSE_Button4, FALSE);
	ButtonStates[NumButtons++] = ButtonState(VK_XBUTTON2, 0x8000, IK_MouseButton5, MOUSE_Button5, FALSE);
	RelativeMouseMotion = TRUE;
	return TRUE;
}

void FWindowsLegacyWin32InputHandler::WarpMouse(UWindowsViewport* Viewport, BOOL FlushPendingInput)
{
	RECT TempRect;
	HWND ViewportWnd = Viewport->Window->hWnd;
	::GetClientRect(ViewportWnd, &TempRect);
	MouseResetTarget.x = (TempRect.left + TempRect.right) / 2;
	MouseResetTarget.y = (TempRect.top + TempRect.bottom) / 2;
	::ClientToScreen(ViewportWnd, &MouseResetTarget);
	
	// Jitter the mouse so Windows doesn't ignore our SetCursorPos call. See https://github.com/libsdl-org/SDL/commit/40ed9f75c9e1ed1dd99ee699ff4f678438ac3662
	SetCursorPos(MouseResetTarget.x, MouseResetTarget.y);
	SetCursorPos(MouseResetTarget.x + 1, MouseResetTarget.y);
	SetCursorPos(MouseResetTarget.x, MouseResetTarget.y);

	// Read the mouse position to force Windows to apply our changes
	if (GIsEditor)
	{
		POINT CurrentScreenCoords;
		GetCursorPos(&CurrentScreenCoords);
	}

	// Remember the time of this warp so we can flush all unprocessed inputs up to and including this warp
	if (FlushPendingInput)
	{
		MouseResetTime = GetTickCount();

		// Handle overflow
		if (MouseResetTime == 0)
			MouseResetTime = 1;
	}
}

void FWindowsLegacyWin32InputHandler::PollInputs(UWindowsViewport* Viewport)
{
	if (!Viewport->CapturingMouse)
	{
		// Process Win32 cursor input using GetCursorPos
		GetCursorPos(&NewCursorPos);
		ScreenToClient(Viewport->Window->hWnd, &NewCursorPos);
	}

	if (NewCursorPos.x != OldCursorPos.x ||
		NewCursorPos.y != OldCursorPos.y)
	{
		CursorMoved = TRUE;
		if (GIsEditor)
			UpdateCumulativeMovement(Abs<LONG>(NewCursorPos.x - OldCursorPos.x) + Abs<LONG>(NewCursorPos.y - OldCursorPos.y));
	}
}

LRESULT FWindowsLegacyWin32InputHandler::ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	const UBOOL Prev = PrevCaptureMouse;

	if (GIsEditor)
		PrevCaptureMouse = Viewport->CapturingMouse;

	if (iMessage == WM_MOUSEMOVE && Viewport->CapturingMouse)
	{
		// We ignore all pending WM_MOUSEMOVEs because they were delivered before we warped the mouse
		if (GIsEditor && !Prev)
		{
			WarpMouse(Viewport, TRUE);
			return 0;
		}

		// Flush pre-warp input
		if (MouseResetTime != 0)
		{
			if (GMessageTime <= MouseResetTime &&
				// check for GetTickCount() wraparound
				MouseResetTime - GMessageTime < 0x10000000)
			{
				// flush
				return 0;
			}

			MouseResetTime = 0;
		}
		
		// Get window rectangle.
		RECT TempRect;
		::GetClientRect(Viewport->Window->hWnd, &TempRect);

		// Get center of window.			
		POINT TempPoint, Base;
		TempPoint.x = (TempRect.left + TempRect.right) / 2;
		TempPoint.y = (TempRect.top + TempRect.bottom) / 2;
		Base = TempPoint;

		// Movement accumulators.
		LONG Cumulative = 0;

		// Grab all pending mouse movement.
		INT DX = 0, DY = 0;
		while (true)
		{
			POINTS Points = MAKEPOINTS(lParam);
			INT X = Points.x - Base.x;
			INT Y = Points.y - Base.y;
			Cumulative += Abs(X) + Abs(Y);
			DX += X;
			DY += Y;

			MSG Msg;
			if (PeekMessageW(&Msg, Viewport->Window->hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
			{
				lParam = Msg.lParam;
				wParam = Msg.wParam;
				Base.x = Points.x;
				Base.y = Points.y;
			}
			else
				break;
		}

		if (Cumulative > 0)
		{
			CursorMoved = TRUE;
			if (GIsEditor)
				UpdateCumulativeMovement(Cumulative);
		}

		if (DX || DY)
		{
			NewCursorPos.x += DX;
			NewCursorPos.y += DY;
			WarpMouse(Viewport, FALSE);
		}
		return 0;
	}
	else return ProcessCommonEvent(Viewport, iMessage, wParam, lParam);
}

void FWindowsLegacyWin32InputHandler::AcquireMouse(UWindowsViewport* Viewport)
{
	OldCursorPos = NewCursorPos = POINT{ 0,0 };
	RelativeMouseMotion = TRUE;

	// Warp and flush everything that came in just before the warp. We need this to prevent sudden camera jumps when clicking into a UEd viewport
	WarpMouse(Viewport, TRUE);

	debugfSlow(NAME_DevInput, TEXT("Win32 Input (Legacy): Acquired mouse for Viewport %ls"), *FObjectPathName(Viewport));
}

void FWindowsLegacyWin32InputHandler::ReleaseMouse(UWindowsViewport* Viewport)
{
	ResetMouseState();
	RelativeMouseMotion = FALSE;

	debugfSlow(NAME_DevInput, TEXT("Win32 Input (Legacy): Released mouse from Viewport %ls"), *FObjectPathName(Viewport));
}

/*-----------------------------------------------------------------------------
	DirectInput.
-----------------------------------------------------------------------------*/

IDirectInput8* FWindowsDirectInputHandler::di = nullptr;
FWindowsDirectInputHandler::DI_CREATE_FUNC FWindowsDirectInputHandler::diCreateFunc = nullptr;

UBOOL FWindowsDirectInputHandler::SetupInput()
{
	// Init DirectInput
	HINSTANCE Instance = LoadLibraryW(TEXT("dinput8.dll"));
	if (Instance == NULL)
	{
		debugf(NAME_DevInput, TEXT("DirectInput not installed"));
		return FALSE;
	}

	diCreateFunc = (DI_CREATE_FUNC)GetProcAddress(Instance, "DirectInput8Create"); //!!W version for UNICODE?
	if (!diCreateFunc)
	{
		debugf(NAME_DevInput, TEXT("DirectInput GetProcAddress failed"));
		ShutdownInput();
		return FALSE;
	}

	HRESULT Result = (*diCreateFunc)(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&di), NULL);
	if (Result != DI_OK)
	{
		debugf(NAME_DevInput, TEXT("DirectInput created failed: %s"), diError(Result));
		ShutdownInput();
		return FALSE;
	}

	debugf(NAME_Init, TEXT("DirectInput initialized successfully."));

	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON0, 0x80, IK_LeftMouse, MOUSE_Left, TRUE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON1, 0x80, IK_RightMouse, MOUSE_Right, TRUE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON2, 0x80, IK_MiddleMouse, MOUSE_Middle, TRUE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON3, 0x80, IK_MouseButton4, MOUSE_Button4, FALSE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON4, 0x80, IK_MouseButton5, MOUSE_Button5, FALSE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON5, 0x80, IK_MouseButton6, MOUSE_None, FALSE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON6, 0x80, IK_MouseButton7, MOUSE_None, FALSE);
	ButtonStates[NumButtons++] = ButtonState(DIMOFS_BUTTON7, 0x80, IK_MouseButton8, MOUSE_None, FALSE);
	RelativeMouseMotion = FALSE;
	return TRUE;
}

void FWindowsDirectInputHandler::ShutdownInput()
{
	if (di)
	{
		di->Release();
		di = nullptr;
	}
}

void FWindowsDirectInputHandler::RegisterViewport(UWindowsViewport* Viewport, UBOOL StartCaptured)
{
	// Init DirectInput Keyboard/Mouse for this viewport
	if( di )
	{
		if (diMouse)
		{
			diMouse->Unacquire();
			diMouse->Release();
			diMouse = nullptr;
			if (diMouseBuffer)
				appFree(diMouseBuffer);
		}

		HRESULT Result = di->CreateDevice(GUID_SysMouse, &diMouse, NULL);
		if (Result == DI_OK)
		{
			diMouseBuffer = (DIDEVICEOBJECTDATA*)appMalloc(100 * sizeof(DIDEVICEOBJECTDATA), TEXT("DirectInputMouseBuffer"));
			Result = diMouse->SetDataFormat(&c_dfDIMouse2);
		}

		if (Result == DI_OK)
			Result = diMouse->SetCooperativeLevel(Viewport->Window->hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

		if (Result == DI_OK)
		{
			DIPROPDWORD dipdw;
			dipdw.diph.dwSize = sizeof(DIPROPDWORD);
			dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			dipdw.diph.dwObj = 0;
			dipdw.diph.dwHow = DIPH_DEVICE;
			dipdw.dwData = DIPROPAXISMODE_ABS;
			Result = diMouse->SetProperty(DIPROP_AXISMODE, &dipdw.diph);
			if (Result == DI_OK)
			{
				dipdw.diph.dwSize = sizeof(DIPROPDWORD);
				dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
				dipdw.diph.dwObj = 0;
				dipdw.diph.dwHow = DIPH_DEVICE;
				dipdw.dwData = 100;	// buffer size of 100 mouse entries
				Result = diMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

				diMouse->Acquire();
				diMouse->GetDeviceState(sizeof(diMouseState), &diMouseState);
				PollInputs(Viewport);
				ResetMouseState();
				if (!StartCaptured)
					diMouse->Unacquire();
			}
		}

		if (Result != DI_OK)
		{
			debugf(NAME_DevInput, TEXT("DirectInput: failed to gain access to mouse: %ls"), diError(Result));
			if (diMouse)
			{
				diMouse->Unacquire();
				diMouse->Release();
				diMouse = nullptr;
				if (diMouseBuffer)
					appFree(diMouseBuffer);
			}
			ShutdownInput();
		}
	} 
}

void FWindowsDirectInputHandler::AcquireMouse(UWindowsViewport* Viewport)
{
	// Skip this if we're already capturing. When we're in windowed mode, we get AcquireMouse calls whenever we start dragging...
	if (Viewport && Viewport->CapturingMouse)
		return;

	if (diMouse)
	{
		diMouse->Acquire();
		debugf(NAME_DevInput, TEXT("DirectInput: Acquired mouse for Viewport %ls"), *FObjectPathName(Viewport));
	}
	else
	{
		debugf(NAME_DevInput, TEXT("DirectInput: Failed to acquire mouse for Viewport %ls"), *FObjectPathName(Viewport));
	}

	OldCursorPos = NewCursorPos = SavedCursor;
}

void FWindowsDirectInputHandler::ReleaseMouse(UWindowsViewport* Viewport)
{
	if (Viewport && !Viewport->CapturingMouse)
		return;

	SavedCursor = NewCursorPos;

	if (diMouse)
	{
		diMouse->Unacquire();
		debugf(NAME_DevInput, TEXT("DirectInput: Released mouse from Viewport %ls"), *FObjectPathName(Viewport));
	}
	else
	{
		debugf(NAME_DevInput, TEXT("DirectInput: Failed to release mouse from Viewport %ls"), *FObjectPathName(Viewport));
	}

	ResetMouseState();
}

void FWindowsDirectInputHandler::PollInputs(UWindowsViewport* Viewport)
{
	if (!diMouse || !Viewport->CapturingMouse)
	{
		::GetCursorPos(&NewCursorPos);
		if (NewCursorPos.x != OldCursorPos.x || NewCursorPos.y != OldCursorPos.y)
		{
			CursorMoved = TRUE;
			if (GIsEditor)
				UpdateCumulativeMovement(Abs<LONG>(NewCursorPos.x - OldCursorPos.x) + Abs<LONG>(NewCursorPos.y - OldCursorPos.y));
		}
		return;
	}

	HRESULT Result = diMouse->GetDeviceState(sizeof(diMouseState), &diMouseState);
	if (Result == DIERR_INPUTLOST)
	{
		diMouse->Acquire();
		Result = diMouse->GetDeviceState(sizeof(diMouseState), &diMouseState);
	}

	if (!diInitialized)
	{
		diInitialized = TRUE;
		OldCursorPos.x = diMouseState.lX;
		OldCursorPos.y = diMouseState.lY;
		OldWheelPos = diMouseState.lZ;
	}

	LONG DX = diMouseState.lX - OldCursorPos.x;
	LONG DY = diMouseState.lY - OldCursorPos.y;
	LONG DZ = diMouseState.lZ - OldWheelPos;

	NewCursorPos.x = diMouseState.lX;
	NewCursorPos.y = diMouseState.lY;
	NewWheelPos = diMouseState.lZ;

	if (DX || DY)
	{
		CursorMoved = TRUE;
		if (GIsEditor)
			UpdateCumulativeMovement(Abs<LONG>(DX) + Abs<LONG>(DY));
	}
	if (DZ)	WheelMoved = TRUE;

	// Buffered mouse clicks
	DWORD dwItems = 100;
	Result = diMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), diMouseBuffer, &dwItems, 0);

	for (INT i = 0; i < (INT)dwItems; i++)
	{
		for (INT j = 0; j < NumButtons; ++j)
		{
			if (diMouseBuffer[i].dwOfs == ButtonStates[j].ButtonStateSelectionMask)
				ButtonStates[j].IsDown = diMouseBuffer[i].dwData & ButtonStates[j].ButtonStateCompareMask;
		}
	}

	for (INT j = 0; j < NumButtons; ++j)
	{
		if (ButtonStates[j].IsDown != ButtonStates[j].WasDown)
		{
			if (GIsEditor && ButtonStates[j].IsDown)
			{
				ButtonStates[j].CumulativeMovementSinceLastClick = 0;
				ButtonStates[j].LastClickTime = 0.f;
			}
			ButtonsClicked = TRUE;
		}
	}
}

LRESULT FWindowsDirectInputHandler::ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	// Ignore windows mouse messages while capture is active
	if (Viewport->CapturingMouse)
		return 0;
	return ProcessCommonEvent(Viewport, iMessage, wParam, lParam);
}

/*-----------------------------------------------------------------------------
	RawInput.
-----------------------------------------------------------------------------*/

UBOOL FWindowsRawInputHandler::SetupInput()
{
	ButtonStates[NumButtons++] = ButtonState(RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_LEFT_BUTTON_DOWN, IK_LeftMouse, MOUSE_Left, TRUE);
	ButtonStates[NumButtons++] = ButtonState(RI_MOUSE_RIGHT_BUTTON_DOWN | RI_MOUSE_RIGHT_BUTTON_UP, RI_MOUSE_RIGHT_BUTTON_DOWN, IK_RightMouse, MOUSE_Right, TRUE);
	ButtonStates[NumButtons++] = ButtonState(RI_MOUSE_MIDDLE_BUTTON_DOWN | RI_MOUSE_MIDDLE_BUTTON_UP, RI_MOUSE_MIDDLE_BUTTON_DOWN, IK_MiddleMouse, MOUSE_Middle, TRUE);
	ButtonStates[NumButtons++] = ButtonState(RI_MOUSE_BUTTON_4_DOWN | RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_4_DOWN, IK_MouseButton4, MOUSE_Button4, FALSE);
	ButtonStates[NumButtons++] = ButtonState(RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_5_UP, RI_MOUSE_BUTTON_5_DOWN, IK_MouseButton5, MOUSE_Button5, FALSE);
	ResetMouseState();
	RelativeMouseMotion = FALSE;
	return TRUE;
}

void FWindowsRawInputHandler::ShutdownInput()
{
	ReleaseMouse(nullptr);
}

void FWindowsRawInputHandler::AcquireMouse(UWindowsViewport* Viewport)
{
	if (Viewport && Viewport->CapturingMouse)
		return;

	RawInputDevices[0].usUsagePage = 0x01;
	RawInputDevices[0].usUsage = 0x02;
	//
	// stijn: RIDEV_CAPTUREMOUSE has some weird side effects. If you release the mouse from a
	// window that was capturing the mouse _with_ RIDEV_CAPTUREMOUSE, then that window will
	// retain the mouse focus until you use the same mouse button that triggered the initial
	// capture to click on another window. This might be a Windows bug! 
	//
	RawInputDevices[0].dwFlags = /*(GIsEditor ? 0 : RIDEV_CAPTUREMOUSE) |*/ RIDEV_NOLEGACY;
	RawInputDevices[0].hwndTarget = (HWND)Viewport->GetWindow();

	// Register.
	if (!RegisterRawInputDevices(RawInputDevices, NUM_RAW_DEVICES, sizeof(RAWINPUTDEVICE)))
	{
		INT Error = GetLastError();
		GWarn->Logf(NAME_DevInput, TEXT("Failed to register RawInput Devices %ls (%i)"), appGetSystemErrorMessage(Error), Error);
		return;
	}

	debugfSlow(NAME_DevInput, TEXT("RawInput: Acquired mouse for Viewport %ls"), *FObjectPathName(Viewport));
	OldCursorPos = NewCursorPos = POINT{ 0,0 };
	RelativeMouseMotion = TRUE;
}

void FWindowsRawInputHandler::ReleaseMouse(UWindowsViewport* Viewport)
{
	if (Viewport && !Viewport->CapturingMouse)
		return;

	RawInputDevices[0].usUsagePage = 0x01;
	RawInputDevices[0].usUsage = 0x02;
	RawInputDevices[0].dwFlags = RIDEV_REMOVE;
	RawInputDevices[0].hwndTarget = 0;

	// Register.
	if (!RegisterRawInputDevices(RawInputDevices, NUM_RAW_DEVICES, sizeof(RAWINPUTDEVICE)))
	{
		INT Error = GetLastError();
		GWarn->Logf(NAME_DevInput, TEXT("Failed to register RawInput Devices %ls (%i)"), appGetSystemErrorMessage(Error), Error);
	}

	debugfSlow(NAME_DevInput, TEXT("RawInput: Released mouse from Viewport %ls"), *FObjectPathName(Viewport));
	ResetMouseState();
	RelativeMouseMotion = FALSE;
}

void FWindowsRawInputHandler::RegisterViewport(UWindowsViewport* Viewport, UBOOL StartCaptured)
{
	if (StartCaptured)
		AcquireMouse(Viewport);
	else
		ReleaseMouse(Viewport);
}

void FWindowsRawInputHandler::PollInputs(UWindowsViewport* Viewport)
{
	// We don't receive WM_INPUT while not capturing, so make sure we
	// tell base input handler when the mouse moves so it can deliver
	// MousePosition updates!
	if (!Viewport->CapturingMouse)
	{
		::GetCursorPos(&NewCursorPos);
		if (NewCursorPos.x != OldCursorPos.x || NewCursorPos.y != OldCursorPos.y)
		{
			CursorMoved = TRUE;
			if (GIsEditor)
				UpdateCumulativeMovement(Abs<LONG>(NewCursorPos.x - OldCursorPos.x) + Abs<LONG>(NewCursorPos.y - OldCursorPos.y));
		}
	}
}

LRESULT FWindowsRawInputHandler::ProcessInputEvent(UWindowsViewport* Viewport, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if (iMessage != WM_INPUT)
	{
		if (Viewport->CapturingMouse)
			return 0;
		return ProcessCommonEvent(Viewport, iMessage, wParam, lParam);
	}

	// Query size of raw input data to be read.
	UINT RawDataSize;
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &RawDataSize, sizeof(RAWINPUTHEADER)) == INDEX_NONE)
	{
		debugf(NAME_DevInput, TEXT("GetRawInputData failed to retrieve size."));
		return 0;
	}

	// Read raw input data.
	RAWINPUT* RawData = (RAWINPUT*)appAlloca(RawDataSize);
	check(RawData);
	INT Read = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, RawData, &RawDataSize, sizeof(RAWINPUTHEADER));
	if (Read != (INT)RawDataSize)
	{
		debugf(NAME_DevInput, TEXT("GetRawInputData failed to retrieve data (Read=%i,RawDataSize=%i)."), Read, RawDataSize);
		return 0;
	}

	MouseButtons = RawData->data.mouse.ulButtons;
	MouseButtonData = RawData->data.mouse.usButtonData;
	NewCursorPos.x += RawData->data.mouse.lLastX;
	NewCursorPos.y += RawData->data.mouse.lLastY;

	if (0) // For debug
	debugf(TEXT("WM_INPUT: %x %x, %x %x %p %x, %x %x %x %x, %x %d %d %x"), wParam, lParam, 
		RawData->header.dwType, RawData->header.dwSize, RawData->header.hDevice, RawData->header.wParam,
		RawData->data.mouse.usFlags, RawData->data.mouse.ulButtons, RawData->data.mouse.usButtonFlags, RawData->data.mouse.usButtonData,
		RawData->data.mouse.ulRawButtons, RawData->data.mouse.lLastX, RawData->data.mouse.lLastY, RawData->data.mouse.ulExtraInformation);

	// Process buffered clicks
	for (INT i = 0; i < NumButtons; ++i)
	{
		if (MouseButtons & ButtonStates[i].ButtonStateSelectionMask)
		{
			ButtonStates[i].IsDown = (MouseButtons & ButtonStates[i].ButtonStateCompareMask) ? TRUE : FALSE;
			ButtonsClicked = TRUE;
		}

		if (ButtonStates[i].IsDown != ButtonStates[i].WasDown)
		{
			if (GIsEditor && ButtonStates[i].IsDown)
			{
				ButtonStates[i].CumulativeMovementSinceLastClick = 0;
				ButtonStates[i].LastClickTime = 0.f;
			}
			ButtonsClicked = TRUE;
		}
	}

	if (NewCursorPos.x != OldCursorPos.x || NewCursorPos.y != OldCursorPos.y)
	{
		CursorMoved = TRUE;
		if (GIsEditor)
			UpdateCumulativeMovement(Abs<LONG>(NewCursorPos.x - OldCursorPos.x) + Abs<LONG>(NewCursorPos.y - OldCursorPos.y));
	}

	// Process buffered mouse wheel status
	if ((MouseButtons & RI_MOUSE_WHEEL) && MouseButtonData != 0)
	{
		NewWheelPos = MouseButtonData;
		WheelMoved = TRUE;
	}

	// Reset state
	MouseButtons = 0;
	MouseButtonData = 0;
	return 0;
}