/*=============================================================================
	UnProfile.h: Engine Profiling definitions.
	Copyright 1999 Human Head Studios, Inc. All Rights Reserved.

	Revision history:
		* Created by Paul MacArthur

	Example Use:
		PROFILE(NAME_Render);
			PROFILE(NAME_RenderSkeletons);
				PROFILESLOW(NAME_GetFrame);
				UNPROFILESLOW(NAME_GetFrame);
			UNPROFILE(NAME_RenderSkeletons);
		UNPROFILE(NAME_Render);
=============================================================================*/

#define PROFILE(statname)		STAT(GProfile.Start(statname, PROF_LEVEL_NORMAL))
#define UNPROFILE(statname)		STAT(GProfile.Stop (statname, PROF_LEVEL_NORMAL))
#define PROFILESLOW(statname)	STAT(GProfile.Start(statname, PROF_LEVEL_SLOW))
#define UNPROFILESLOW(statname)	STAT(GProfile.Stop (statname, PROF_LEVEL_SLOW))


#define PROF_LEVEL_NORMAL		1
#define PROF_LEVEL_SLOW			2

#define PROF_HISTORY			50
#define NUM_PROF_COLORS			10
#define PROF_STACK_DEPTH		50
#define PROF_NUM_TIMINGS		100
#define PROF_NUM_COLORS			10


enum EProfCommand
{
	PROFCOMMAND_NONE,
	PROFCOMMAND_SELECTPREV,
	PROFCOMMAND_SELECTNEXT,
	PROFCOMMAND_TRAVERSEIN,
	PROFCOMMAND_TRAVERSEOUT,
	PROFCOMMAND_STARTCAPTURE,
	PROFCOMMAND_STOPCAPTURE,
	PROFCOMMAND_ENABLE,
	PROFCOMMAND_DISABLE
};


struct ProfileTiming
{
	bool		bUsed;					// Whether profiled this frame
	FName		statname;				// Name of statistic
	INT			parent;					// Index of parent Timing
	INT			cycles[PROF_HISTORY];	// History of cycle timings
	INT			maxcycles;				// Maximum number of cycles (range of child graph)
};


class ENGINE_API Profiler
{
private:
	bool bInactive;							// Whether profiler is off or on
	bool bCapturing;						// Whether currently advancing time
	int Selected;							// Currently selected category
	int ProfileChildrenOf;					// View the children of this category
	int prioritylevel;						// Timing priority (used to compile out PROFILESLOW commands)
	int ProfilePresent;						// Presend time in history
	FPlane colors[PROF_NUM_COLORS];			// Color table
	EProfCommand PendingCommand;			// Currently pending command (executed at frame boundary)
	ProfileTiming Timing[PROF_NUM_TIMINGS];	// Timing Categories

	TArray<INT> IndexStack;					// stack of indicies
	int Top;

	void PushIndex(int index);
	void PopIndex();
	int TopIndex();
	void SubmitCommand(EProfCommand command);
	void ExecuteCommand();

public:
	Profiler();

	void Enable(bool on);
	void Capture(bool on);
	void SelectPrev();
	void SelectNext();
	void TraverseUp();
	void TraverseDown();

	void Start(enum EName slotname, int priority);
	void Stop(enum EName slotname, int priority);
	void Render(FSceneNode* Frame);
	bool Enabled();
	bool Capturing();
};


extern ENGINE_API Profiler GProfile;
