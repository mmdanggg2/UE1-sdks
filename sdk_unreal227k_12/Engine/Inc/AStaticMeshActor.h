	AStaticMeshActor() {}

	FStaticLightActor* GetStaticLights();
	void Modify();
	void PostEditMove();
	BYTE HasSlowRayTrace();
	void PostRaytrace(FThreadLock& Mutex);
	void DeleteData();
	UBOOL IsValidOnImport();
	void PostScriptDestroyed();
	void InitActorZone();
	UBOOL ShouldExportProperty(UProperty* Property) const;
