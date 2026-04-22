
void C3DReinitialiseIfNecessary(AActor*);

struct C3DTRMATRIX
{
	class FVector m[0x4];
	DWORD flags;
};

struct C3DMeshStateThing {
	char* actorName;
	INT unk2;
	void* state;  // points to the UC3DMESHSTATE member at offset after inherited UObject (0x28) ??
	void* unk4;
	INT unk5;
	void* unk6;
	INT unk7;
	C3DTRMATRIX matrix;
};
