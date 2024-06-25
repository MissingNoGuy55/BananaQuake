#include "quakedef.h"

#define NUMVERTEXNORMALS	162

refdef_t r_refdef;
viddef_t vid;

enum modestate_t modestate;
double r_avertexnormals[NUMVERTEXNORMALS][3];

HWND mainwindow = nullptr;
HINSTANCE global_instance = nullptr;
HINSTANCE render_instance = nullptr;
WNDPROC wndproc = {};
WNDCLASSEX wcx = {};
int vid_width = 0;
int vid_height = 0;
bool bWantsToRender = false;

bool block_drawing = false;
bool DDActive = false;
bool scr_disabled_for_loading = false;
bool scr_skipupdate = false;

HANDLE handlestdin = nullptr;
HANDLE handlestdout = nullptr;

vec3_t r_origin;
vec3_t vpn;
vec3_t vright;
vec3_t vup;

float scr_centertime_off = 0.0f;
float scr_con_current = 0.0f;
int scr_copyeverything = 0;
int scr_copytop = 0;
int scr_fullupdate = 0;

float xscaleshrink = 0.0f;
float yscaleshrink = 0.0f;

int clearnotify = 0;

int DIBWidth = 0, DIBHeight = 0;
int window_center_x = 0, window_center_y = 0;
int window_width = 0, window_height = 0;

constexpr size_t bitmapsize = sizeof(BITMAPINFOHEADER);

typedef struct resolution_types_s
{
	long double x;
	long double y;
} resolution_types_t;

resolution_types_t s_ValidResolutions[] = {

	{43.817804600413289076557582624064, 32.863353450309966807418186968048},		// Missi: 1920x1080 (1/17/2023)
	{40.000000000000000000000000000000, 30.000000000000000000000000000000},		// Missi: 1600x900 (1/17/2023)
	{30.983866769659335081434123198259, 23.237900077244501311075592398694},		// Missi: 960x540 (1/17/2023)
	{28.284271247461900976033774484194, 24.494897427831780981972840747059}		// Missi: 800x600 (1/17/2023)

};

static resolution_types_t default_resolution = s_ValidResolutions[2];
static resolution_types_t current_resolution = {};

using namespace D2D1;
using namespace DirectX;

namespace DisplayMetrics
{
	// High resolution displays can require a lot of GPU and battery power to render.
	// High resolution phones, for example, may suffer from poor battery life if
	// games attempt to render at 60 frames per second at full fidelity.
	// The decision to render at full fidelity across all platforms and form factors
	// should be deliberate.
	static const bool SupportHighResolutions = false;

	// The default thresholds that define a "high resolution" display. If the thresholds
	// are exceeded and SupportHighResolutions is false, the dimensions will be scaled
	// by 50%.
	static const float DpiThreshold = 192.0f;		// 200% of standard desktop display.
	static const float WidthThreshold = 1920.0f;	// 1080p width.
	static const float HeightThreshold = 1080.0f;	// 1080p height.
};

// Constants used to calculate screen rotations
namespace ScreenRotation
{
	// 0-degree Z-rotation
	static const XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 90-degree Z-rotation
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 180-degree Z-rotation
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 270-degree Z-rotation
	static const XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
};

CDXRenderer::CDXRenderer() : CCoreRenderer(),
dx_active(false),
m_constantBuffer(nullptr),
m_constantBufferData(),
m_degreesPerSecond(0.0f),
m_indexBuffer(nullptr),
m_indexCount(0),
m_inputLayout(nullptr),
m_loadingComplete(false),
m_pixelShader(nullptr),
m_tracking(false),
m_vertexBuffer(nullptr),
m_vertexShader(nullptr),
#ifdef _DEBUG
m_Debug(nullptr),
#endif
m_currentOrientation(0),
m_d2dContext(nullptr),
m_d3dContext(nullptr),
m_d2dDevice(nullptr),
m_d3dDevice(nullptr),
m_d2dRenderTargetSize(),
m_d2dRenderTargetView(nullptr),
m_d2dFactory(nullptr),
m_d2dTargetBitmap(nullptr),
m_d3dDepthStencilView(nullptr),
m_d3dFeatureLevel(D3D_FEATURE_LEVEL_11_1),
m_d3dRenderTargetSize(),
m_d3dRenderTargetView(nullptr),
m_deviceNotify(nullptr),
m_dpi(0.0f),
m_dwriteFactory(nullptr),
m_effectiveDpi(0.0f),
m_logicalSize(),
m_nativeOrientation(0),
m_orientationTransform3D(),
m_outputSize(),
m_screenViewport(),
m_swapChain(nullptr),
m_wicFactory(nullptr),
m_window(nullptr)
{
	pBitmapBrushes = {};
	pBitmaps = {};

	CreateDeviceResources();
}

static	IWICBitmapDecoder* pDecoder = nullptr;
static	IWICStream* pStream = nullptr;
static	IWICFormatConverter* pConverter = nullptr;
static	IWICBitmapScaler* pScaler = nullptr;

ID2D1Bitmap* CDXRenderer::LoadBitmapTexture(const wchar_t* texName, UINT32 sizeX, UINT32 sizeY, D2D1_BITMAP_PROPERTIES& rBitmapProperties,
	IWICImagingFactory* pWICImagingFactory, ID2D1Bitmap* renderBitmap, ID2D1HwndRenderTarget* pRenderTarget)
{
	HRESULT result = S_OK;
	IWICBitmapFrameDecode* pSource = nullptr;
	ID2D1BitmapBrush* pBitmapBrush = nullptr;

	D2D1_SIZE_U bitmapSize = { sizeX, sizeY };

	result = pWICImagingFactory->CreateDecoderFromFilename(
		texName,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	THROW_IF_FAIL(result);

	result = pRenderTarget->CreateBitmap(bitmapSize, rBitmapProperties, &renderBitmap);

	THROW_IF_FAIL(result);

	result = pRenderTarget->CreateBitmapBrush(renderBitmap, &pBitmapBrush);

	THROW_IF_FAIL(result);

	pBitmapBrushes.AddToEnd(pBitmapBrush);

	D2D1::Matrix3x2F matrixTransform = D2D1::Matrix3x2F::Identity();

	pBitmapBrush->SetTransform(matrixTransform);

	result = pDecoder->GetFrame(0, &pSource);

	THROW_IF_FAIL(result);

	result = pWICImagingFactory->CreateFormatConverter(&pConverter);

	THROW_IF_FAIL(result);

	result = pConverter->Initialize(
		pSource,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.f,
		WICBitmapPaletteTypeMedianCut
	);

	THROW_IF_FAIL(result);

	result = pRenderTarget->CreateBitmapFromWicBitmap(
		pConverter,
		NULL,
		&renderBitmap
	);

	THROW_IF_FAIL(result);

	pBitmaps.AddToEnd(renderBitmap);

	return renderBitmap;
}

HRESULT CALLBACK CDXRenderer::RenderLoop()
{
	const wchar_t* command = nullptr;
	MSG msg = {};
	HRESULT result = {};
	bool bInit = false;
	BOOL bRet = FALSE;
	bWantsToRender = true;

	IDWriteTextFormat* format = {};

	GetDWriteFactory()->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		64.0f,
		L"",
		&format
	);

	format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

	format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	while (bWantsToRender && (bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			exit(0);
		}
		else
		{
			if (mainwindow)
			{
				static ID2D1Bitmap* bitmap = GetD2DTargetBitmap();
				static ID3D11Device* device3D = GetD3DDevice();
				static ID3D11DeviceContext* context = GetD3DDeviceContext();
				static ID2D1DeviceContext* context2d = GetD2DDeviceContext();
				static ID2D1Factory* factory = GetD2DFactory();
				static IDXGISwapChain* swapchain = GetSwapChain();
				static ID2D1HwndRenderTarget* renderTarget = GetD2DRenderTarget();
				static IWICImagingFactory* wicImaging = GetWicImagingFactory();
				static ID3D11Texture2D* texture = nullptr;
				static IDXGISurface* surface = nullptr;
				static ID2D1Layer* layer = nullptr;

				D2D1_SIZE_F renderTargetSize = renderTarget->GetSize();

				D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

				D2D1_BITMAP_PROPERTIES bitmapProperties2 = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

				if (!bitmap)
				{
					bitmap = LoadBitmapTexture(L"pic\\front\\pidgeotto.bmp", 48, 48, bitmapProperties2,
						wicImaging, bitmap, renderTarget);

					if (bitmap)
					{
						D2D1_SIZE_F bitmapSize = bitmap->GetSize();
						bitmapSize.width = pow(bitmapSize.width, 2) / 16;
						bitmapSize.height = pow(bitmapSize.height, 2) / 16;

						ID2D1SolidColorBrush* pCornflowerBlueBrush = nullptr;
						ID2D1SolidColorBrush* pWhiteBrush = nullptr;

						result = renderTarget->CreateSolidColorBrush(
							D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
							&pCornflowerBlueBrush
						);

						THROW_IF_FAIL(result);

						result = renderTarget->CreateSolidColorBrush(
							D2D1::ColorF(D2D1::ColorF::White),
							&pWhiteBrush
						);

						THROW_IF_FAIL(result);

						RECT rc = {};
						GetWindowRect(mainwindow, &rc);

						static const wchar_t sc_helloWorld[] = L"Test!";

						// Missi: load texture here (1/16/2023)

						D2D1_BITMAP_BRUSH_PROPERTIES bmpBrushProperties = {};
						bmpBrushProperties.interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

						renderTarget->BeginDraw();

						renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

						renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::CornflowerBlue));

						if (!format)
						{
							Sys_Error(__FUNCTION__, __LINE__, __FILE__, TEXT("Failed to create DX format"));
							return 1;
						}

						renderTarget->DrawText(
							sc_helloWorld,
							ARRAYSIZE(sc_helloWorld) - 1,
							format,
							D2D1::RectF(0, 0, renderTargetSize.width, renderTargetSize.height),
							pWhiteBrush
						);

						D2D1_RECT_F rect = D2D1::RectF(0, 0, bitmapSize.width, bitmapSize.height);

						FLOAT dpiX = 0.0f, dpiY = 0.0f;

						auto pixelFormat = bitmap->GetPixelFormat();
						auto pixelSize = bitmap->GetPixelSize();
						bitmap->GetDpi(&dpiX, &dpiY);

						renderTarget->DrawBitmap(bitmap, rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

						result = renderTarget->EndDraw();

						THROW_IF_FAIL(result);
					}
				}
				else
				{

				}
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

// Determine the dimensions of the render target and whether it will be scaled down.
void CDXRenderer::UpdateRenderTargetSize()
{
	m_effectiveDpi = m_dpi;

	// To improve battery life on high resolution devices, render to a smaller render target
	// and allow the GPU to scale the output when it is presented.
	if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
	{
		float width = ConvertDipsToPixels((float)m_logicalSize.Width, m_dpi);
		float height = ConvertDipsToPixels((float)m_logicalSize.Height, m_dpi);

		// When the device is in portrait orientation, height > width. Compare the
		// larger dimension against the width threshold and the smaller dimension
		// against the height threshold.
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// To scale the app we change the effective DPI. Logical size does not change.
			m_effectiveDpi /= 2.0f;
		}
	}

	// Calculate the necessary render target size in pixels.
	m_outputSize.Width = (UINT)ConvertDipsToPixels((float)m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = (UINT)ConvertDipsToPixels((float)m_logicalSize.Height, m_effectiveDpi);

	// Prevent zero size DirectX content from being created.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}

// Recreate all device resources and set them back to the current state.
void CDXRenderer::HandleDeviceLost()
{
	m_swapChain = nullptr;

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceLost();
	}

	CreateDeviceResources();
	m_d2dContext->SetDpi(m_dpi, m_dpi);

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceRestored();
	}
}

// Configures the Direct3D device, and stores handles to it and the device context.
HRESULT CDXRenderer::CreateDeviceResources()
{
	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (SdkLayersAvailable())
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDWriteFactory* dwrite = nullptr;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,	// Create a device using the hardware graphics driver.
		0,							// Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
		creationFlags,				// Set debug and Direct2D compatibility flags.
		featureLevels,				// List of feature levels this app can support.
		ARRAYSIZE(featureLevels),	// Size of the list above.
		D3D11_SDK_VERSION,			// Always set this to D3D11_SDK_VERSION for Microsoft Store apps.
		&device,					// Returns the Direct3D device created.
		&m_d3dFeatureLevel,			// Returns feature level of device created.
		&context					// Returns the device immediate context.
	);

	if (FAILED(hr))
	{
		// If the initialization fails, fall back to the WARP device.
		// For more information on WARP, see: 
		// https://go.microsoft.com/fwlink/?LinkId=286690
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
			0,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&device,
			&m_d3dFeatureLevel,
			&context
		);

		THROW_IF_FAIL(hr);
	}

	// Store pointers to the Direct3D 11.3 API device and immediate context.
	hr = device->QueryInterface(__uuidof(ID3D11Device), (void**)&m_d3dDevice);

	THROW_IF_FAIL(hr);

	hr = context->QueryInterface(__uuidof(ID3D11DeviceContext), (void**)&m_d3dContext);

	THROW_IF_FAIL(hr);

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_dwriteFactory));

	THROW_IF_FAIL(hr);

	// Create the Direct2D device object and a corresponding context.
	IDXGIDevice* dxgiDevice = {};
	hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

	THROW_IF_FAIL(hr);

	if (dxgiDevice)
	{
		hr = D2D1CreateDevice(dxgiDevice, NULL, &m_d2dDevice);

		THROW_IF_FAIL(hr);
	}

	// Initialize Direct2D resources.
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
	HRESULT result = 0;

	// Initialize the Direct2D Factory.
	hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		&m_d2dFactory
	);

	THROW_IF_FAIL(hr);

	hr = m_d2dDevice->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		&m_d2dContext
	);

	THROW_IF_FAIL(hr);

	IDXGIAdapter* dxgiAdapter;
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);

	THROW_IF_FAIL(hr);

	IDXGIFactory2* dxgiFactory;
	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

	THROW_IF_FAIL(hr);

	dx_active = true;

	SetWindow(mainwindow);

	DisplaySize outputSize = GetOutputSize();
	float aspectRatio = (float)outputSize.Width / (float)outputSize.Height;
	float fovAngleY = 70.0f * DirectX::XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	DirectX::XMMATRIX perspectiveMatrix = DirectX::XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);

	DirectX::XMFLOAT4X4 orientation = GetOrientationTransform3D();

	DirectX::XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const DirectX::XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const DirectX::XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const DirectX::XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	UpdateRenderTargetSize();

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	DXGI_MODE_ROTATION displayRotation = DXGI_MODE_ROTATION_IDENTITY;

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d2dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d2dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	vid_width = m_d2dRenderTargetSize.Width;
	vid_height = m_d2dRenderTargetSize.Height;

	dxBuffer.AddMultipleToEnd(vid_width * vid_height * 3, (byte*)'\0');

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	DXGI_SCALING scaling = DXGI_SCALING_NONE;

	swapChainDesc.Width = lround(m_d2dRenderTargetSize.Width);		// Match the size of the window.
	swapChainDesc.Height = lround(m_d2dRenderTargetSize.Height);
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// This is the most common swap chain format.
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;								// Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;									// Use double-buffering to minimize latency.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// All Microsoft Store apps must use this SwapEffect.
	swapChainDesc.Flags = 0;
	swapChainDesc.Scaling = scaling;

	RECT rc;
	GetWindowRect(mainwindow, &rc);

	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory), (void**)&m_wicFactory);

	THROW_IF_FAIL(hr);

	D2D1_SIZE_U size = D2D1::SizeU(
		rc.right - rc.left,
		rc.bottom - rc.top
	);

	hr = m_d2dFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(mainwindow, size),
		&m_d2dRenderTargetView
	);

	THROW_IF_FAIL(hr);

	hr = dxgiFactory->CreateSwapChainForHwnd(
		m_d3dDevice,
		mainwindow,
		&swapChainDesc,
		nullptr,
		nullptr,
		&m_swapChain
	);

	THROW_IF_FAIL(hr);

#ifdef _DEBUG

	HINSTANCE hDLL;               // Handle to DLL
	LPFNDLLFUNC1 lpfnDllFunc1;    // Function pointer
	//const IID rrid;
	//UINT  uParam2;
	HRESULT uReturnVal;

	hDLL = LoadLibrary("DXGIDebug.dll");
	if (hDLL != NULL)
	{
		lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,
			"DXGIGetDebugInterface");
		if (!lpfnDllFunc1)
		{
			// handle the error
			FreeLibrary(hDLL);
			HRESULT result = GetLastError();
			Sys_Error(__func__, __LINE__, __FILE__, result);
		}
		else
		{
			// call the function
			uReturnVal = lpfnDllFunc1(__uuidof(IDXGIDebug), (void**)&m_Debug);

			if (uReturnVal != S_OK)
			{
				HRESULT result = GetLastError();
				Sys_Error(__func__, __LINE__, __FILE__, result);
			}
		}
	}
#endif

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = GetD3DDevice()->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = GetD3DDevice()->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pInputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	GetD3DDeviceContext()->IASetInputLayout(m_pInputLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = GetD3DDevice()->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	ShowWindow(mainwindow, 0);
}

void CDXRenderer::CreateDeviceDependentResources()
{
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d2dRenderTargetView = nullptr;
	m_d2dContext->SetTarget(nullptr);
	m_d2dTargetBitmap = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush();

	UpdateRenderTargetSize();

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	DXGI_MODE_ROTATION displayRotation = DXGI_MODE_ROTATION_IDENTITY;

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d2dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d2dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	vid_width = m_d2dRenderTargetSize.Width;
	vid_height = m_d2dRenderTargetSize.Height;

	dxBuffer.AddMultipleToEnd(vid_width * vid_height * 3, (byte*)'\0');

	if (m_swapChain != nullptr)
	{
		// If the swap chain already exists, resize it.
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // Double-buffered swap chain.
			lround(m_d2dRenderTargetSize.Width),
			lround(m_d2dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
		);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
		{
			THROW_IF_FAIL(hr);
		}
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SCALING scaling = DXGI_SCALING_NONE;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = { 0, 0, (DXGI_MODE_SCANLINE_ORDER)0, (DXGI_MODE_SCALING)0, 0 };
		IDXGISwapChain1* swapChain = NULL;
		HRESULT result = 0;

		swapChainDesc.Width = lround(m_d2dRenderTargetSize.Width);		// Match the size of the window.
		swapChainDesc.Height = lround(m_d2dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// This is the most common swap chain format.
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc.Count = 1;								// Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;									// Use double-buffering to minimize latency.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// All Microsoft Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = scaling;

		// This sequence obtains the DXGI factory that was used to create the Direct3D device above.
		IDXGIDevice3* dxgiDevice = NULL;
		m_d3dDevice->QueryInterface(_uuidof(IDXGIDevice3), (void**)&dxgiDevice);

		IDXGIAdapter* dxgiAdapter;
		dxgiDevice->GetAdapter(&dxgiAdapter);

		IDXGIFactory2* dxgiFactory;
		result = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

		RECT rc;
		GetWindowRect(mainwindow, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		result = m_d2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(mainwindow, size),
			&m_d2dRenderTargetView
		);

		THROW_IF_FAIL(result);

		result = dxgiFactory->CreateSwapChainForHwnd(
			m_d3dDevice,
			mainwindow,
			&swapChainDesc,
			nullptr,
			nullptr,
			&m_swapChain
		);

		THROW_IF_FAIL(result);

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		result = dxgiDevice->SetMaximumFrameLatency(1);

		THROW_IF_FAIL(result);
	}

	// Set the proper orientation for the swap chain, and generate 2D and
	// 3D matrix transformations for rendering to the rotated swap chain.
	// Note the rotation angle for the 2D and 3D transforms are different.
	// This is due to the difference in coordinate spaces.  Additionally,
	// the 3D matrix is specified explicitly to avoid rounding errors.

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform2D = Matrix3x2F::Identity();
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(90.0f) *
			Matrix3x2F::Translation((float)m_logicalSize.Height, 0.0f);
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(180.0f) *
			Matrix3x2F::Translation((float)m_logicalSize.Width, (float)m_logicalSize.Height);
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform2D =
			Matrix3x2F::Rotation(270.0f) *
			Matrix3x2F::Translation(0.0f, (float)m_logicalSize.Width);
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		return exit(1);
	}

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;

	// Create a depth stencil view for use with 3D rendering if needed.
	//D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	//depthStencilDesc.Width = m_d3dRenderTargetSize.Width;
	//depthStencilDesc.Height = m_d3dRenderTargetSize.Height;
	//depthStencilDesc.MipLevels = (UINT)1; // Use a single mipmap level.
	//depthStencilDesc.ArraySize = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	//depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//depthStencilDesc.SampleDesc = sampleDesc;
	//depthStencilDesc.SampleDesc.Quality = 0;
	//depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	//ID3D11Texture2D* depthStencil;
	//ThrowIfFailed(
	//	m_d3dDevice->CreateTexture2D(
	//		&depthStencilDesc,
	//		nullptr,
	//		&depthStencil
	//		)
	//	);

	//CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	//ThrowIfFailed(
	//	m_d3dDevice->CreateDepthStencilView(
	//		depthStencil,
	//		&depthStencilViewDesc,
	//		&m_d3dDepthStencilView
	//		)
	//	);

	// Set the 3D rendering viewport to target the entire window.
	/*m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_d3dRenderTargetSize.Width,
		m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);*/

	// Create a Direct2D target bitmap associated with the
	// swap chain back buffer and set it as the current target.
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
		);

	ID3D11Texture2D* dxgiBackBuffer;
	HRESULT result = m_swapChain->GetBuffer(0, __uuidof(dxgiBackBuffer), reinterpret_cast<void**>(&dxgiBackBuffer));

	THROW_IF_FAIL(result);

	D2D1_SIZE_U size;
	size.width = vid_width;
	size.height = vid_height;

	m_d2dContext->SetTarget(m_d2dTargetBitmap);
	m_d2dContext->SetDpi(m_effectiveDpi, m_effectiveDpi);

	// Grayscale text anti-aliasing is recommended for all Microsoft Store apps.
	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// This method is called when the CoreWindow is created (or re-created).
void CDXRenderer::SetWindow(HWND window)
{
	m_window = window;

	RECT lrect;
	GetWindowRect(window, &lrect);

	size_t width = lrect.right;
	size_t height = lrect.bottom;

	m_logicalSize.Width = (UINT)width;
	m_logicalSize.Height = (UINT)height;

	/*m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;*/
	m_dpi = (float)GetDpiForWindow(window);
	m_d2dContext->SetDpi(m_dpi, m_dpi);
}

void CDXRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource(
		m_constantBuffer,
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
	);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		&m_vertexBuffer,
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer,
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout);

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader,
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		&m_constantBuffer
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader,
		nullptr,
		0
	);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);
}

void CDXRenderer::ReleaseEverything()
{
	if (m_inputLayout)
		m_inputLayout->Release();

	if (m_vertexBuffer)
		m_vertexBuffer->Release();

	if (m_indexBuffer)
		m_indexBuffer->Release();

	if (m_vertexShader)
		m_vertexShader->Release();

	if (m_pixelShader)
		m_pixelShader->Release();

	if (m_constantBuffer)
		m_constantBuffer->Release();

}

LRESULT WINAPI Loop(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return 0;

	case WM_PAINT:
	{
		if (g_pDXRenderer)
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(mainwindow, &ps);
			EndPaint(mainwindow, &ps);
		}
	}
	return 0;

	case WM_SIZE:
		// Set the size and position of the window. 
		return 0;

	case WM_DESTROY:
		// Clean up window-specific data objects.
		delete g_pDXRenderer;
		g_pDXRenderer = nullptr;
		bWantsToRender = false;
		DestroyWindow(hWnd);
		UnregisterClass(wcx.lpszClassName, wcx.hInstance);
		mainwindow = nullptr;
		break;

	case WM_QUIT:
	{
		if (MessageBox(mainwindow, "Really quit?", "My application", MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(mainwindow);
		}
		break;
	}

	// 
	// Process other messages. 
	// 
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void VID_Init(unsigned char* palette)
{
	if (!g_pDXRenderer)
	{
		// Fill in the window class structure with parameters 
	// that describe the main window. 

		LPCWSTR icon = L"PokemonRGB_CPP.ico";
		HRESULT res = {};

		wcx.cbSize = sizeof(wcx);			// size of structure 
		wcx.style = CS_OWNDC;				// redraw if size changes 
		wcx.lpfnWndProc = Loop;				// points to window procedure 
		wcx.cbClsExtra = 0;					// no extra class memory 
		wcx.cbWndExtra = 0;					// no extra window memory 
		wcx.hInstance = global_instance;	// handle to instance 
		wcx.hIcon = static_cast<HICON>(LoadImage(global_instance,
			MAKEINTRESOURCE(1),
			IMAGE_ICON,
			32, 32,
			LR_DEFAULTCOLOR));

		wcx.hCursor = NULL;					// predefined arrow 
		wcx.hbrBackground = WHITE_BRUSH;	// white background brush 
		wcx.lpszMenuName = "Pokemon Red";	// name of menu resource 
		wcx.lpszClassName = "Pokemon Red";	// name of window class 
		wcx.hIconSm = NULL;					// small class icon 

		if (!RegisterClassEx(&wcx))
		{
			HRESULT lresult = GetLastError();
			THROW_IF_FAIL(lresult);
		}

		long double baseSqrSizeX = default_resolution.x;
		long double baseSqrSizeY = default_resolution.y;

		int scaledSqrSizeX = abs(pow(baseSqrSizeX, 2));
		int scaledSqrSizeY = abs(pow(baseSqrSizeY, 2));

		mainwindow = CreateWindowEx(
			0,
			wcx.lpszMenuName,			// Missi: was WinQuake (12/1/2022)
			wcx.lpszClassName,			// Missi: was GLQuake (12/1/2022)
			WS_OVERLAPPEDWINDOW,
			0, 0,
			scaledSqrSizeX,
			scaledSqrSizeY,
			NULL,
			NULL,
			wcx.hInstance,
			NULL);

		if (!mainwindow)
		{
			HRESULT result = GetLastError();
			THROW_IF_FAIL(result);
		}

		current_resolution = { baseSqrSizeX, baseSqrSizeY };

		g_pDXRenderer = new CDXRenderer;

		if (!g_pDXRenderer)
			Sys_Error(__FUNCTION__, __LINE__ - 3, __FILE__, TEXT("Failed to create DirectX renderer"));

		UpdateWindow(mainwindow);
	}
}

void SCR_CenterPrint(const char* msg)
{

}

void SCR_BeginLoadingPlaque()
{

}

void SCR_EndLoadingPlaque()
{

}

void SCR_UpdateScreen()
{

}

void SCR_Init()
{

}

int SCR_ModalMessage(const char* msg)
{
	return 0;
}

void VID_HandlePause(bool bPause)
{

}

int VID_ForceUnlockedAndReturnState()
{
	return 0;
}

void VID_UnlockBuffer() {};
void VID_Shutdown() {};
void VID_ShiftPalette(unsigned char* palette) {};
void VID_ForceLockState(int state) {};
void VID_SetDefaultMode() {};
void VID_LockBuffer() {};

unsigned CCoreRenderer::blocklights[BLOCK_WIDTH * BLOCK_HEIGHT * 3];

CCoreRenderer::CCoreRenderer() : solidskytexture(NULL)
{
	solidskytexture = NULL;
	alphaskytexture = NULL;
	speedscale = 1.0f;
	memset(blocklights, 0, sizeof(blocklights));
}

CCoreRenderer::CCoreRenderer(const CCoreRenderer& src)
{
	solidskytexture = NULL;
	alphaskytexture = NULL;
	speedscale = 1.0f;
	memset(blocklights, 0, sizeof(blocklights));
}

CDXTexture::CDXTexture() :
	checksum(0),
	flags(0),
	height(0),
	identifier(),
	mipmap(false),
	next(nullptr),
	owner(nullptr),
	source_format(SRC_NONE),
	source_height(0),
	source_offset(0),
	source_width(0),
	texnum(0),
	width(0)
{
}

CDXTexture::CDXTexture(CQuakePic qpic, float sl, float tl, float sh, float th) :
	checksum(0),
	flags(0),
	height(0),
	identifier(),
	mipmap(false),
	next(nullptr),
	owner(nullptr),
	source_format(SRC_NONE),
	source_height(0),
	source_offset(0),
	source_width(0),
	texnum(0),
	width(0)
{
}

CDXTexture::~CDXTexture()
{
}
