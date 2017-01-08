//-----------------------------------------------------------------------------
// PARTICLE SYSTEM CLASS EXAMPLE
//-----------------------------------------------------------------------------

// Include these files...

#include <stdlib.h>		// Included for the random number generator routines.
#include <d3dx9.h>		// Direct 3D library (for all Direct 3D funtions).
#include <vector>
#include <algorithm>
#include <functional>

#pragma once

#define SAFE_DELETE(p)       {if(p) {delete (p);     (p)=NULL;}}
#define SAFE_DELETE_ARRAY(p) {if(p) {delete[] (p);   (p)=NULL;}}
#define SAFE_RELEASE(p)      {if(p) {(p)->Release(); (p)=NULL;}}

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
		PARTICLE_SYSTEM_BASE() : max_particles_(0), start_particles_(0), alive_particles_(0), max_lifetime_(0), origin_(D3DXVECTOR3(0, 0, 0)), points_(NULL), particle_size_(1.0f)
		{}

		~PARTICLE_SYSTEM_BASE()
		{

			// Destructor - release the points buffer.
			SAFE_RELEASE(points_);
		}

		HRESULT initialise(LPDIRECT3DDEVICE9 device)
		{
			// Store the render target for later use...
			render_target_ = device;
			
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
			render_target_	-> SetRenderState(D3DRS_POINTSPRITEENABLE, true);
			render_target_  -> SetRenderState(D3DRS_POINTSCALEENABLE,  true);

			// Disable z buffer while rendering the particles. Makes rendering quicker and
			// stops any visual (alpha) 'artefacts' on screen while rendering.
			render_target_ -> SetRenderState(D3DRS_ZENABLE, false);
		    
			// Scale the points according to distance...
			render_target_ -> SetRenderState(D3DRS_POINTSIZE,     FtoDW(particle_size_));
			render_target_ -> SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(0.00f));
			render_target_ -> SetRenderState(D3DRS_POINTSCALE_A,  FtoDW(0.00f));
			render_target_ -> SetRenderState(D3DRS_POINTSCALE_B,  FtoDW(0.00f));
			render_target_ -> SetRenderState(D3DRS_POINTSCALE_C,  FtoDW(1.00f));

			// Now select the texture for the points...
			// Use texture colour and alpha components.
			render_target_ -> SetTexture(0, particle_texture_);
			render_target_ -> SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			render_target_ -> SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
			render_target_ -> SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			render_target_ -> SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			render_target_ -> SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_SELECTARG1);
			render_target_ -> SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			render_target_ -> SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);

			// Render the contents of the vertex buffer.
			render_target_ -> SetStreamSource(0, points_, 0, sizeof(POINTVERTEX));
			render_target_ -> SetFVF(D3DFVF_POINTVERTEX);
			render_target_ -> DrawPrimitive(D3DPT_POINTLIST, 0, alive_particles_);

			// Reset the render states.
			render_target_ -> SetRenderState(D3DRS_POINTSPRITEENABLE, false);
			render_target_ -> SetRenderState(D3DRS_POINTSCALEENABLE,  false);
			render_target_ -> SetRenderState(D3DRS_ALPHABLENDENABLE,  false);
			render_target_ -> SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
			render_target_ -> SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		}

		int max_particles_;						// The maximum number of particles in this particle system.
		int start_particles_;					// Number of particles to start in each batch.

		int alive_particles_;					// The number of particles that are currently alive.
		int max_lifetime_;					    // The start age of each particle (count down from this, kill particle when zero).

		LPDIRECT3DTEXTURE9 particle_texture_;	// The texture for the points.				
		D3DXVECTOR3 origin_;					// Vectors for origin of the particle system.

		int start_timer_;						// Count-down timer, start another particle when zero.		
		int start_interval_;		     		// Interval between starting a new particle (used to initialise 'start_timer_').
		float time_increment_;					// Used to increase the value of 'time'for each particle - used to calculate vertical position.

		float particle_size_;					// Size of the point.

	private:

		class is_particle_dead			// This is a private class, only available inside 'PARTICLE_SYSTEM_BASE' - functor to determine if a particle is alive or dead.
		{	
			public:	
				bool operator()(const PARTICLE &p)
				{
					return p.lifetime_ == 0;
				}
		};

		std::vector<PARTICLE>::iterator find_next_dead_particle()
		{
			return std::find_if(particles_.begin(), particles_.end(), is_particle_dead());
		}

	protected:
	
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

		std::vector<PARTICLE>	particles_;

		LPDIRECT3DVERTEXBUFFER9 points_;  // Vertex buffer for the points.
		LPDIRECT3DDEVICE9		render_target_;
		
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
					// Calculate the new position of the particle...

					// Vertical distance.
					float s = (p -> velocity_.y * p -> time_) + (gravity_ * p -> time_ * p -> time_);

					p -> position_.y = s + origin_.y;
					p -> position_.x = (p -> velocity_.x * p -> time_) + origin_.x;
					p -> position_.z = (p -> velocity_.z * p -> time_) + origin_.z;

					p -> time_ += time_increment_;
					--(p -> lifetime_);

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


class FIREWORK_CLASS : public PARTICLE_SYSTEM_BASE
{
public:
	FIREWORK_CLASS() : PARTICLE_SYSTEM_BASE(), gravity_(0), terminate_on_floor_(false), floorY_(0) {}

	// Update the positions of the particles, and start new particles if necessary.
	void update()
	{
		// Start particles, if necessary...
		start_particles();

		// Update the particles that are still alive...
		for (std::vector<PARTICLE>::iterator p(particles_.begin()); p != particles_.end(); ++p)
		{
			if (p->lifetime_ > 0)	// Update only if this particle is alive.
			{
				// Calculate the new position of the particle...

				// Vertical distance.
				float s = (p->velocity_.y * p->time_) + (gravity_ * p->time_ * p->time_);

				p->position_.y = s + origin_.y;
				p->position_.x = (p->velocity_.x * p->time_) + origin_.x;
				p->position_.z = (p->velocity_.z * p->time_) + origin_.z;

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
	}

	bool  terminate_on_floor_;		// Flag to indicate that particles will die when they hit the floor (floorY_).
	float gravity_, floorY_, launch_velocity_;

private:

	virtual void start_single_particle(std::vector<PARTICLE>::iterator &p)	// Initialise/start particle 'p'.
	{
		if (p == particles_.end()) return;	// Safety net - if there are no dead particles, don't start any new ones...

											// Reset the particle's time (for calculating it's position with s = ut+0.5t*t)
		p->time_ = 0;

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
		int n = random_number(100, max_lifetime_);

		p->lifetime_ = n;

		++alive_particles_;
	}
};
