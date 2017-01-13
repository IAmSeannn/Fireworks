#pragma once
//includes
// Include these files...
#include <stdlib.h>		// Included for the random number generator routines.
#include <algorithm>
#include <functional>
#include <d3dx9.h>
#include <vector>
#include <memory>

#define SAFE_DELETE(p)       {if(p) {delete (p);     (p)=NULL;}}
#define SAFE_DELETE_ARRAY(p) {if(p) {delete[] (p);   (p)=NULL;}}
#define SAFE_RELEASE(p)      {if(p) {(p)->Release(); (p)=NULL;}}

//initialisers (I think)
class PARTICLE_SYSTEM_BASE;

//global vars
LPDIRECT3DDEVICE9       device = NULL;	// The rendering device
std::vector<std::shared_ptr<PARTICLE_SYSTEM_BASE>> g_Particles;
float windSpeed = 0.0f;

void CreateRocket(D3DXVECTOR3 startLocation);
void CreateExplosion(D3DXVECTOR3 startLocation);

LPDIRECT3DTEXTURE9	blueTex = NULL, redTex = NULL, yellowTex = NULL, greenTex = NULL, skyboxTex = NULL;

//-----------------------------------------------------------------------------
// PARTICLE CLASSES
//-----------------------------------------------------------------------------

unsigned int random_number()
{
	unsigned int n;
	rand_s(&n);
	return n;
}

unsigned int random_number(unsigned int a, unsigned int b)	// return a random number between a and b.
{
	unsigned int n;
	rand_s(&n);
	return a + (n % (b - a));
}

// A structure for point sprites.
struct POINTVERTEX
{
    D3DXVECTOR3 position_;		// X, Y, Z position of the point (sprite).
};

// The structure of a vertex in our vertex buffer...
#define D3DFVF_POINTVERTEX D3DFVF_XYZ

// Helper function to convert a float into a DWORD.
inline DWORD FtoDW(float f) {return *((DWORD*)&f); }

//-----------------------------------------------------------------------------

struct PARTICLE
{
	int			lifetime_;
	D3DXVECTOR3 position_, velocity_;
	float		time_;
};

#define reset_particle(p) SecureZeroMemory(&p, sizeof(PARTICLE));

//-----------------------------------------------------------------------------

class PARTICLE_SYSTEM_BASE
{
	public:
		PARTICLE_SYSTEM_BASE() : max_particles_(0), alive_particles_(0), max_lifetime_(0), origin_(D3DXVECTOR3(0, 0, 0)), points_(NULL), particle_size_(1.0f), safeToDelete(false)
		{}

		~PARTICLE_SYSTEM_BASE()
		{

			// Destructor - release the points buffer.
			SAFE_RELEASE(points_);
		}

		virtual HRESULT initialise()
		{			
			PARTICLE p;
			reset_particle(p);
			particles_.resize(max_particles_, p);	// Create a vector of empty particles - make 'max_particles_' copies of particle 'p'.

			// Create a vertex buffer for the particles (each particule represented as an individual vertex).
			int buffer_size = max_particles_ * sizeof(POINTVERTEX);

			// The data in the buffer doesn't exist at this point, but the memory space
			// is allocated and the pointer to it (g_pPointBuffer) also exists.
			if (FAILED(device -> CreateVertexBuffer(buffer_size, 0, D3DFVF_POINTVERTEX, D3DPOOL_DEFAULT, &points_, NULL)))
			{
				return E_FAIL; // Return if the vertex buffer culd not be created.
			}

			return S_OK;
		}

		virtual void update() = 0;	// Specific implementations to provide this - this is to update the positions
									// of the particles.

		void render()
		{
			// Enable point sprites, and set the size of the point.
			device -> SetRenderState(D3DRS_POINTSPRITEENABLE, true);
			device-> SetRenderState(D3DRS_POINTSCALEENABLE,  true);

			// Disable z buffer while rendering the particles. Makes rendering quicker and
			// stops any visual (alpha) 'artefacts' on screen while rendering.
			device-> SetRenderState(D3DRS_ZENABLE, false);
		    
			// Scale the points according to distance...
			device-> SetRenderState(D3DRS_POINTSIZE,     FtoDW(particle_size_));
			device-> SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(0.00f));
			device-> SetRenderState(D3DRS_POINTSCALE_A,  FtoDW(0.00f));
			device-> SetRenderState(D3DRS_POINTSCALE_B,  FtoDW(0.00f));
			device-> SetRenderState(D3DRS_POINTSCALE_C,  FtoDW(1.00f));

			// Now select the texture for the points...
			// Use texture colour and alpha components.
			device-> SetTexture(0, particle_texture_);
			device-> SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			device-> SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
			device-> SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device-> SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			device-> SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_SELECTARG1);
			device-> SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			device-> SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);

			// Render the contents of the vertex buffer.
			device-> SetStreamSource(0, points_, 0, sizeof(POINTVERTEX));
			device-> SetFVF(D3DFVF_POINTVERTEX);
			device-> DrawPrimitive(D3DPT_POINTLIST, 0, alive_particles_);

			// Reset the render states.
			device-> SetRenderState(D3DRS_POINTSPRITEENABLE, false);
			device-> SetRenderState(D3DRS_POINTSCALEENABLE,  false);
			device-> SetRenderState(D3DRS_ALPHABLENDENABLE,  false);
			device-> SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
			device-> SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		}

		int max_particles_;						// The maximum number of particles in this particle system.

		int alive_particles_;					// The number of particles that are currently alive.
		int max_lifetime_;					    // The start age of each particle (count down from this, kill particle when zero).

		LPDIRECT3DTEXTURE9 particle_texture_;	// The texture for the points.				
		D3DXVECTOR3 origin_;					// Vectors for origin of the particle system.

		float time_increment_;					// Used to increase the value of 'time'for each particle - used to calculate vertical position.
		float particle_size_;					// Size of the point.
		bool safeToDelete;

	private:

		class is_particle_dead			// This is a private class, only available inside 'PARTICLE_SYSTEM_BASE' - functor to determine if a particle is alive or dead.
		{	
			public:	
				bool operator()(const PARTICLE &p)
				{
					return p.lifetime_ == 0;
				}
		};

	protected:

		std::vector<PARTICLE>::iterator find_next_dead_particle()
		{
			return std::find_if(particles_.begin(), particles_.end(), is_particle_dead());
		}
	
		virtual void start_particles() = 0;

		std::vector<PARTICLE>	particles_;

		LPDIRECT3DVERTEXBUFFER9 points_;  // Vertex buffer for the points.
		
		// Specific implemention to define to policy for starting/creating a single particle.
		virtual void start_single_particle(std::vector<PARTICLE>::iterator &) = 0;
};

//-----------------------------------------------------------------------------------------------------------------------------------------------------

class FOUNTAIN_CLASS : public PARTICLE_SYSTEM_BASE
{
	public:
		FOUNTAIN_CLASS() : PARTICLE_SYSTEM_BASE(), gravity_(0), terminate_on_floor_(false), floorY_(0) {}

		// Update the positions of the particles, and start new particles if necessary.
		void update()
		{
			// Start particles, if necessary...
			start_particles();

			// Update the particles that are still alive...
			for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
			{
				if (p -> lifetime_ > 0)	// Update only if this particle is alive.
				{
					--(p->lifetime_);
					// Calculate the new position of the particle...

					// Vertical distance.
					float s = (p -> velocity_.y * p -> time_) + (gravity_ * p -> time_ * p -> time_);

					p -> position_.y = s + origin_.y;
					p -> position_.x = (p -> velocity_.x * p -> time_) + origin_.x;
					p -> position_.z = (p -> velocity_.z * p -> time_) + origin_.z;

					p -> time_ += time_increment_;
					

					if (p -> lifetime_ == 0)	// Has this particle come to the end of it's life?
					{
						--alive_particles_;		// If so, terminate it.
					}
					else
					{
						if (terminate_on_floor_)	// or has the particle hit the floor? if so, terminate it. Flag to determine if to do this.
						{
							if (p -> position_.y < floorY_)
							{
								p -> lifetime_ = 0;
								--alive_particles_;
							}
						}
					}
				}
			}

			// Create a pointer to the first vertex in the buffer
			// Also lock it, so nothing else can touch it while the values are being inserted.
			POINTVERTEX *points;
			points_ -> Lock(0, 0, (void**)&points, 0);

			// Fill the vertex buffers with data...
			int P(0);

			// Now update the vertex buffer - after the update has been
			// performed, just in case this particle has died in the process.

			for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
			{
				if (p -> lifetime_ > 0)
				{
					points[P].position_.y = p -> position_.y;
					points[P].position_.x = p -> position_.x;
					points[P].position_.z = p -> position_.z;
					++P;
				}
			}

			points_ -> Unlock();
		}

		bool  terminate_on_floor_;		// Flag to indicate that particles will die when they hit the floor (floorY_).
		float gravity_, floorY_, launch_angle_, launch_velocity_;

	private:

		virtual void start_single_particle(std::vector<PARTICLE>::iterator &p)	// Initialise/start particle 'p'.
		{
			if (p == particles_.end()) return;	// Safety net - if there are no dead particles, don't start any new ones...

			// Reset the particle's time (for calculating it's position with s = ut+0.5t*t)
			p -> time_ = 0;

			// Now calculate the particle's horizontal and depth components.
			// The particle can be ejected at a random angle, around a circle.
			float direction_angle = (float)(D3DXToRadian(random_number()));

			// Calculate the vertical component of velocity.
			p -> velocity_.y = launch_velocity_ * (float)sin(launch_angle_);

			// Calculate the horizontal components of velocity.
			// This is X and Z dimensions.
			p -> velocity_.x = launch_velocity_ * (float)cos(launch_angle_) * (float)cos(direction_angle);
			p -> velocity_.z = launch_velocity_ * (float)cos(launch_angle_) * (float)sin(direction_angle);

			p -> lifetime_ = max_lifetime_;

			++alive_particles_;
		}
};

//-----------------------------------------------------------------------------------------------------------------------------------------------------

class FIREWORK_EXPLOSION_CLASS : public PARTICLE_SYSTEM_BASE
{
public:
	FIREWORK_EXPLOSION_CLASS() : PARTICLE_SYSTEM_BASE(), gravity_(0), terminate_on_floor_(false), floorY_(0) {}

	HRESULT initialise()
	{
		HRESULT temp = PARTICLE_SYSTEM_BASE::initialise();
		start_particles();
		return temp;	
	}

	// Update the positions of the particles, and start new particles if necessary.
	void update()
	{
		// Update the particles that are still alive...
		for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
		{

			if (p->lifetime_ > 0)	// Update only if this particle is alive.
			{
				// Calculate the new position of the particle...

				// Vertical distance.
				/*float s = (p->velocity_.y * p->time_) + gravity_;

				p->position_.y = s + origin_.y;
				p->position_.x = (p->velocity_.x * p->time_) + origin_.x;
				p->position_.z = (p->velocity_.z * p->time_) + origin_.z;*/

				p->position_.y += p->velocity_.y + gravity_;
				p->position_.x += p->velocity_.x + windSpeed;
				p->position_.z += p->velocity_.z;

				p->velocity_.y *= time_increment_;
				p->velocity_.x *= time_increment_; 
				p->velocity_.z *= time_increment_;

				p->time_ += time_increment_;
				--(p->lifetime_);

				if (p->lifetime_ == 0)	// Has this particle come to the end of it's life?
				{
					--alive_particles_;		// If so, terminate it.
				}
				else
				{
					if (terminate_on_floor_)	// or has the particle hit the floor? if so, terminate it. Flag to determine if to do this.
					{
						if (p->position_.y < floorY_)
						{
							p->lifetime_ = 0;
							--alive_particles_;
						}
					}
				}
			}
		}

		// Create a pointer to the first vertex in the buffer
		// Also lock it, so nothing else can touch it while the values are being inserted.
		POINTVERTEX *points;
		points_->Lock(0, 0, (void**)&points, 0);

		// Fill the vertex buffers with data...
		int P(0);

		// Now update the vertex buffer - after the update has been
		// performed, just in case this particle has died in the process.

		for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
		{
			if (p->lifetime_ > 0)
			{
				points[P].position_.y = p->position_.y;
				points[P].position_.x = p->position_.x;
				points[P].position_.z = p->position_.z;
				++P;
			}
		}

		points_->Unlock();

		if (alive_particles_ == 0)
		{
			safeToDelete = true;
		}
	}

	void start_particles()
	{
		// start all the particles
		for (int i(0); i < max_particles_; ++i)
		{
			if (alive_particles_ < max_particles_) start_single_particle(find_next_dead_particle());
		}
	}

	bool  terminate_on_floor_;		// Flag to indicate that particles will die when they hit the floor (floorY_).
	float gravity_, floorY_, launch_velocity_;

private:

	virtual void start_single_particle(std::vector<PARTICLE>::iterator &p)	// Initialise/start particle 'p'.
	{
		if (p == particles_.end()) return;	// Safety net - if there are no dead particles, don't start any new ones...

		// Reset the particle's time (for calculating it's position with s = ut+0.5t*t)
		p->time_ = 1;

		// Now calculate the particle's horizontal and depth components.
		// The particle can be ejected at a random angle, around a sphere.
		float direction_angle = (float)(D3DXToRadian(random_number()));
		float launch_angle_ = (float)(D3DXToRadian(random_number()));

		// Calculate the vertical component of velocity.
		p->velocity_.y = launch_velocity_ * (float)sin(launch_angle_);

		// Calculate the horizontal components of velocity.
		// This is X and Z dimensions.
		p->velocity_.x = launch_velocity_ * (float)cos(launch_angle_) * (float)cos(direction_angle);
		p->velocity_.z = launch_velocity_ * (float)cos(launch_angle_) * (float)sin(direction_angle);

		//have random lifetime
		int n = random_number(0, max_lifetime_);

		//set initial position
		p->position_ = origin_;

		p->lifetime_ = n;

		++alive_particles_;
	}
};

//-----------------------------------------------------------------------------------------------------------------------------------------------------

class FIREWORK_ROCKET_CLASS : public PARTICLE_SYSTEM_BASE
{
public:
	FIREWORK_ROCKET_CLASS() : PARTICLE_SYSTEM_BASE(), gravity_(0), terminate_on_floor_(false), floorY_(0), activated(false) {}

	// Update the positions of the particles, and start new particles if necessary.
	void update()
	{
		// Start particles, if necessary...
		//and if rocket is still alive
		if (rocketTime > 0)
		{
			start_particles();
		}

		// Update the particles that are still alive...
		for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
		{
			if (p->lifetime_ > 0)	// Update only if this particle is alive.
			{
				// Calculate the new position of the particle...

				// Vertical distance.
				/*float s = (p->velocity_.y * p->time_) + (gravity_ * p->time_ * p->time_);

				p->position_.y = s + origin_.y;
				p->position_.x = (p->velocity_.x * p->time_) + origin_.x;
				p->position_.z = (p->velocity_.z * p->time_) + origin_.z;*/

				p->position_ += p->velocity_;
				p->position_.x += windSpeed;

				p->time_ += time_increment_;
				--(p->lifetime_);

				if (p->lifetime_ == 0)	// Has this particle come to the end of it's life?
				{
					--alive_particles_;		// If so, terminate it.
				}
				else
				{
					if (terminate_on_floor_)	// or has the particle hit the floor? if so, terminate it. Flag to determine if to do this.
					{
						if (p->position_.y < floorY_)
						{
							p->lifetime_ = 0;
							--alive_particles_;
						}
					}
				}
			}
		}

		// Create a pointer to the first vertex in the buffer
		// Also lock it, so nothing else can touch it while the values are being inserted.
		POINTVERTEX *points;
		points_->Lock(0, 0, (void**)&points, 0);

		// Fill the vertex buffers with data...
		int P(0);

		// Now update the vertex buffer - after the update has been
		// performed, just in case this particle has died in the process.

		for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
		{
			if (p->lifetime_ > 0)
			{
				points[P].position_.y = p->position_.y;
				points[P].position_.x = p->position_.x;
				points[P].position_.z = p->position_.z;
				++P;
			}
		}

		points_->Unlock();

		//move the rocket up along the y axis a little
		origin_ += RocketVel;
		origin_.x += windSpeed;

		//check if time to explode
		if (!activated)
		{
			if (rocketTime > 0)
			{
				--rocketTime;
			}
			else
			{
				activated = true;
				//explode explosion
				CreateExplosion(origin_);
			}
		}
		else
		{
			if (alive_particles_ == 0)
			{
				safeToDelete = true;
			}
		}
			
	}

	void start_particles()
	{
		// Only start a new particle when the time is right and there are enough dead (inactive) particles.
		if (start_timer_ == 0 && alive_particles_ < max_particles_)
		{
			// Number of particles to start in this batch...
			for (int i(0); i < start_particles_; ++i)
			{
				if (alive_particles_ < max_particles_) start_single_particle(find_next_dead_particle());
			}

			// Reset the start timer for the next batch of particles.
			start_timer_ = start_interval_;
		}
		else
		{
			// Otherwise decrease the start timer.
			--start_timer_;
		}
	}

	bool  terminate_on_floor_;		// Flag to indicate that particles will die when they hit the floor (floorY_).
	float gravity_, floorY_, launch_velocity_;
	float rocketTime;
	D3DXVECTOR3 RocketVel;

	int start_particles_;					// Number of particles to start in each batch.
	int start_timer_;						// Count-down timer, start another particle when zero.		
	int start_interval_;		     		// Interval between starting a new particle (used to initialise 'start_timer_').

private:

	bool activated;

	virtual void start_single_particle(std::vector<PARTICLE>::iterator &p)	// Initialise/start particle 'p'.
	{
		if (p == particles_.end()) return;	// Safety net - if there are no dead particles, don't start any new ones...

											// Reset the particle's time (for calculating it's position with s = ut+0.5t*t)
		p->time_ = 0;

		//set initial position
		p->position_ = origin_;

		// Now calculate the particle's horizontal and depth components.
		// The particle can be ejected at a random angle, around a sphere.
		float direction_angle = (float)(D3DXToRadian(random_number(85, 95)));
		float launch_angle_ = (float)(D3DXToRadian(0));

		// Calculate the vertical component of velocity.
		p->velocity_.y = launch_velocity_ * (float)sin(launch_angle_);

		// Calculate the horizontal components of velocity.
		// This is X and Z dimensions.
		p->velocity_.x = launch_velocity_ * (float)cos(launch_angle_) * (float)cos(direction_angle);
		p->velocity_.z = launch_velocity_ * (float)cos(launch_angle_) * (float)sin(direction_angle);

		p->lifetime_ = max_lifetime_;

		++alive_particles_;
	}
};

//-----------------------------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FIREWORK CREATORS
//-----------------------------------------------------------------------------

LPDIRECT3DTEXTURE9 &getRandomTexture()
{
	int i = random_number(0, 4);

	switch (i)
	{
	case 0:
		return greenTex;
		break;
	case 1:
		return redTex;
		break;
	case 2:
		return blueTex;
		break;
	default:
		return yellowTex;
		break;
	}
}

void CreateRocket(D3DXVECTOR3 startLocation)
{
	std::shared_ptr<FIREWORK_ROCKET_CLASS> f(new FIREWORK_ROCKET_CLASS);

	//add a rocket1
	f->max_particles_ = 500;
	f->rocketTime = 160 + random_number(0,20);
	f->origin_ = startLocation;
	f->start_interval_ = 1;
	f->start_timer_ = 0;
	f->launch_velocity_ = 1.0f;
	f->time_increment_ = 0.05f;
	f->max_lifetime_ = 20;
	f->start_particles_ = 2;
	f->particle_size_ = 0.5f;

	//slightly random vel
	float x = (float)random_number(0, 30);
	float z = (float)random_number(0, 30);
	x = ((x - 15) / 100);
	z = ((z - 15) / 100);
	f->RocketVel = D3DXVECTOR3(x, 2.0f, z);

	f->particle_texture_ = getRandomTexture();

	f->initialise();
	g_Particles.push_back(f);
}

void CreateExplosion(D3DXVECTOR3 startLocation)
{
	std::shared_ptr<FIREWORK_EXPLOSION_CLASS> f(new FIREWORK_EXPLOSION_CLASS);

	////add a firework1
	f->max_particles_ = 300;
	f->origin_ = startLocation;
	f->gravity_ = -0.5f;
	f->launch_velocity_ = 5.0f;
	f->time_increment_ = 0.95;
	f->max_lifetime_ = 100;
	f->particle_size_ = 2.5f;

	f->particle_texture_ = getRandomTexture();

	f->initialise();
	g_Particles.push_back(f);
}

//-----------------------------------------------------------------------------
// FIREWORK SPAWNER
//-----------------------------------------------------------------------------

class FireworkSpawner
{
public:
	FireworkSpawner(int count, D3DXVECTOR3 Loc)
	: MAX_COUNTER(count), counter(0), Location(Loc){};
	~FireworkSpawner() {};

	D3DXVECTOR3 Location;
	int counter;
	const int MAX_COUNTER;

	void Update()
	{
		if (counter == 0)
		{
			//activate
			CreateRocket(Location);
			//reset
			counter = MAX_COUNTER;
		}
		else
		{
			--counter;
		}
	}
};