enum class ProjectileType : int8 {
	Hero,
	Wheels,

	Count
};

enum class ProjectileCollisionResponse : uint8 {
	Dissipate,
	Bounce,
	BounceWithConstantSpeed,
};

struct ProjectileTraits {
	float initialSpeed;
	vec2 maxSpeed;
	vec2 bounceSpeed;  // only used if collision response is BounceWithConstantSpeed
	float acceleration;
	float alive;
	float gravityModifier;
	float airFrictionCoeffictient;
	float bounceModifier;
	int8 durability;
	struct {
		uint8 hasHurtbox : 1;
		ProjectileCollisionResponse leftRightResponse : 2;
		ProjectileCollisionResponse upResponse : 2;
		ProjectileCollisionResponse downResponse : 2;
	} flags;
};

struct ProjectileData {
	rectf collision;
	rectf hitbox;
	rectf hurtbox;

	TextureId texture;
	VoxelCollection::Frame frame;
	float z;
};

struct Projectile {
	vec2 position;
	vec2 velocity;
	vec2 acceleration;
	vec2 positionDelta;
	EntityHandle handle;
	ProjectileType type;
	int8 durability;
	EntityTeam team;
	bool8 deflected;
	CountdownTimer aliveCountdown;
};

struct ProjectileSystem {
	ProjectileData data[valueof( ProjectileType::Count )];
	UArray< Projectile > entries;
};