//-----------------------------------------------------------------------------
// PARTICLE SYSTEM EXAMPLE

//-----------------------------------------------------------------------------
// Include these files

#define _CRT_RAND_S	// Define this before any includes to select correct rand function in stdlib.h.

#include <Windows.h>	// Windows library (for window functions, menus, dialog boxes, etc)
#include "particlesystem.h"

//---------------------------------------------------------------------------------------------------------------------------------
// Global variables

LPDIRECT3D9             d3d		= NULL;	// Used to create the device
LPDIRECT3DDEVICE9       device	= NULL;	// The rendering device

LPD3DXMESH g_BoxMesh = NULL;						// Mesh used for the floor.

//FOUNTAIN_CLASS /*fountain1,*/ fountain2;
//FIREWORK_EXPLOSION_CLASS firework1, firework2, firework3;

std::vector<std::shared_ptr<PARTICLE_SYSTEM_BASE>> g_ParticlesAll;

LPDIRECT3DTEXTURE9	spark_bitmap = NULL, particle_bitmap = NULL, spark_bitmap2 = NULL, spark_bitmap3 = NULL;			// The texture for the point sprites.

//float view_angle = 10;								// Angle for moving the camera.

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
}

//-----------------------------------------------------------------------------
// Initialise the geometry and meshes for this demonstration.

HRESULT SetupGeometry()
{
	return D3DXCreateBox(device, 250, 1, 150, &g_BoxMesh, NULL);
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

    D3DXVECTOR3 vCamera(0.0f, 600.0f, 600.0f);
    D3DXVECTOR3 vLookat(0.0f, 150.0f, 0.0f);
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
// Render the scene.

void render()
{
    // Clear the backbuffer to a blue colour, also clear the Z buffer at the same time.
    device -> Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(70, 70, 100), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(device -> BeginScene()))
    {
		D3DMATERIAL9 Mtl;
		ZeroMemory(&Mtl, sizeof(D3DMATERIAL9));

		Mtl.Diffuse.r = 1.0f;
		Mtl.Diffuse.g = 1.0f;
		Mtl.Diffuse.b = 1.0f;
		device -> SetMaterial(&Mtl);

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

		g_BoxMesh -> DrawSubset(0);

		// Render the fountains last, as Z buffer is turned off during rendering these.
		//fountain1.render();
		//fountain2.render();
		/*firework1.render();
		firework2.render();
		firework3.render();*/
		//rocket1.render();

		for (auto &p : g_ParticlesAll)
		{
			p->render();
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
	////add a firework1
	//firework1.max_particles_ = 500;
	//firework1.origin_ = D3DXVECTOR3(0.0f, 300.0f, 0);
	//firework1.gravity_ -9.81f / 2;  // Gravity - pre divided by 2.
	//firework1.start_interval_ = 500;
	//firework1.start_timer_ = 0;
	//firework1.launch_velocity_ = 10.0f;
	//firework1.time_increment_ = 0.05f;
	//firework1.max_lifetime_ = 300;
	//firework1.start_particles_ = 100;
	//firework1.particle_size_ = 8.0f;
	//D3DXCreateTextureFromFile(device, "spark.png", &spark_bitmap);
	//firework1.particle_texture_ = spark_bitmap;
	//firework1.initialise(device);

	D3DXCreateTextureFromFile(device, "spark3.png", &spark_bitmap3);

	std::shared_ptr<FIREWORK_ROCKET_CLASS> f(new FIREWORK_ROCKET_CLASS);


	//add a rocket1
	f->max_particles_ = 500;
	f->rocketTime = 200;
	f->origin_ = D3DXVECTOR3(100.0f, 0.0f, 0);
	f->gravity_ - 9.81f / 2;  // Gravity - pre divided by 2.
	f->start_interval_ = 2;
	f->start_timer_ = 0;
	f->launch_velocity_ = 1.0f;
	f->time_increment_ = 0.05f;
	f->max_lifetime_ = 100;
	f->start_particles_ = 2;
	f->particle_size_ = 1.0f;
	f->particle_texture_ = spark_bitmap3;
	f->globalParticles = &g_ParticlesAll;
	f->initialise(device);

	g_ParticlesAll.push_back(f);
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
    HWND hWnd = CreateWindow("PSystem", "Particle System Demonstration", WS_OVERLAPPEDWINDOW, 50, 20, 700, 700, GetDesktopWindow(), NULL, wc.hInstance, NULL);

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
					//fountain1.update();
					//fountain2.update();
					/*firework1.update();
					firework2.update();
					firework3.update();*/
					//rocket1.update();

					/*for (auto &p : g_ParticlesAll)
					{
						p->update();
					}*/

					for (int i = 0; i < g_ParticlesAll.size(); i++)
					{
						g_ParticlesAll[i]->update();
					}

					render();
				}
            }
        }
    }

	CleanUp();

    UnregisterClass("PSystem", wc.hInstance);
    return 0;
}
