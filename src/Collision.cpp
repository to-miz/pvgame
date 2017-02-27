const float SafetyDistance = 0.01f;

bool testPointVsAxisAlignedLineSegment( float x, float y, float deltaX, float deltaY,
                                        float segmentX, float segmentStartY, float segmentEndY,
                                        float* t )
{
	*t                 = 0;
	auto relativePos   = segmentX - x;
	auto intersectionT = relativePos / deltaX;
	auto intersectionY = y + deltaY * intersectionT;
	if( intersectionT > 0 && intersectionY >= segmentStartY && intersectionY < segmentEndY ) {
		*t = intersectionT;
		return true;
	}
	return false;
}
struct CollisionInfo {
	float t;
	vec2 normal;
	vec2 push;
};
struct PushPair {
	float push;
	vec2 normal;
	friend bool operator<( const PushPair& a, const PushPair& b ) { return a.push < b.push; }
};
bool testAabVsAab( rectfarg a, vec2arg aPosition, vec2arg delta, rectfarg b, float maxT,
                   CollisionInfo* info )
{
	rectf sum = {b.left - a.right + SafetyDistance, b.top - a.bottom + SafetyDistance,
	             b.right - a.left - SafetyDistance, b.bottom - a.top - SafetyDistance};

	float intermediate;
	bool collided = false;
	if( isPointInside( sum, aPosition ) ) {
		PushPair leftPush       = {aPosition.x - sum.left, -1, 0};
		PushPair upPush         = {aPosition.y - sum.top, 0, -1};
		PushPair rightPush      = {sum.right - aPosition.x, 1, 0};
		PushPair downPush       = {sum.bottom - aPosition.y, 0, 1};
		const PushPair& minPush = min( min( leftPush, upPush ), min( rightPush, downPush ) );
		info->t                 = -1;
		info->normal            = minPush.normal;
		info->push              = minPush.push * minPush.normal;
		collided                = true;
		if( ( minPush.normal.x < 0 && delta.x < info->push.x )
		    || ( minPush.normal.x > 0 && delta.x > info->push.x )
		    || ( minPush.normal.y < 0 && delta.y < info->push.y )
		    || ( minPush.normal.y > 0 && delta.y > info->push.y ) ) {
			// if we are moving away from collision faster than the push, ignore it
			// the collision will be resolved by just letting the point move
			collided = false;
		}
	} else {
		if( delta.x != 0 ) {
			if( testPointVsAxisAlignedLineSegment( aPosition.x, aPosition.y, delta.x, delta.y,
			                                       sum.left, sum.top, sum.bottom,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {-1, 0};
					collided     = true;
				}
			}
			if( testPointVsAxisAlignedLineSegment( aPosition.x, aPosition.y, delta.x, delta.y,
			                                       sum.right, sum.top, sum.bottom,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {1, 0};
					collided     = true;
				}
			}
		}

		if( delta.y != 0 ) {
			if( testPointVsAxisAlignedLineSegment( aPosition.y, aPosition.x, delta.y, delta.x,
			                                       sum.top, sum.left, sum.right, &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {0, -1};
					collided     = true;
				}
			}
			if( testPointVsAxisAlignedLineSegment( aPosition.y, aPosition.x, delta.y, delta.x,
			                                       sum.bottom, sum.left, sum.right,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {0, 1};
					collided     = true;
				}
			}
		}
	}

	return collided;
}

struct CollisionResult {
	CollidableRef collision;
	CollisionInfo info;
	inline explicit operator bool() const { return static_cast< bool >( collision ); }
};
static CollisionResult detectCollisionVsTileGrid( rectfarg aab, vec2arg position, vec2arg velocity,
                                                  TileGrid grid, recti tileGridRegion, float maxT )
{
	using namespace GameConstants;

	CollisionResult result = {};
	result.info.t          = maxT;
	for( auto y = tileGridRegion.top; y < tileGridRegion.bottom; ++y ) {
		for( auto x = tileGridRegion.left; x < tileGridRegion.right; ++x ) {
			auto index = grid.index( x, y );
			auto tile  = grid[index];
			if( tile ) {
				rectf tileBounds   = RectWH( x * TileWidth, y * TileHeight, TileWidth, TileHeight );
				CollisionInfo info = {};
				if( testAabVsAab( aab, position, velocity, tileBounds, result.info.t, &info ) ) {
					if( info.t < result.info.t ) {
						result.collision.setTile( index );
						result.info = info;
					}
				}
			}
		}
	}
	return result;
}
static CollisionResult detectCollisionVsDynamics( rectfarg aab, vec2arg position, vec2arg velocity,
                                                  Array< Entity > dynamics,
                                                  float maxT )
{
	using namespace GameConstants;

	CollisionResult result = {};
	result.info.t          = maxT;
	FOR( dynamic : dynamics ) {
		CollisionInfo info = {};
		if( testAabVsAab( aab, position, velocity, translate( dynamic.aab, dynamic.position ),
		                  result.info.t, &info ) ) {
			if( info.t < result.info.t ) {
				result.collision.setDynamic( indexof( dynamics, dynamic ), dynamic.handle );
				result.info = info;
			}
		}
	}
	return result;
}

static recti getSweptTileGridRegion( rectfarg aab, vec2arg position, vec2arg velocity,
                                     rectiarg mapBounds )
{
	using namespace GameConstants;
	auto currentPlayerAab = translate( aab, position );
	// sweep the bounding box along collidable velocity to get bounding box of the sweep region
	auto entrySweptAab = sweep( currentPlayerAab, velocity );
	// turn the swept bounding box to tile grid region that we need to check for collisions
	// tiles outside this region can't possibly collide with the collidable
	auto tileGridRegion = RectTiledIndex( entrySweptAab, TileWidth, TileHeight );
	tileGridRegion      = RectMin( tileGridRegion, mapBounds );
	return tileGridRegion;
}

CollisionResult findCollision( rectfarg aab, vec2arg position, vec2arg velocity, TileGrid grid,
                               recti tileGridRegion, Array< Entity > dynamics, float maxT,
                               bool dynamic )
{
	const recti mapBounds = {0, 0, grid.width, grid.height};
	auto collision =
	    detectCollisionVsTileGrid( aab, position, velocity, grid, tileGridRegion, maxT );
	if( !dynamic && ( !collision || collision.info.t > 0 ) ) {
		auto dynamicCollision =
		    detectCollisionVsDynamics( aab, position, velocity, dynamics, maxT );
		if( !collision || ( dynamicCollision && dynamicCollision.info.t < collision.info.t ) ) {
			if( dynamicCollision.info.t < 0 ) {
				// calculate max movement in given push direction by doing another round of
				// tile grid collision detection
				auto safety      = dynamicCollision.info.normal * SafetyDistance;
				auto adjustedVel = dynamicCollision.info.push + safety;
				auto sweptRegion = getSweptTileGridRegion( aab, position, adjustedVel, mapBounds );
				auto maxMovementCollision =
				    detectCollisionVsTileGrid( aab, position, adjustedVel, grid, sweptRegion, 1 );
				if( maxMovementCollision ) {
					if( maxMovementCollision.info.t > 0 ) {
						// TODO: we are being squished between dynamic and tile, handle?

						collision      = dynamicCollision;
						auto newSafety = maxMovementCollision.info.normal * SafetyDistance;
						// recalculate push by taking into account the old and new
						// safetyDistances
						collision.info.push =
						    adjustedVel * maxMovementCollision.info.t - safety + newSafety;
					} else {
						// TODO: we are being squished between dynamic and tile, handle?
					}
				} else {
					collision = dynamicCollision;
				}
			} else {
				collision = dynamicCollision;
			}
		}
	}
	return collision;
}

// TODO: rename function, since this does way more than colliding
void processCollidables( Array< Entity > entries, TileGrid grid,
                         Array< TileInfo > infos, Array< Entity > dynamics,
                         bool dynamic, float dt )
{
	using namespace GameConstants;
	constexpr const float eps = 0.00001f;

	const recti MapBounds = {0, 0, grid.width, grid.height};
	// TODO: air movement should be accelerated instead of instant
	FOR( entry : entries ) {
		auto traits      = getEntityTraits( entry.type );
		auto oldPosition = entry.position;

		entry.walljumpWindow   = processTimer( entry.walljumpWindow, dt );
		entry.walljumpDuration = processTimer( entry.walljumpDuration, dt );
		auto alive             = entry.aliveCountdown;
		entry.aliveCountdown   = processTimer( entry.aliveCountdown, dt );
		if( alive && isCountdownTimerExpired( entry.aliveCountdown ) ) {
			entry.flags.deathFlag = true;
		}

		// make entry move away from the wall if a walljump was executed
		if( entry.walljumpDuration ) {
			// move away only for 4 frames in total
			if( entry.walljumpDuration.value > WalljumpMoveThreshold ) {
				if( entry.walljumpLeft() ) {
					entry.velocity.x = -MovementSpeed;
				} else {
					entry.velocity.x = MovementSpeed;
				}
			}
		}

		// update
		entry.prevVeloctiy = entry.velocity;
		// no need for dt multiplication, because we are on frame boundary
		entry.velocity += entry.acceleration;
		if( !entry.grounded || traits->movement != EntityMovement::Grounded ) {
			entry.velocity.y += Gravity * entry.gravityModifier;
			entry.velocity.y -= entry.airFrictionCoeffictient * entry.velocity.y;
			if( entry.wallslideCollidable && entry.velocity.y > 0 ) {
				// apply wallslide friction only if falling
				auto friction = getFrictionCoefficitonFromCollidableRef(
				    dynamics, grid, infos, entry.wallslideCollidable );
				entry.velocity.y -= friction * WallslideFrictionCoefficient * entry.velocity.y;
			}
		}
		if( !floatEqZero( entry.maxSpeed.x ) ) {
			entry.velocity.x = clamp( entry.velocity.x, -entry.maxSpeed.x, entry.maxSpeed.x );
		}
		if( !floatEqZero( entry.maxSpeed.y ) ) {
			entry.velocity.y = clamp( entry.velocity.y, -entry.maxSpeed.y, entry.maxSpeed.y );
		}
		auto oldVelocity = entry.velocity;

		processSpatialState( &entry, dt );

		auto isGroundBased = traits->movement == EntityMovement::Grounded
		                     && floatEqSoft( entry.bounceModifier, 1.0f );

		// collision detection begins here
		auto velocity = entry.velocity * dt;

		// broadphase
		// get the region of tiles that we actually touch when moving along velocity
		auto tileGridRegion =
		    getSweptTileGridRegion( entry.aab, entry.position, velocity, MapBounds );

		// check grounded state of entry
		if( entry.grounded ) {
			switch( entry.grounded.type ) {
				case CollidableRef::None: {
					break;
				}
				case CollidableRef::Tile: {
					assert_init( auto tile = grid[entry.grounded.index], tile );
					auto p = grid.coordinatesFromIndex( entry.grounded.index );
					auto tileBounds =
					    RectWH( p.x * TileWidth, p.y * TileHeight, TileWidth, TileHeight );
					CollisionInfo info = {};
					if( !testAabVsAab( entry.aab, entry.position, {0, 1}, tileBounds, 1, &info )
					    || info.t > SafetyDistance + eps || info.t < 0 ) {
						entry.grounded.clear();
					}
					break;
				}
				case CollidableRef::Dynamic: {
					auto other         = getDynamicFromCollidableRef( dynamics, entry.grounded );
					CollisionInfo info = {};
					if( !other
					    || !testAabVsAab( entry.aab, entry.position, {0, 1},
					                      translate( other->aab, other->position ), 1, &info )
					    || info.t > SafetyDistance + eps || info.t < 0 ) {
						entry.grounded.clear();
					}
					break;
				}
					InvalidDefaultCase;
			}
			if( !entry.grounded ) {
				// try and find a static entry as new ground
				auto findNewStaticGround = [&]() {
					auto bottom = MIN( tileGridRegion.bottom + 1, MapBounds.bottom );
					for( int32 y = tileGridRegion.top; y < bottom; ++y ) {
						for( int32 x = tileGridRegion.left; x < tileGridRegion.right; ++x ) {
							auto index = grid.index( x, y );
							auto tile  = grid[index];
							if( tile ) {
								auto tileBounds =
								    RectWH( x * TileWidth, y * TileHeight, TileWidth, TileHeight );
								CollisionInfo info = {};
								if( testAabVsAab( entry.aab, entry.position, {0, 1}, tileBounds, 1,
								                  &info ) ) {
									if( info.normal.y < 0 && info.t >= 0
									    && info.t < SafetyDistance + eps ) {
										entry.position.y += info.t - SafetyDistance;
										entry.grounded.setTile( index );
										return;
									}
								}
							}
						}
					}
				};
				findNewStaticGround();
			}
			if( !entry.grounded ) {
				// try and find a dynamic entry as new ground
				auto findNewDynamicGround = [&]() {
					FOR( other : dynamics ) {
						if( &entry == &other ) {
							continue;
						}
						CollisionInfo info = {};
						if( testAabVsAab( entry.aab, entry.position, {0, 1},
						                  translate( other.aab, other.position ), 1, &info ) ) {
							if( info.normal.y < 0 && info.t >= 0
							    && info.t < SafetyDistance + eps ) {
								entry.position.y += info.t - SafetyDistance;
								entry.grounded.setDynamic( indexof( dynamics, other ),
								                           other.handle );
								return;
							}
						}
					}
				};
				findNewDynamicGround();
			}

			if( !entry.grounded ) {
				setSpatialState( &entry, SpatialState::FallingOff );
			}
		}

		if( entry.grounded && entry.grounded.type == CollidableRef::Dynamic ) {
			if( auto dynamicGround = getDynamicFromCollidableRef( dynamics, entry.grounded ) ) {
				velocity += dynamicGround->positionDelta;
			}
		}
		// recalculate tileGridRegion with new velocity
		tileGridRegion = getSweptTileGridRegion( entry.aab, entry.position, velocity, MapBounds );

		// remaining is the amount of "frame time" we want to move this frame, 1 == full one frame
		// worth of movement. If entry is not alive for the whole frame, we want to move for the
		// amount it will be alive for
		float remaining = 1;
		if( alive ) {
			remaining = min( 1.0f, alive.value );
		}

		entry.lastCollision.clear();
		constexpr const auto maxIterations = 4;
		for( auto iterations = 0; iterations < maxIterations && remaining > 0.0f; ++iterations ) {
			auto collision = findCollision( entry.aab, entry.position, velocity, grid,
			                                tileGridRegion, dynamics, remaining, dynamic );

			auto normal = collision.info.normal;
			auto t      = collision.info.t;
			if( traits->flags.useForcedNormal ) {
				normal = traits->forcedNormal;
				if( collision.info.normal.x < 0 ) {
					normal.x = -normal.x;
				}
				if( collision.info.normal.y < 0 ) {
					normal.y = -normal.y;
				}
			}
			if( t > 0 ) {
				bool processCollision = true;
				if( isGroundBased ) {
					// check whether we just fell off and are trying to get back on top of previous
					// ground
					if( entry.spatialState == SpatialState::FallingOff && !floatEqZero( velocity.x )
					    && velocity.y > 0 ) {

						// TODO: implement
					}
				}

				if( processCollision ) {
					entry.position += velocity * t;
					if( collision ) {
						entry.position += normal * SafetyDistance;
					}
					remaining -= t;
				}
			} else {
				entry.position += collision.info.push + normal * SafetyDistance;
			}

			if( normal.y < 0 ) {
				entry.grounded = collision.collision;
				setSpatialState( &entry, SpatialState::Grounded );
			}

#ifdef GAME_DEBUG
			if( entry.grounded ) {
				debug_Values->groundPosition = entry.position.y;
			}
			if( !entry.grounded && velocity.y < 0 ) {
				debug_Values->jumpHeight = entry.position.y - debug_Values->groundPosition;
			}
			if( normal.y < 0 ) {
				if( debug_Values->jumpHeight < debug_Values->maxJumpHeight ) {
					debug_Values->maxJumpHeight = debug_Values->jumpHeight;
				}
				debug_Values->jumpHeightError =
				    abs( debug_Values->jumpHeight - debug_Values->lastJumpHeight );
				debug_Values->lastJumpHeight = debug_Values->jumpHeight;
			}
#endif  // defined( GAME_DEBUG )

			// response
			if( collision ) {
				entry.lastCollision = collision.collision;
			}
			if( traits->response == CollisionResponse::FullStop ) {
				if( collision ) {
					break;
				}
			} else if( traits->response == CollisionResponse::Bounce ) {
				if( collision ) {
					// reflect velocity based on the collision normal
					entry.velocity -= entry.bounceModifier * normal * dot( normal, entry.velocity );
					velocity -= entry.bounceModifier * normal * dot( normal, velocity );
				}
				if( collision && normal.x != 0 && normal.y == 0 && !entry.grounded
				    && ( ( oldVelocity.x >= 0 ) == ( normal.x < 0 ) ) ) {

					// we are wallsliding
					entry.flags.walljumpLeft  = ( normal.x < 0 );
					entry.walljumpWindow      = {WalljumpWindowDuration};
					entry.wallslideCollidable = collision.collision;
				}
				auto lengthSquared = dot( entry.velocity, entry.velocity );
				if( t >= 0 && ( t <= 0.000001f || lengthSquared < eps ) ) {
					break;
				}
			} else {
				InvalidCodePath();
			}
		}
		if( remaining > 0.0f ) {
			entry.position += velocity * remaining;
		}
		entry.positionDelta = entry.position - oldPosition;

		if( !entry.walljumpWindow ) {
			entry.wallslideCollidable.clear();
		}
		if( entry.grounded ) {
			entry.walljumpWindow = {};
			entry.wallslideCollidable.clear();
		}
		// check that we are upholding safety distance to collidables in velocity direction
		if( isGroundBased && traits->flags.canWalljump ) {
			auto wasAlreadySliding = (bool)entry.wallslideCollidable;
			if( !floatEqZero( entry.velocity.x ) || entry.wallslideCollidable ) {
				vec2 searchDir = {1, 0};
				if( entry.velocity.x < 0 ) {
					searchDir = {-1, 0};
				}
				if( entry.wallslideCollidable ) {
					if( !entry.walljumpLeft() ) {
						searchDir = {-1, 0};
					}
				}
				auto region =
				    getSweptTileGridRegion( entry.aab, entry.position, searchDir, MapBounds );
				auto collision = findCollision( entry.aab, entry.position, searchDir, grid, region,
				                                dynamics, 1, dynamic );
				if( !collision || collision.info.normal.y != 0 ) {
					// clear wallsliding
					entry.walljumpWindow = {};
					entry.wallslideCollidable.clear();
				} else {
					if( !entry.grounded && wasAlreadySliding ) {
						// reset wallsliding
						// TODO: do we want to reset the walljump duration here?
						// that means wallsliding will behave like a toggle (not requiring holding a
						// button down)
						entry.walljumpWindow = {WalljumpWindowDuration};
						entry.wallslideCollidable = collision.collision;
					}
				}
				// make sure that we are SafetyDistance away from the wall we are sliding against
				if( collision && collision.info.t < SafetyDistance ) {
					auto diff = SafetyDistance - collision.info.t;
					entry.position.x += diff * collision.info.normal.x;
					entry.velocity.x = 0;
				}
			}

			// adjust face direction if we are wallsliding
			if( entry.walljumpWindow ) {
				if( entry.walljumpLeft() ) {
					entry.faceDirection = EntityFaceDirection::Left;
				} else {
					entry.faceDirection = EntityFaceDirection::Right;
				}
			}
		}
	}

}

static void doCollisionDetection( Room* room, EntitySystem* system, float dt )
{
	using namespace GameConstants;
	assert( room );

	auto grid = getCollisionLayer( room );

	processCollidables( system->dynamicEntries(), grid, room->tileSet->infos, {}, true, dt );
	processCollidables( system->staticEntries(), grid, room->tileSet->infos,
	                    system->dynamicEntries(), false, dt );
}