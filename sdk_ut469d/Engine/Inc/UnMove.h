class ENGINE_API ASavedMove : public AInfo
{
public:
    class ASavedMove* NextMove;
    FLOAT TimeStamp;
    FLOAT Delta;
    BITFIELD bRun : 1;
    BITFIELD bDuck : 1;
    BITFIELD bPressedJump : 1;
    BITFIELD bFire : 1;
    BITFIELD bAltFire : 1;
    BITFIELD bForceFire : 1;
    BITFIELD bForceAltFire : 1;
    BYTE DodgeMove GCC_PACK(INT_ALIGNMENT);
    FVector SavedLocation;
    FVector SavedVelocity;
    FRotator SavedViewRotation;
    FLOAT OrigTimeStamp;
    FVector OrigVelocity;
    DECLARE_CLASS(ASavedMove, AInfo, 0, Engine)
    NO_DEFAULT_CONSTRUCTOR(ASavedMove)
};