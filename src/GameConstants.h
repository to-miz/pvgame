namespace GameConstants
{
constexpr const float Gravity                       = 0.1f;
constexpr const float MovementSpeed                 = 1.1f;
constexpr const float JumpingSpeed                  = -3.3f;
constexpr const float WalljumpingSpeed              = -3.3f;
constexpr const float WalljumpMaxDuration           = 8;
constexpr const float WalljumpFixDuration           = 4;
constexpr const float WalljumpWindowDuration        = 10;
constexpr const float WalljumpMoveThreshold         = WalljumpMaxDuration - WalljumpFixDuration;
constexpr const float WallslideFrictionCoefficient  = 0.05f;
constexpr const float DefaultAirFrictionCoefficient = 0.025f;

constexpr const float TileWidth  = 16.0f;
constexpr const float TileHeight = 16.0f;
constexpr const float TileDepth = 16.0f;

constexpr const float DeltaToFrameTime = 60.0f / 1000.0f;  // constant to convert elapsed
                                                           // miliseconds to number between 0 and 1,
                                                           // where 1 = 1/60 of a second

const rectf PlayableArea = {-1000, -1000, 1000, 1000};
}