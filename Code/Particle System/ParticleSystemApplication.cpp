//-----------------------------------------------------------------------------
// PARTICLE SYSTEM EXAMPLE

//-----------------------------------------------------------------------------
// Include these files

#define _CRT_RAND_S	// Define this before any includes to select correct rand function in stdlib.h.

#include <Windows.h>	// Windows library (for window functions, menus, dialog boxes, etc)
#include "ParticleSystem.h"
#include <string>
#include "PerlinNoise.h"

//---------------------------------------------------------------------------------------------------------------------------------
// Global variables

LPDIRECT3D9             d3d		= NULL;	// Used to create the device
LPD3DXMESH g_BoxMesh = NULL;						// Mesh used for the floor.

std::vector<std::shared_ptr<FireworkSpawner>> g_Spawners;

//noise
std::vector<double> g_noise;
std::vector<double>::iterator CurrentNoise;

//testing for text
ID3DXFont *font;
RECT fRectangle;
std::string message;

LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL; // Buffer to hold vertices for the rectangle

struct CUSTOMVERTEX
{
	D3DXVECTOR3 position;	// Position
	FLOAT u, v;				// Texture co-ordinates.
};

// The structure of a vertex in our vertex buffer...
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

//---------------------------------------------------------------------------------------------------------------------------------
// Initialise Direct 3D.
// Requires a handle to the window in which the graphics will be drawn.

HRESULT SetupD3D(HWND hWnd)
{
	// Create the D3D object.
    if (NULL == (d3d = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;

    // Set up the structure used to create the device
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the device
    if (FAILED(d3d -> CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device)))
    {
        return E_FAIL;
    }
    
	// Enable the Z buffer, since we're dealing with 3D geometry.
	device -> SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Release (delete) all the resources used by this program.
// Only release things if they are valid (i.e. have a valid pointer).
// If not, the program will crash at this point.

void CleanUp()
{
    SAFE_RELEASE(g_BoxMesh);
	SAFE_RELEASE(device);
    SAFE_RELEASE(d3d);
	SAFE_RELEASE(font);
	SAFE_RELEASE(skyboxTex);
}

//-----------------------------------------------------------------------------
// Initialise the geometry and meshes for this demonstration.

HRESULT SetupGeometry()
{
	// Calculate the number of vertices required, and the size of the buffer to hold them.
	int Vertices = 2 * 3;	// Six vertices for the square.
	int BufferSize = Vertices * sizeof(CUSTOMVERTEX);

	// Create the vertex buffer.
	if (FAILED(device->CreateVertexBuffer(BufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL; // if the vertex buffer could not be created.
	}

	// Fill the buffer with appropriate vertices to describe the cube...

	// Create a pointer to the first vertex in the buffer.
	CUSTOMVERTEX *pVertices;
	if (FAILED(g_pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		return E_FAIL;  // if the pointer to the vertex buffer could not be established.
	}

	// Fill the vertex buffers with data...

	// Side 1 - Front face
	pVertices[0].position.x = -200;	// Vertex co-ordinate.
	pVertices[0].position.y = -200;
	pVertices[0].position.z = -200;
	pVertices[0].u = 0;
	pVertices[0].v = 1;

	pVertices[1].position.x = -200;
	pVertices[1].position.y = 200;
	pVertices[1].position.z = -200;
	pVertices[1].u = 0;
	pVertices[1].v = 0;

	pVertices[2].position.x = 200;
	pVertices[2].position.y = -200;
	pVertices[2].position.z = -200;
	pVertices[2].u = 1;
	pVertices[2].v = 1;

	pVertices[3].position.x = 200;
	pVertices[3].position.y = -200;
	pVertices[3].position.z = -200;
	pVertices[3].u = 1;
	pVertices[3].v = 1;

	pVertices[4].position.x = -200;
	pVertices[4].position.y = 200;
	pVertices[4].position.z = -200;
	pVertices[4].u = 0;
	pVertices[4].v = 0;

	pVertices[5].position.x = 200;
	pVertices[5].position.y = 200;
	pVertices[5].position.z = -200;
	pVertices[5].u = 1;
	pVertices[5].v = 0;


	// Unlock the vertex buffer...
	g_pVertexBuffer->Unlock();

	return S_OK;
}

//-----------------------------------------------------------------------------
// Set up the view - the camera and projection matrices.

void SetupViewMatrices()
{
	// Set up the view matrix.
	// This defines which way the 'camera' will look at, and which way is up.
	//float x = 350 * cos(D3DXToRadian(view_angle));
	//float z = 350 * sin(D3DXToRadian(view_angle));

	//view_angle += 0.2f;

    D3DXVECTOR3 vCamera(0.0f, 0.0f, -600.0f);
    D3DXVECTOR3 vLookat(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f);
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vCamera, &vLookat, &vUpVector);
    device -> SetTransform(D3DTS_VIEW, &matView);

	// Set up the projection matrix.
	// This transforms 2D geometry into a 3D space.
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 800.0f);
    device -> SetTransform(D3DTS_PROJECTION, &matProj);
}

//------------------------------------------------------------------------------
// Initialise one light.

void SetupLights()
{
	// Define a light - possesses only a diffuse colour.
    D3DLIGHT9 Light1;
    ZeroMemory(&Light1, sizeof(D3DLIGHT9));
    Light1.Type = D3DLIGHT_POINT;

	Light1.Diffuse.r = 1.0f;
	Light1.Diffuse.g = 1.0f;
	Light1.Diffuse.b = 1.0f;

	Light1.Position.x = 45.0f;
	Light1.Position.y = 50.0f;
	Light1.Position.z = -150.0f;

	Light1.Attenuation0 = 1.0f; 
	Light1.Attenuation1 = 0.0f; 
	Light1.Attenuation2 = 0.0f;

    Light1.Range = 500.0f;

	// Select and enable the light.
    device -> SetLight(0, &Light1);
    device -> LightEnable(0, true);
}

//-----------------------------------------------------------------------------
// Run All Update Functions

void Update()
{
	//UPDATE WIND
	if(random_number(1, 100) >= 95)
	{
		if (CurrentNoise != g_noise.end())
		{
			++CurrentNoise;
		}
		else
		{
			CurrentNoise = g_noise.begin();
		}

		float f = (float)(*CurrentNoise);
		//d is between 0 and 255
		f = f * 2.0f;
		f -= 1.0f;

		windSpeed = f;
	}

	//UPDATE ALL SPAWNERS

	for (auto s : g_Spawners)
	{
		s->Update();
	}

	//UPDATE ALL PARTICLES

	//for (auto &it = g_ParticlesAll.rbegin(); it != g_ParticlesAll.rend(); ++it)
	for (int i = 0; i < g_Particles.size(); ++i)
	{
		g_Particles[i]->update();

		//check if particle should be removed
		if (g_Particles[i]->safeToDelete)
		{
			g_Particles.erase(g_Particles.begin() + i);
			--i;
		}
	}
}


//-----------------------------------------------------------------------------
// Render the scene.

void render()
{
    // Clear the backbuffer to a blue colour, also clear the Z buffer at the same time.
    device -> Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(70, 70, 100), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(device -> BeginScene()))
    {
		D3DXQUATERNION Rotation, ScalingRotation;
		D3DXMATRIX TransformMatrix;

		D3DXVECTOR3 ScalingCentre(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 RotationCentre(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 Translate(0.0f,0.0f, 0.0f);
		D3DXVECTOR3 Scaling(1.0f, 1.0f, 1.0f);

		D3DXQuaternionRotationYawPitchRoll(&Rotation, 0.0f, 0.0f, 0.0f);
		D3DXQuaternionRotationYawPitchRoll(&ScalingRotation, 0.0f, 0.0f, 0.0f);

		D3DXMatrixTransformation(&TransformMatrix, &ScalingCentre, &ScalingRotation, &Scaling, &RotationCentre, &Rotation, &Translate);
		device -> SetTransform(D3DTS_WORLD, &TransformMatrix);

		device->SetTexture(0, skyboxTex);
		device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);

		// Render the contents of the vertex buffer.
		device->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
		device->SetFVF(D3DFVF_CUSTOMVERTEX);
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

		for (auto &p : g_Particles)
		{
			p->render();
		}

		//draw text
		if (font)
		{
			message = "Wind Speed: " + std::to_string(windSpeed);
			font->DrawTextA(NULL, message.c_str(), -1, &fRectangle, DT_LEFT, D3DCOLOR_XRGB(255,255,255));
		}

        device -> EndScene();
    }

    // Present the backbuffer to the display.
    device -> Present(NULL, NULL, NULL, NULL);
}

//-----------------------------------------------------------------------------
// Initialise the parameters for the particle system.

void SetupParticleSystems()
{
	D3DXCreateTextureFromFile(device, "yellow.png", &yellowTex);
	D3DXCreateTextureFromFile(device, "red.png", &redTex);
	D3DXCreateTextureFromFile(device, "blue.png", &blueTex);
	D3DXCreateTextureFromFile(device, "green.png", &greenTex);

	//setup text
	font = NULL;
	D3DXCreateFont(device, 20, 15, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY, FF_DONTCARE, "Arial", &font);

	SetRect(&fRectangle, 0, 0, 500, 300);

	message = "";

	//setup noise
	PerlinNoise pn(random_number());

	for (unsigned int i = 0; i < 600; ++i)
	{     // y
		for (unsigned int j = 0; j < 600; ++j)
		{  // x
			double x = (double)j / ((double)600);
			double y = (double)i / ((double)600);

			// Typical Perlin noise
			double n = pn.noise(10 * x, 10 * y, 0.8);

			g_noise.push_back(n);
		}
	}

	CurrentNoise = g_noise.begin();

	//setup skybox
	D3DXCreateTextureFromFile(device, "skybox.jpg", &skyboxTex);

	//---------------------------------------
	// SPAWNERS
	//---------------------------------------

	std::shared_ptr<FireworkSpawner> a(new FireworkSpawner(D3DXVECTOR3(150.0f, -200.0f, 0)));
	std::shared_ptr<FireworkSpawner> b(new FireworkSpawner(D3DXVECTOR3(0.0f, -200.0f, 0)));
	std::shared_ptr<FireworkSpawner> c(new FireworkSpawner(D3DXVECTOR3(-150.0f, -200.0f, 0)));
	std::shared_ptr<FireworkSpawner> d(new FireworkSpawner(D3DXVECTOR3(75.0f, -200.0f, 0)));
	std::shared_ptr<FireworkSpawner> e(new FireworkSpawner(D3DXVECTOR3(-75.0f, -200.0f, 0)));


	g_Spawners.push_back(a);
	g_Spawners.push_back(b);
	g_Spawners.push_back(c);
	g_Spawners.push_back(d);
	g_Spawners.push_back(e);

}

//-----------------------------------------------------------------------------
// The window's message handling function.

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
		{
            PostQuitMessage(0);
            return 0;
		}
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// WinMain() - The application's entry point.

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    // Register the window class
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "PSystem", NULL};
    RegisterClassEx(&wc);

    // Create the application's window
    HWND hWnd = CreateWindow("PSystem", "Particle System Demonstration", WS_OVERLAPPEDWINDOW, 50, 20, 1280, 960, GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Seed the random number generator with the value
	// from the high resolution CPU counter.
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);	
	srand(counter.LowPart);

    // Initialize Direct3D
    if (SUCCEEDED(SetupD3D(hWnd)))
    {
        // Create the scene geometry
        if (SUCCEEDED(SetupGeometry()))
        {
            // Show the window
            ShowWindow(hWnd, SW_SHOWDEFAULT);
            UpdateWindow(hWnd);

			// Set up the light.
			SetupLights();

			SetupParticleSystems();

            // Enter the message loop
            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
				{
					SetupViewMatrices();

					Update();

					render();
				}
            }
        }
    }

	CleanUp();

    UnregisterClass("PSystem", wc.hInstance);
    return 0;
}
