#include "tchar.h"
#include "windows.h"
#include "windowsx.h"
#include "SysUtilsWin32.h"
#include "GlobalInput.h"

static HWND MainWindow;
static int ScreenWidth, ScreenHeight;
static int PixelSize = 1;
static int PixelFormat = 1;

static const unsigned long Palette16[] =
{
  0x00999999,
  0x00FFFFFF,
  0x00000000,
  0x005577AA,
  0x00676767,
  0x00AA0000,
  0x0012C036,
  0x00FFFF7B,
  0x003E31A2,
  0x007ABFC7,
  0x0068A941,
  0x00ACEA88,
  0x008C3E34,
  0x00FF3E09,
  0x00F7E0BA,
  0x008D47B3,
  0x00FFFFFF,
};

static const unsigned long Palette2[] =
{
  0x00000000,
  0x00FFFFFF,
  0x00FFFFFF,
};

static unsigned long *Palette = (unsigned long*)Palette2;
static int CountColors = 2;

void UpdateMessages(void)
{
  static MSG Msg;
  while (PeekMessage(&Msg, 0, 0, 0, PM_NOREMOVE) != 0)
  {
    if (GetMessage(&Msg, 0, 0, 0))
    {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
  }
}

void SysDrawBuffer(unsigned char *p)
{ 
  BITMAPINFO *BitsInfo;
  HDC DC;
  int i, j;

  DC = GetDC(MainWindow);

  BitsInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + 12 + 256 * sizeof(RGBQUAD));
  BitsInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BitsInfo->bmiHeader.biHeight = -ScreenHeight;// Image.height;
  BitsInfo->bmiHeader.biWidth = ScreenWidth;// Image.width;
  BitsInfo->bmiHeader.biPlanes = 1;
  BitsInfo->bmiHeader.biCompression = 0;
  if(PixelFormat == 4)
  {
    BitsInfo->bmiHeader.biBitCount = 4;
    //BitsInfo->bmiHeader.biSizeImage = ScreenHeight * ((ScreenHeight + 1) / 2);
  }
  if(PixelFormat == 1)
  {
    BitsInfo->bmiHeader.biBitCount = 1;
    //BitsInfo->bmiHeader.biSizeImage = ScreenHeight * ((ScreenHeight + 7) / 8);
  }

  for(i = 0; i < CountColors; i++)
  {
    j = Palette[i];
    BitsInfo->bmiColors[i].rgbRed   = (Palette[i] >> 16) & 0xFF;
    BitsInfo->bmiColors[i].rgbGreen = (Palette[i] >> 8) & 0xFF;
    BitsInfo->bmiColors[i].rgbBlue  = (Palette[i] >> 0) & 0xFF;
  }

  StretchDIBits(DC,
    0, 0, ScreenWidth * PixelSize, ScreenHeight * PixelSize,
    0, 0, ScreenWidth, ScreenHeight,
    p,
    BitsInfo,
    DIB_RGB_COLORS,
    SRCCOPY);

  ReleaseDC(MainWindow, DC);

  free(BitsInfo);

  UpdateMessages();
}


void SysSleep(int MS)
{
  long time;

  time = SysGetTime();

  do
  {
    Sleep(0);
    UpdateMessages();
  }
  while(SysGetTime() < time + MS);
  //Sleep(MS);
}

long SysGetTime(void)
{
  return GetTickCount();
}

void SysSetPixelFormat(int pf)
{
  PixelFormat = pf;
  if(PixelFormat == 1)
  {
    Palette = (unsigned long*)Palette2;
    CountColors = 2;
  }
  else
  if(PixelFormat == 4)
  {
    Palette = (unsigned long*)Palette16;
    CountColors = 16;
  }
}

int SysGetPixelFormat(void)
{
  return PixelFormat;
}

// ---

static HWND CreateWin(void (*WindowProc), unsigned int Width, unsigned int Height, unsigned long Flags)
{
  int ClientWidth, ClientHeight, Multiple;
  RECT Rect = {0, 0};
  HWND Wnd;
  WNDCLASS wndclass;

  Multiple = PixelSize = (Flags & 0x00000003) + 1;
  ClientWidth = Multiple * Width;
  ClientHeight = Multiple * Height;

  Rect.right = ClientWidth;
  Rect.bottom = ClientHeight;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc   = WindowProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = 0;
  wndclass.hIcon         = LoadIcon(0, L"idi_Application");
  wndclass.hCursor       = LoadCursor (0, IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wndclass.lpszMenuName  = L"";
  wndclass.lpszClassName = L"MainWindow";
  
  RegisterClass(&wndclass);

  Wnd = CreateWindow( 
    L"MainWindow",
    L"-",
    /*WS_OVERLAPPED WS_SYSMENU*/WS_POPUPWINDOW, // style
    CW_USEDEFAULT,// x
    CW_USEDEFAULT,// y
    ClientWidth,// width
    ClientHeight,// height
    NULL,// parent win
    NULL,// menu
    GetModuleHandle(NULL),
    NULL// other
    );
    
  GetWindowRect(Wnd, &Rect);
  Rect.bottom = Rect.left + ClientHeight;// ClientHeight
  Rect.right = Rect.top + ClientWidth;// ClientWidth
  AdjustWindowRect(&Rect, GetWindowLong(Wnd, GWL_STYLE) ,0);
    
  SetWindowPos(Wnd, 0, Rect.left, Rect.top,
    Rect.right - Rect.left,
    Rect.bottom - Rect.top, 0);
    
  ShowWindow(Wnd, SW_SHOW);
  UpdateWindow(Wnd); 

  return Wnd;
}

static LONG WINAPI WndProc(
  HWND    hWnd,
  UINT    uMsg,
  WPARAM  wParam,
  LPARAM  lParam)
{
  static int MoveWin = 0;
  static POINT MouseInWin;
  static RECT WinRect;
  LONG lRet = 1;
  POINT point;

  switch (uMsg) 
  {
    case WM_RBUTTONDOWN:
      MoveWin = 1;
      GetWindowRect(hWnd, &WinRect);
	  
      point.x = GET_X_LPARAM(lParam);
      point.y = GET_Y_LPARAM(lParam);
      ClientToScreen(hWnd, (LPPOINT)&point);
      
      MouseInWin.x = point.x - WinRect.left;
      MouseInWin.y = point.y - WinRect.top;
      
      SetCapture(hWnd);
      break;

    case WM_MOUSEMOVE:
      GetCursorPos((LPPOINT)&point);
      if(MoveWin)SetWindowPos(hWnd, 0,
        point.x - MouseInWin.x, point.y - MouseInWin.y,
        WinRect.right - WinRect.left, WinRect.bottom - WinRect.top,
        0);

      SetMousePos(GET_X_LPARAM(lParam) / PixelSize, GET_Y_LPARAM(lParam) / PixelSize);
      break;

    case WM_LBUTTONDOWN:
      SetCapture(hWnd);
      SetMouseButton(1);
      break;

    case WM_LBUTTONUP:
      SetMouseButton(0);
      ReleaseCapture();
      break;

    case WM_RBUTTONUP:
      MoveWin = 0;
      ReleaseCapture();
      break;

    case WM_KEYDOWN://WM_MBUTTONUP:
      if(wParam == VK_ESCAPE)
        PostMessage(hWnd, WM_DESTROY, 0, 0);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      ExitProcess(0);
      break;

    default:
      lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
      break;
  }
  
  return lRet;
}

// ---

int SysCreateScreen(unsigned int Width, unsigned int Height, int PF, unsigned long Flags)
{
  ScreenWidth = Width;
  ScreenHeight = Height;
  MainWindow = CreateWin(&WndProc, Width, Height, Flags);
  SysSetPixelFormat(PF);
  return 0;
}

void SysDestroyScreen(void)
{
  PostMessage(MainWindow, WM_DESTROY, 0, 0);
}

void SysProcessMessages(void)
{
  UpdateMessages();
}

int SysGetScreenWidth(void)
{
  return ScreenWidth;
}

int SysGetScreenHeight(void)
{
  return ScreenHeight;
}

unsigned long* SysGetPalette(void)
{
  return Palette;
}