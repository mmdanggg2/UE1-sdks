/*=============================================================================
	UnThreadingBaseUnix.h: Unreal Tournament Threading Support
	Copyright 2021-2022 OldUnreal. All Rights Reserved.

	Revision history:
		* Created by Stijn Volckaert
=============================================================================*/

#ifndef _UNTHREADINGUNIX_H
#define _UNTHREADINGUNIX_H

/*----------------------------------------------------------------------------
	Unix-specific classes
----------------------------------------------------------------------------*/

class FCriticalSectionUnix : public FCriticalSection
{
	pthread_mutex_t CriticalSection;

public:
	FCriticalSectionUnix();
	virtual ~FCriticalSectionUnix();
	virtual void Lock();
	virtual void Unlock();
};

class FEventUnix : public FEvent
{
	pthread_cond_t Event;
	pthread_mutex_t _Lock;
	UBOOL Initialized{};
	volatile UBOOL Triggered{};
	UBOOL ManualReset{};

protected:
	static INT WaitForEventHelper(FEventUnix* Event, DWORD WaitTime);

public:
	FEventUnix();
	virtual ~FEventUnix();
	virtual void Lock();
	virtual void Unlock();
	virtual UBOOL Create(UBOOL bIsManualReset = FALSE);
	virtual void Trigger();
	virtual void Reset();
	virtual void Pulse();
	virtual UBOOL Wait(DWORD WaitTime = INFINITE);
};

class CORE_API FSynchronizationFactoryUnix : public FSynchronizationFactory
{
public:
	FSynchronizationFactoryUnix();
	virtual FCriticalSection* CreateCriticalSection();
	virtual FEvent* CreateSynchEvent(UBOOL bIsManualReset = FALSE);
	virtual void Destroy(FSynchronize* SyncObject);
};

class FRunnableThreadUnix : public FRunnableThread
{
	pthread_t Thread;
	FRunnable* Runnable;
	UBOOL bShouldDeleteSelf;
	UBOOL bShouldDeleteRunnable;
	UBOOL bThreadJoined;
	FEventUnix* ThreadCompleted;
	static void* ThreadProc(void* Params);
	void* Run();

public:
	FRunnableThreadUnix();
	~FRunnableThreadUnix();
	UBOOL Create(FRunnable* InRunnable, UBOOL bAutoDeleteSelf = FALSE, UBOOL bAutoDeleteRunnable = FALSE, DWORD StackSize = 0);
	virtual void Suspend(UBOOL bShouldPause = TRUE);
	virtual UBOOL Kill(UBOOL bShouldWait = FALSE, DWORD MaxWaitTime = 0);
	virtual void WaitForCompletion();
};

class FThreadFactoryUnix : public FThreadFactory
{
public:
	virtual FRunnableThread* CreateThread(FRunnable* InRunnable, UBOOL bAutoDeleteSelf = FALSE, UBOOL bAutoDeleteRunnable = FALSE, DWORD StackSize = 0);
	virtual void Destroy(FRunnableThread* Thread);
};

#endif
