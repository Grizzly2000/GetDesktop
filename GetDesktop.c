#include <windows.h>
#include <stdio.h>
#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")
using namespace Gdiplus;

#define SCREEN_MS 5000

typedef struct sJPG
{
	BYTE * JPG;
	unsigned int SizeOfJPG;
}JPG;

/*get the CLSID encoded*/
int Get_CLSID(WCHAR *format, CLSID *pClsid)
{
	unsigned int N = 0;
	unsigned int Size = 0;
	GetImageEncodersSize(&N, &Size);
	if (!Size)
	{
		return -1;
	}
	ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *)(malloc(Size));

	if (!pImageCodecInfo)
	{
		return -1;
	}
	GetImageEncoders(N, Size, pImageCodecInfo);
	for (unsigned int j = 0; j < N; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

/* Return JPG structure in memory. Using GDIPLUS.*/
JPG  GetScreeny(ULONG uQuality)
{
	IStream *pStream = NULL;
	LARGE_INTEGER liZero = {};
	ULARGE_INTEGER pos = {};
	STATSTG stg = {};
	ULONG bytesRead = 0;
	HRESULT hrRet = S_OK;
	BYTE* buffer = NULL;
	DWORD dwBufferSize = 0;
	CLSID imageCLSID;
	/* Initialisation of JPG structure */
	JPG JPG_Screen;
	JPG_Screen.JPG = NULL;
	JPG_Screen.SizeOfJPG = 0;

	/* Initialisation of GDI token*/
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	/* Get monitor sreen size*/
	HDC hdcScreen = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	HDC hdcCapture = CreateCompatibleDC(hdcScreen);
	int nWidth = GetDeviceCaps(hdcScreen, HORZRES),
		nHeight = GetDeviceCaps(hdcScreen, VERTRES),
		nBPP = GetDeviceCaps(hdcScreen, BITSPIXEL);


	/* Store Bitmap */
	LPBYTE lpCapture;
	BITMAPINFO bmiCapture = { { sizeof(BITMAPINFOHEADER), nWidth, -nHeight, 1, nBPP, BI_RGB, 0, 0, 0, 0, 0, } };
	HBITMAP hbmCapture = CreateDIBSection(hdcScreen, &bmiCapture, DIB_PAL_COLORS, (LPVOID *)&lpCapture, NULL, 0);

	/* If screenshot failed, Return NULL JPG.*/
	if (!hbmCapture) {
		DeleteDC(hdcCapture);
		DeleteDC(hdcScreen);
		GdiplusShutdown(gdiplusToken);
		return JPG_Screen;
	}

	/* Select the specified device */
	int nCapture = SaveDC(hdcCapture);
	SelectObject(hdcCapture, hbmCapture);
	/* Take Screenshot */
	BitBlt(hdcCapture, 0, 0, nWidth, nHeight, hdcScreen, 0, 0, SRCCOPY);
	/* Clean everything*/
	RestoreDC(hdcCapture, nCapture);
	DeleteDC(hdcCapture);
	DeleteDC(hdcScreen);

	/* Encode BMP to JPG */
	Bitmap *pScreenShot = new Bitmap(hbmCapture, (HPALETTE)NULL);
	EncoderParameters encoderParams;
	encoderParams.Count = 1;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Guid = EncoderQuality;
	encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].Value = &uQuality;
	Get_CLSID(L"image/jpeg", &imageCLSID);

	/* Save BMP to memory stream */
	hrRet = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
	hrRet = pScreenShot->Save(pStream, &imageCLSID, &encoderParams) == 0 ? S_OK : E_FAIL;
	hrRet = pStream->Seek(liZero, STREAM_SEEK_SET, &pos);
	hrRet = pStream->Stat(&stg, STATFLAG_NONAME);

	/* Allocate Buffer for JPG */
	JPG_Screen.JPG = new BYTE[stg.cbSize.LowPart];
	hrRet = (JPG_Screen.JPG == NULL) ? E_OUTOFMEMORY : S_OK;
	JPG_Screen.SizeOfJPG = stg.cbSize.LowPart;

	/* Copy Stream to JPG Buffer */
	hrRet = pStream->Read(JPG_Screen.JPG, stg.cbSize.LowPart, &bytesRead);

	/*  Release BMP stream */
	if (pStream)
	{
		pStream->Release();
	}

	/* Free everything */
	delete pScreenShot;
	DeleteObject(hbmCapture);
	GdiplusShutdown(gdiplusToken);
	/* return JPG + size */
	return JPG_Screen;
}

int Get_Screenshot(ULONG uQuality)
{
	FILE * File = NULL;
	//JPG * JPGScreen;// = GetScreeny(100);

	while (1)
	{
		JPG JPGScreen = GetScreeny(uQuality);
		File = fopen("C:\\Temp\\desktop.jpg", "wb");
		fwrite(JPGScreen.JPG, JPGScreen.SizeOfJPG, 1, File);
		fclose(File);
		Sleep(SCREEN_MS);
	}
}

int main()
{
	Get_Screenshot(10);
	return 0;
}