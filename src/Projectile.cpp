const ProjectileTraits* getProjectileTraits( ProjectileType type )
{
	static const ProjectileTraits ProjectileTraitsEntries[] = {
	    // Hero
	    {
	        3,                                             // initialSpeed
	        {},                                            // maxSpeed
	        {},                                            // bounceSpeed
	        0,                                             // acceleration
	        60,                                            // alive
	        0,                                             // gravityModifier
	        GameConstants::DefaultAirFrictionCoefficient,  // airFrictionCoeffictient
	        1,                                             // bounceModifier
	        1,                                             // durability
	        {
	            false,                                   // hasHurtbox
	            ProjectileCollisionResponse::Dissipate,  // leftRightResponse
	            ProjectileCollisionResponse::Dissipate,  // upResponse
	            ProjectileCollisionResponse::Dissipate,  // downResponse
	        },
	    },

	    // Wheels
	    {
	        1,                                             // initialSpeed
	        {},                                            // maxSpeed
	        {0, 2.6f},                                     // bounceSpeed
	        0,                                             // acceleration
	        300,                                           // alive
	        1,                                             // gravityModifier
	        GameConstants::DefaultAirFrictionCoefficient,  // airFrictionCoeffictient
	        2,                                             // bounceModifier
	        1,                                             // durability
	        {
	            false,                                                 // hasHurtbox
	            ProjectileCollisionResponse::Dissipate,                // leftRightResponse
	            ProjectileCollisionResponse::BounceWithConstantSpeed,  // upResponse
	            ProjectileCollisionResponse::Dissipate,                // downResponse
	        },
	    },
	};

	assert( valueof( type ) >= 0 && valueof( type ) < valueof( ProjectileType::Count ) );
	return &ProjectileTraitsEntries[valueof( type )];
}

const ProjectileData* getProjectileData( ProjectileSystem* system, ProjectileType type )
{
	assert( valueof( type ) >= 0 && valueof( type ) < valueof( ProjectileType::Count ) );
	return &system->data[valueof( type )];
}

ProjectileSystem makeProjectileSystem( StackAllocator* allocator, int32 maxProjectiles )
{
	ProjectileSystem result = {};
	result.entries          = makeUArray( allocator, Projectile, maxProjectiles );

	// load data
	static const StringView Files[] = {
	    "Data/voxels/projectile_anim.json",   // Hero
	    "Data/voxels/green_projectile.json",  // Wheels
	};

	for( int32 i = 0, count = (int32)ProjectileType::Count; i < count; ++i ) {
		TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
			auto definitionAllocator = makeStackAllocator( GlobalScrap, kilobytes( 10 ) );
			auto filename  = Files[i];
			auto dest      = &result.data[i];

			auto definition = loadSkeletonDefinition( &definitionAllocator, filename, 0 );
			if( !definition ) {
				LOG( ERROR, "{}: Failed to load projectile skeleton definition", filename );
				// TODO: display error message and kill program
				break;
			}

			auto hitboxes = definition.baseHitboxes;
			auto collision =
			    getHitboxIndex( &definition, "collision", SkeletonHitboxState::Collision );
			auto hitbox  = getHitboxIndex( &definition, "hitbox", SkeletonHitboxState::Hitbox );
			auto hurtbox = getHitboxIndex( &definition, "hurtbox", SkeletonHitboxState::Hurtbox );

			if( !collision || !hitbox || !definition.baseVisuals.size()
			    || !definition.baseNodes.size()
			    || ( getProjectileTraits( (ProjectileType)i )->flags.hasHurtbox && !hurtbox ) ) {

				LOG( ERROR, "{}: Invalid projectile skeleton", filename );
				// TODO: display error message and kill program
				break;
			}

			dest->collision        = hitboxes[collision.get()].relative;
			dest->collision.top    = -dest->collision.top;
			dest->collision.bottom = -dest->collision.bottom;
			dest->hitbox           = hitboxes[hitbox.get()].relative;
			dest->hitbox.top       = -dest->hitbox.top;
			dest->hitbox.bottom    = -dest->hitbox.bottom;
			if( hurtbox ) {
				dest->hurtbox        = hitboxes[hurtbox.get()].relative;
				dest->hurtbox.top    = -dest->hurtbox.top;
				dest->hurtbox.bottom = -dest->hurtbox.bottom;
			}
			auto visual     = definition.baseVisuals[0];
			auto collection = &definition.voxels[visual.assetIndex];
			auto range      = collection->animations[visual.animation].range;
			if( !range || !collection->texture ) {
				LOG( ERROR, "{}: Invalid projectile skeleton", filename );
				// TODO: display error message and kill program
				break;
			}
			dest->texture = collection->texture;
			dest->frame   = collection->frames[range.min];
			if( !dest->frame.mesh ) {
				LOG( ERROR, "{}: Invalid projectile skeleton", filename );
				// TODO: display error message and kill program
				break;
			}
			dest->z = definition.baseNodes[0].translation.z;
		}
	}
	return result;
};

bool emitProjectile( GameState* game, vec2arg origin, vec2arg direction, ProjectileType type,
                     EntityTeam team )
{
	assert( floatEq( length( direction ), 1 ) );
	// TODO: emit different types of projectiles based on upgrades
	auto system = &game->projectileSystem;
	if( system->entries.remaining() ) {
		auto projectile = system->entries.emplace_back();
		auto traits     = getProjectileTraits( type );

		*projectile = {
		    origin,                                   // position
		    direction * traits->initialSpeed,         // velocity
		    direction * traits->acceleration,         // acceleration
		    {},                                       // positionDelta
		    addEntityHandle( &game->entityHandles ),  // handle
		    type,                                     // type
		    traits->durability,                       // durability
		    team,                                     // team
		    traits->alive,                            // aliveCountdown
		};
		return true;
	}
	return false;
}

void processProjectiles( GameState* game, TileGrid grid, Array< Entity > dynamics, float dt )
{
	using namespace GameConstants;

	static bool grounded = false;
	static float yStart  = 0;
	static float yMax    = 0;

	auto system = &game->projectileSystem;
	FOR( entry : system->entries ) {
		auto traits        = getProjectileTraits( entry.type );
		auto data          = getProjectileData( system, entry.type );
		auto oldPosition   = entry.position;

		auto alive           = entry.aliveCountdown;
		entry.aliveCountdown = processTimer( entry.aliveCountdown, dt );
		if( !alive ) {
			continue;
		}

		entry.velocity += entry.acceleration;
		entry.velocity.y += Gravity * traits->gravityModifier;
		entry.velocity.y -= traits->airFrictionCoeffictient * entry.velocity.y;

		const auto maxSpeed = traits->maxSpeed;
		if( !floatEqZero( maxSpeed.x ) ) {
			entry.velocity.x = clamp( entry.velocity.x, -maxSpeed.x, maxSpeed.x );
		}
		if( !floatEqZero( maxSpeed.y ) ) {
			entry.velocity.y = clamp( entry.velocity.y, -maxSpeed.y, maxSpeed.y );
		}

		float remaining = min( 1.0f, alive.value );
		auto velocity   = entry.velocity * dt;

		auto tileGridRegion = getSweptTileGridRegion( data->collision, entry.position, velocity );
		constexpr const auto maxIterations = 4;
		for( auto iterations = 0; iterations < maxIterations && remaining > 0.0f; ++iterations ) {
			const auto collision = findCollision( data->collision, entry.position, velocity, grid,
			                                      tileGridRegion, dynamics, remaining, false );

			const auto& info = collision.info;
			if( collision ) {
				// resolve collision
				if( info.t > 0 ) {
					entry.position += velocity * info.t + info.normal * SafetyDistance;
					remaining -= info.t;
				} else {
					entry.position += info.push + info.normal * SafetyDistance;
				}

				// respond to collision
				ProjectileCollisionResponse response = {};
				int32 component                      = VectorComponent_X;
				if( !floatEqZero( info.normal.x ) ) {
					// collision with vertical edge
					response  = traits->flags.leftRightResponse;
					component = VectorComponent_X;
				} else {
					assert( info.normal.y != 0 );
					// collision with horizontal edge
					response = ( info.normal.y < 0 ) ? ( traits->flags.upResponse )
					                                 : ( traits->flags.downResponse );
					component = VectorComponent_Y;
					if( response == traits->flags.upResponse ) {
						if( grounded ) {
							debugLogln( "{}", yMax );
							yMax = 0;
						}
						grounded = true;
						yStart = entry.position.y;
					}
				}
				switch( response ) {
					case ProjectileCollisionResponse::Dissipate: {
						entry.aliveCountdown = {};
						remaining            = 0;
						break;
					}
					case ProjectileCollisionResponse::Bounce: {
						entry.velocity -= traits->bounceModifier * info.normal
						                  * dot( info.normal, entry.velocity );
						velocity -=
						    traits->bounceModifier * info.normal * dot( info.normal, velocity );
						break;
					}
					case ProjectileCollisionResponse::BounceWithConstantSpeed: {
						assert( info.normal.elements[component] != 0 );
						assert( floatEq( abs( info.normal.elements[component] ), 1 ) );
						auto value = info.normal.elements[component]
						             * traits->bounceSpeed.elements[component];
						entry.velocity.elements[component] = value;

						// we set remaining to zero to break out of this collision detection loop
						// because we want to bounce with a constant speed. If we didn't break out
						// here, we get variable bounce heights, defeating the purpose of this
						// response type
						remaining = 0;
						break;
					}
						InvalidDefaultCase;
				}
			} else {
				entry.position += velocity * remaining;
				remaining = 0;
			}
		}
		if( remaining > 0.0f ) {
			entry.position += velocity * remaining;
		}

		if( grounded ) {
			yMax = max( yStart - entry.position.y, yMax );
		}

		entry.positionDelta = entry.position - oldPosition;
	}

	TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
		auto handles = beginVector( GlobalScrap, EntityHandle );
		erase_if( system->entries, [&]( const Projectile& entry ) {
			if( !entry.aliveCountdown ) {
				emitParticles( &game->particleSystem, entry.position,
				               ParticleEmitterId::SmallDissipate );
			}
			auto dead = !entry.aliveCountdown
			            || !isPointInside( GameConstants::PlayableArea, entry.position );
			if( dead ) {
				handles.push_back( entry.handle );
			}
			return dead;
		} );
		removeEntities( &game->hitboxSystem, makeArrayView( handles ) );
	}
}