struct SkeletonTransform {
	vec3 translation;
	vec3 rotation;
	vec3 scale;
	Color flashColor;
	float length;
	int16 parent;
	int16 id;
};
enum class SkeletonVisualType : int8 { None, Voxel };
struct SkeletonVoxelVisuals {
	static const AnimatorAsset::Type AssetType = AnimatorAsset::type_collection;

	int16 assetIndex;
	int16 index;
	int8 animation;
	int8 frame;
	int16 id;
};
struct SkeletonEmitterState {
	static const AnimatorAsset::Type AssetType = AnimatorAsset::type_emitter;

	int16 assetIndex;
	int16 index;
	int16 id;
	bool8 active;
	float time;
};
struct SkeletonHitboxState {
	rectf relative;
	vec3 origin;
	int16 assetIndex;
	int16 index;
	int16 id;
	bool8 active;
	enum Type : int8 { Collision, Hitbox, Hurtbox } type;
};
struct SkeletonKeyframesState {
	int16 translation;
	int16 rotation;
	int16 scale;
	int16 flashColor;
	int16 custom;
};
struct SkeletonAnimationState {
	enum : uint16 {
		Repeating = BITFIELD( 0 ),
		Playing   = BITFIELD( 1 ),
	};
	int16 animationIndex;
	uint16 flags;
	float prevFrame;
	float currentFrame;
	float blendFactor;
	Array< SkeletonKeyframesState > keyframeStates;
	int16 prevEvent;
	int16 event;
};
struct SkeletonWorldTransform {
	mat4 transform;
	Color flashColor;
};
struct SkeletonDefinition;
struct Skeleton {
	Array< SkeletonTransform > transforms;
	Array< SkeletonWorldTransform > worldTransforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< SkeletonEmitterState > emitters;
	Array< const VoxelCollection* > voxels;
	Array< SkeletonHitboxState > hitboxes;
	UArray< SkeletonAnimationState > animations;
	Array< Array< SkeletonKeyframesState > > keyframeStatesPool;
	int16 definitionId;
	bool8 dirty;
	bool8 mirrored;
	const SkeletonDefinition* definition;

	SkeletonWorldTransform rootTransform;

	void* base;
};
void setTransform( Skeleton* skeleton, mat4arg transform );
void setMirrored( Skeleton* skeleton, bool mirrored );

enum class SkeletonEaseType : int8 {
	Lerp,
	Step,
	Smoothstep,
	EaseOutBounce,
	EaseOutElastic,
	Curve,
};
template< class T >
struct SkeletonKeyframe {
	typedef T value_type;

	float t;
	T data;
	SkeletonEaseType easeType;
	int16 curveIndex;
};
struct SkeletonEvent {
	float t;
	SkeletonEventType type;
};
struct SkeleonKeyframes {
	Array< SkeletonKeyframe< vec3 > > translation;
	Array< SkeletonKeyframe< vec3 > > rotation;
	Array< SkeletonKeyframe< vec3 > > scale;
	Array< SkeletonKeyframe< Color > > flashColor;
	Array< SkeletonKeyframe< int8 > > frame;
	Array< SkeletonKeyframe< bool8 > > active;
	int16 id;
	bool8 hasKeyframes;
};
struct SkeletonAnimation {
	Array< SkeletonTransform > transforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< SkeletonEmitterState > emitters;
	Array< SkeletonHitboxState > hitboxes;
	Array< SkeleonKeyframes > keyframes;
	Array< SkeletonEvent > events;
	Array< BezierForwardDifferencerData > curves;
	float duration;
};
struct SkeletonId {
	int16 id;
	int16 nameLength;
	int16 index;
	char name[25];

	StringView getName() const { return {name, nameLength}; }
};
struct SkeletonAssetId {
	int16 id;
	int16 assetIndex;
	int16 nameLength;
	AnimatorAsset::Type type;
	char name[10];

	StringView getName() const { return {name, nameLength}; }
};
struct SkeletonDefinition {
	Array< SkeletonAnimation > animations;
	Array< SkeletonTransform > baseNodes;
	Array< SkeletonVoxelVisuals > baseVisuals;
	Array< SkeletonEmitterState > baseEmitters;
	Array< SkeletonHitboxState > baseHitboxes;
	Array< SkeletonId > animationIds;
	Array< SkeletonId > nodeIds;
	Array< SkeletonAssetId > assetIds;

	Array< VoxelCollection > voxels;
	Array< AnimatorParticleEmitter > emitters;
	Array< rectf > hitboxes;
	int16 id;

	int32 sizeInBytes;
	inline explicit operator bool() const { return id >= 0; };
};

#define GenerateNames( type, prefix, ... )                  \
	const StringView type##prefix[]        = {__VA_ARGS__}; \
	Array< const StringView > type::prefix = makeArrayView( type##prefix );
#define GenerateEmpty( type, prefix ) Array< const StringView > type::prefix = {};
template < class T >
Array< int8 > makeIndicesArray( T& entry )
{
	static_assert( alignof( T ) == alignof( int8 ), "T is not properly aligned" );
	static_assert( std::is_pod< T >::value, "T is not pod" );
	static_assert( ( sizeof( T ) / sizeof( int8 ) ) * sizeof( int8 ) == sizeof( T ),
	               "Cannot turn T into an array of int8" );
	return makeArrayView( (int8*)&entry, sizeof( T ) / sizeof( int8 ) );
};
Array< int8 > makeIndicesArray( nullptr_t )
{
	return {};
};
#define GenerateMembers( animationIds, nodeIds, collisionIds, hitboxIds, hurtboxIds ) \
	Array< int8 > getAnimationIds() { return makeIndicesArray( animationIds ); }      \
	Array< int8 > getNodeIds() { return makeIndicesArray( nodeIds ); }                \
	Array< int8 > getCollisionIds() { return makeIndicesArray( collisionIds ); }      \
	Array< int8 > getHitboxIds() { return makeIndicesArray( hitboxIds ); }            \
	Array< int8 > getHurtboxIds() { return makeIndicesArray( hurtboxIds ); }          \
	static Array< const StringView > AnimationNames;                                  \
	static Array< const StringView > NodeNames;                                       \
	static Array< const StringView > CollisionNames;                                  \
	static Array< const StringView > HitboxNames;                                     \
	static Array< const StringView > HurtboxNames;

struct HeroSkeletonDefinition {
	SkeletonDefinition* definition; // if this is null, definition is not loaded

	// these ids need to be contiguous
	// must be same order as AnimationNames (see GenerateNames below)
	struct {
		int8 turn;
		int8 landing;

		int8 idle;
		int8 walk;
		int8 jumpRising;
		int8 jumpFalling;
		int8 wallslide;

		int8 idleShoot;
		int8 walkShoot;
		int8 jumpRisingShoot;
		int8 jumpFallingShoot;
		int8 wallslideShoot;

		int8 hurt;
	} animationIds;

	struct {
		int8 shootPos;
		int8 feetPos;
	} nodeIds;

	struct {
		int8 collision;
	} collisionIds;

	struct {
		int8 hurtbox;
	} hurtboxIds;

	GenerateMembers( animationIds, nodeIds, collisionIds, nullptr, hurtboxIds );
};
GenerateNames( HeroSkeletonDefinition, AnimationNames, "Turn", "Landing", "Idle", "Walk",
               "JumpRising", "JumpFalling", "Wallslide", "IdleShoot", "WalkShoot",
               "JumpRisingShoot", "JumpFallingShoot", "WallslideShoot", "hurt" );
GenerateNames( HeroSkeletonDefinition, NodeNames, "shoot_pos", "feet_pos" );
GenerateNames( HeroSkeletonDefinition, CollisionNames, "collision" );
GenerateEmpty( HeroSkeletonDefinition, HitboxNames );
GenerateNames( HeroSkeletonDefinition, HurtboxNames, "hurtbox" );

struct WheelsSkeletonDefinition {
	SkeletonDefinition* definition;

	struct {
		int8 move;
		int8 attack;
		int8 hurt;
		int8 turn;
	} animationIds;

	struct {
		int8 attackOrigin;
	} nodeIds;

	struct {
		int8 collision;
	} collisionIds;

	struct {
		int8 hurtbox;
	} hurtboxIds;

	GenerateMembers( animationIds, nodeIds, collisionIds, nullptr, hurtboxIds );
};
GenerateNames( WheelsSkeletonDefinition, AnimationNames, "move", "attack", "hurt", "turn" );
GenerateNames( WheelsSkeletonDefinition, NodeNames, "attack_origin" );
GenerateNames( WheelsSkeletonDefinition, CollisionNames, "root" );
GenerateEmpty( WheelsSkeletonDefinition, HitboxNames );
GenerateNames( WheelsSkeletonDefinition, HurtboxNames, "hurtbox" );

struct ProjectileSkeletonDefinition {
	SkeletonDefinition* definition;
	struct {
		int8 collision;
	} collisionIds;

	struct {
		int8 hitbox;
	} hitboxIds;

	GenerateMembers( nullptr, nullptr, collisionIds, hitboxIds, nullptr );
};
GenerateEmpty( ProjectileSkeletonDefinition, AnimationNames );
GenerateEmpty( ProjectileSkeletonDefinition, NodeNames );
GenerateNames( ProjectileSkeletonDefinition, CollisionNames, "collision" );
GenerateNames( ProjectileSkeletonDefinition, HitboxNames, "hitbox" );
GenerateEmpty( ProjectileSkeletonDefinition, HurtboxNames );

struct EntitySkeletonTraits {
	const SkeletonDefinition* definition;
	int8 collisionIdsData[8];
	int8 hitboxIdsData[8];
	int8 hurtboxIdsData[8];
	int8 hitboxesCounts[3];

	Array< const int8 > collisionIds() const
	{
		return makeArrayView( collisionIdsData, hitboxesCounts[0] );
	};
	Array< const int8 > hitboxIds() const
	{
		return makeArrayView( hitboxIdsData, hitboxesCounts[1] );
	};
	Array< const int8 > hurtboxIds() const
	{
		return makeArrayView( hurtboxIdsData, hitboxesCounts[2] );
	};
};

struct SkeletonSystem {
	UArray< Skeleton* > skeletons;
	UArray< SkeletonDefinition > definitions;

	EntitySkeletonTraits skeletonTraits[Entity::type_count];
	HeroSkeletonDefinition hero;
	WheelsSkeletonDefinition wheels;
	ProjectileSkeletonDefinition projectile;
};

const EntitySkeletonTraits* const getSkeletonTraits( SkeletonSystem* system, Entity::Type type )
{
	assert( type >= 0 && type < Entity::type_count );
	assert( type != Entity::type_projectile );
	return &system->skeletonTraits[type];
}