enum class ParticleEmitterId {
	Dust,
	LandingDust,
	SmallDissipate,
	SmallExplosion,
};
enum class ParticleTexture : int8 {
	Dust,
	SmallDissipate,
	Propulsion,
	SmallExplosion,
	Smoke,

	Count
};
namespace ParticleEmitterFlags
{
enum Values : uint32 {
	AlternateSignX = BITFIELD( 0 ),
};
}
struct ParticleEmitter {
	int32 count;
	float maxAlive;
	vec2 velocity;
	ParticleTexture textureId;
	uint32 flags;
};

struct ParticleSystem {
	struct Particle {
		vec3 position;
		vec2 velocity;
		float alive;
		float maxAlive;
		ParticleTexture textureId;
	};
	UArray< Particle > particles;

	TextureId texture;
};

ParticleSystem makeParticleSystem( StackAllocator* allocator, int32 maxParticles )
{
	ParticleSystem result = {};
	result.particles      = makeUArray( allocator, ParticleSystem::Particle, maxParticles );
	return result;
}
static const ParticleEmitter ParticleEmitters[] = {
    {1, 20, {0, -0.1f}, ParticleTexture::Dust},  // Dust
    {2,
     10,
     {0.5f, -0.1f},
     ParticleTexture::Dust,
     ParticleEmitterFlags::AlternateSignX},           // LandingDust
    {1, 10, {}, ParticleTexture::SmallDissipate, 0},  // SmallDissipate
    {1, 20, {}, ParticleTexture::SmallExplosion, 0},  // SmallExplosion
};
Array< ParticleSystem::Particle > emitParticles( ParticleSystem* system, vec3arg position,
                                                 const ParticleEmitter& emitter )
{
	Array< ParticleSystem::Particle > result = {};
	auto count = min( system->particles.remaining(), emitter.count );
	if( count > 0 ) {
		auto start = system->particles.size();
		system->particles.resize( start + count );
		result = makeRangeView( system->particles, start, start + count );
		FOR( entry : result ) {
			entry.position  = position;
			entry.velocity  = emitter.velocity;
			entry.alive     = emitter.maxAlive;
			entry.maxAlive  = emitter.maxAlive;
			entry.textureId = emitter.textureId;
		}
		if( emitter.flags & ParticleEmitterFlags::AlternateSignX ) {
			bool sign = true;
			FOR( entry : result ) {
				if( sign ) {
					entry.velocity.x = -entry.velocity.x;
					sign             = !sign;
				}
			}
		}
	}
	return result;
}
Array< ParticleSystem::Particle > emitParticles( ParticleSystem* system, vec2arg position,
                                                 ParticleEmitterId emitterId )
{
	assert( system );
	assert( valueof( emitterId ) < countof( ParticleEmitters ) );
	auto& emitter = ParticleEmitters[valueof( emitterId )];
	return emitParticles( system, Vec3( position, 0 ), emitter );
}
void processParticles( ParticleSystem* system, float dt )
{
	auto end = system->particles.end();
	for( auto it = system->particles.begin(); it != end; ) {
		it->position.xy += it->velocity * dt;
		it->alive -= dt;
		if( it->alive <= 0 ) {
			it  = unordered_erase( system->particles, it );
			end = system->particles.end();
			continue;
		}
		++it;
	}
}
void renderParticles( RenderCommands* renderer, ParticleSystem* system )
{
	setRenderState( renderer, RenderStateType::DepthTest, false );
	setTexture( renderer, 0, system->texture );
	// TODO: use a texture map/atlas instead of hardcoding
	const int32 CellsX   = 4;
	const int32 CellsY   = 5;
	const float CellSizeX = 1.0f / CellsX;
	const float CellSizeY = 1.0f / CellsY;
	static const trange< int8 > ParticleRanges[]{
	    {8, 12},    // dust
	    {5, 8},    // small dissipate
	    {12, 16},   // propulsion
	    {16, 20},  // small explosion
	    {0, 5},    // smoke
	};

	MESH_STREAM_BLOCK( stream, renderer ) {
		FOR( particle : system->particles ) {
			auto t            = particle.alive / particle.maxAlive;
			auto beenAliveFor = particle.maxAlive - particle.alive;
			stream->color     = Color::White;
			// alpha blend between first and last couple of frames of the particles life time
			constexpr const float FadeDuration    = 3;
			constexpr const float InvFadeDuration = 1 / FadeDuration;
			if( beenAliveFor < FadeDuration ) {
				stream->color = setAlpha( Color::White, beenAliveFor * InvFadeDuration );
			} else if( beenAliveFor > particle.maxAlive - FadeDuration ) {
				beenAliveFor -= particle.maxAlive - FadeDuration;
				stream->color = setAlpha( Color::White, 1 - beenAliveFor * InvFadeDuration );
			}
			rectf rect = {-4, -4, 5, 5};
			rect       = gameToScreen( translate( rect, particle.position.xy ) );

			assert( valueof( particle.textureId ) >= 0
			        && valueof( particle.textureId ) < valueof( ParticleTexture::Count ) );

			auto range = ParticleRanges[valueof( particle.textureId )];
			auto index = clamp( (int8)lerp( 1.0f - t, 0.0f, (float)width( range ) ), (int8)0,
			                    width( range ) )
			             + range.min;
			// TODO: use a texture atlas for particles and a lookup table
			auto cellX     = index % CellsX;
			auto cellY     = index / CellsX;
			float u        = cellX * CellSizeX;
			float v        = cellY * CellSizeY;
			auto texCoords = makeQuadTexCoords( RectWH( u, v, CellSizeX, CellSizeY ) );
			pushQuad( stream, rect, particle.position.z, texCoords );
		}
	}
	setRenderState( renderer, RenderStateType::DepthTest, true );
}