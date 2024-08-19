# Microsoft Developer Studio Project File - Name="RuneI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RuneI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RuneI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RuneI.mak" CFG="RuneI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RuneI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RuneI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Rune/RuneI", VQIBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RuneI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Lib"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RUNEI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Zp4 /MT /W4 /WX /vd0 /GX /O2 /I "..\Inc" /I "..\..\Core\Inc" /I "..\..\Engine\Inc" /I ".." /I "Inc" /I "..\Core\Inc" /I "..\Engine\Inc" /D "NDEBUG" /D RUNEI_API=__declspec(dllexport) /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\Core\Lib\Core.lib ..\Engine\Lib\Engine.lib kernel32.lib user32.lib gdi32.lib /nologo /dll /incremental:yes /machine:I386 /out:"..\System\RuneI.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "RuneI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Lib"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RUNEI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp4 /MTd /W4 /WX /Gm /vd0 /GX /ZI /Od /I "Inc" /I "..\Core\Inc" /I "..\Engine\Inc" /I ".." /D "_DEBUG" /D RUNEI_API=__declspec(dllexport) /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_REALLY_WANT_DEBUG" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\Core\Lib\Core.lib ..\..\Engine\Lib\Engine.lib kernel32.lib user32.lib gdi32.lib /nologo /dll /debug /machine:I386 /out:"..\..\System\RuneI.dll" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "RuneI - Win32 Release"
# Name "RuneI - Win32 Debug"
# Begin Group "Src"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\runei.cpp
# End Source File
# End Group
# Begin Group "Inc"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\inc\runeiclasses.h
# End Source File
# Begin Source File

SOURCE=.\inc\runeiprivate.h
# End Source File
# End Group
# Begin Group "Classes"

# PROP Default_Filter "*.uc"
# Begin Group "Decoration"

# PROP Default_Filter ""
# Begin Group "Plants"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Bush.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Bush2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Fruit_Tree.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowPlant5.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PineTree.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PineTreeBrown.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PineTreeSnow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\plants.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\sappling.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeMushroom5.uc
# End Source File
# End Group
# Begin Group "Dynamic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BoneBridgeBig.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BoneBridgeSmall.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FootBridge.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HangingChain.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\tarp.uc
# End Source File
# End Group
# Begin Group "Generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\AnimalTrough.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Bench.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Bone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Bucket.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BurlapSack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BurlapSack1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BurlapSack2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BurlapSack3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Chandelier.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Crucifix2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Crusifix.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DeadBaracuda.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EmptyTubestriker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EmptyTubestrikerB.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EmptyTubestrikerC.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinMask.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GrainSack5.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HelChandelier.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HelSpikeBig.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HelSpikeSmall.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Keg.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\KegWithTap.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Kettle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\oar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Pelvis.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Plate.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RibInDirt1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RibInDirt2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RibInDirt3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Ribs.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Skin.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Skinrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Skull.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Stool.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\StoolDwarf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Table.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TableDwarfLong.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TableDwarfShort.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TarpFrame.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Wagon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WagonLarge.uc
# End Source File
# End Group
# Begin Group "Floater"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Floater.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\icechunk.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LavaChunk.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\raft.uc
# End Source File
# End Group
# Begin Group "Statue"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Hel.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiStatue.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Statue.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\StatueGoblin.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\StatueSnowbeast.uc
# End Source File
# End Group
# Begin Group "Instruments"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\drum.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Drum1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Drum2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrumSide.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\gong.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\horn.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\instrument.uc
# End Source File
# End Group
# Begin Group "Limbs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Limb.uc
# End Source File
# End Group
# Begin Group "Carcass"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\CarcassBabyCrab.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CarcassDangler.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CarcassDarkViking.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CarcassGiantCrab.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CarcassGoblin.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CarcassMechDwarf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerAlricCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerBerserkerCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerConrackCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerDarkVikingCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerDarkWarriorCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerElderCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerKarlCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerLokiGuardCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerRagnarSnowCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkAxeCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkConrackCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkHammerCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkSpawnCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkSwordCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerShipWreckRagnarCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSigurdCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSvenCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerTownRagnarCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerTrialPitRagnarCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerUlfCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerValkyrieCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerWolfgarCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerZombie2Carcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerZombieCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowbeastChainedCarcass.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieCarcass.uc
# End Source File
# End Group
# Begin Group "DrownedVikings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DrownedAlric.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedKarl.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedSven.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedUlf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedViking.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrownedWolfgar.uc
# End Source File
# End Group
# Begin Group "WaterPlants"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Coral1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Coral2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Coral3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Coral4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CoralCastle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CoralCastle2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CoralCastle3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CoralFan.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SeaWeed.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sponge1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sponge2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sponge3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sponge4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sponge5.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel5.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpongeBarrel6.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterPlants.uc
# End Source File
# End Group
# Begin Group "FireObjects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\FireObject.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireRing.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireRingWithFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinHeadSconce.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinHeadSconceFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinTorch.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Hotbowl.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LionHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiSconce1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiSconce2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ShortBrazer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TallBrazer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TorchHolder.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Tstadt_torch.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Tstadt_torchWithFire.uc
# End Source File
# End Group
# Begin Group "Gibs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Gib.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GibRedMedium.uc
# End Source File
# End Group
# Begin Group "Rocks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DestroyRock.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockAvalanche.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockAvalancheHuge.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockAvalancheLarge.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockAvalancheMed.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockAvalancheSmall.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockHuge.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockLarge.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockMedium.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Rocks.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RockSmall.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\DecorationRune.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecorationWeapon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RopeWithRock.uc
# End Source File
# End Group
# Begin Group "Pawn"

# PROP Default_Filter ""
# Begin Group "Crabs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BabyCrab.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GiantCrab.uc
# End Source File
# End Group
# Begin Group "Dwarves"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Dwarf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfBlack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfMech.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfUnderground.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarA.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarB.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarC.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarD.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarE.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWarF.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWoodland.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWoodlandBig.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWoodlandSmall.uc
# End Source File
# End Group
# Begin Group "Fish"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Baracuda.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Cuttle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Dangler.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DanglerFemale.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DealieFish.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DealieFishSchool.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Eel.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FlockMasterPawn.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FlockPawn.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Jellyfish.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Manowar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ManowarBaby.uc
# End Source File
# End Group
# Begin Group "Goblins"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Goblin.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinCrazy.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinFemale.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinMohawk.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinWarrior.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinWeakling.uc
# End Source File
# End Group
# Begin Group "Sark"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Sark.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkConrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkSpawn.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkSword.uc
# End Source File
# End Group
# Begin Group "Snowbeasts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\SnowBeast.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowBeastBoss.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowBeastChained.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowBeastTrial.uc
# End Source File
# End Group
# Begin Group "Vikings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Alric.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Berserker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Conrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkViking.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkVikingSnow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkWarrior.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Elder.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Jarl.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Karl.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiGuard.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ScriptableRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ScriptableSarkRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ScriptableTownRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sigurd.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sven.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrainingUlf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Ulf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Viking.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Wolfgar.uc
# End Source File
# End Group
# Begin Group "TubeStriker"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BabyTubeStriker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TubeStriker.uc
# End Source File
# End Group
# Begin Group "Zombie"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Zombie.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Zombie2.uc
# End Source File
# End Group
# Begin Group "Birds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Bird.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Crow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OdinCrow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SeaBird.uc
# End Source File
# End Group
# Begin Group "Mounts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Beetle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Mount.uc
# End Source File
# End Group
# Begin Group "SpiderBot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\SpiderBot.uc
# End Source File
# End Group
# Begin Group "Loki"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Loki.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiBust.uc
# End Source File
# End Group
# Begin Group "Odin"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Odin.uc
# End Source File
# End Group
# Begin Group "StoneGuard"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\StoneGuard.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\ScriptPawn.uc
# End Source File
# End Group
# Begin Group "Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Accelerator.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\beam.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Cinematography.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Confinement.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Movie.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Performer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RopePoint.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpringPoint.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TreePoint.uc
# End Source File
# End Group
# Begin Group "Inventory"

# PROP Default_Filter ""
# Begin Group "Axes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Axe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfBattleAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\goblinaxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinAxePowerup.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HandAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigurdAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VikingAxe.uc
# End Source File
# End Group
# Begin Group "Swords"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DwarfBattleSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWorkSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\romansword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VikingBroadSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VikingShortSword.uc
# End Source File
# End Group
# Begin Group "Hammers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\boneclub.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfBattleHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWorkHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Hammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RustyMace.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrialPitMace.uc
# End Source File
# End Group
# Begin Group "NonStowable"

# PROP Default_Filter ""
# Begin Group "LimbWeapons"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\AxeSarkArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BerserkerLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BerserkerRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ConrackLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ConrackRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CrabClaw.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CrabLeg.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkVikingLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkVikingRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ElderArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\goblinlarm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\goblinrarm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GuardLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GuardRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\KarlLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\KarlRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LimbWeapon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechBladeArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkConArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkHammerArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkRagnarArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkSwordArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigurdLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigurdRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowRagnarArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TownRagnarArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrialRagnarArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TTongue.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WolfLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WolfRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WomanArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WoodDwarfLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WoodDwarfRArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieLArm.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieRArm.uc
# End Source File
# End Group
# Begin Group "Heads"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\AlricHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BerserkerHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ConrackHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkVikingHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ElderHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinBHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinCHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinDHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinEHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GuardHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Head.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\KarlHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkAxeHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkConHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkHammerHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkRagnarHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkSwordHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigurdHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SvenHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TownRagnarHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrialRagnarHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\UlfHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfAHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfBHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfCHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfDHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfEHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WarDwarfFHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WolfHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WomanHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WoodDwarfAHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WoodDwarfBHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WoodDwarfCHead.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieHead.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\HelTorch.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\NonStow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Torch.uc
# End Source File
# End Group
# Begin Group "Shields"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DarkShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfBattleShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DwarfWoodShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MagicShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VikingShield.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VikingShield2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterloggedShield.uc
# End Source File
# End Group
# Begin Group "Pickups"

# PROP Default_Filter ""
# Begin Group "Foods"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\food.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HealthFruit1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HealthFruit2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LegOMeat1.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LegOMeat2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LegOMeat3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Lizard.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Stein.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\RuneOfHealth.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneOfPower.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneOfPowerRefill.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneOfStrength.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneOfStrengthRefill.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Runes.uc
# End Source File
# End Group
# Begin Group "InvisibleWeapon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\CrabPincer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GoblinClaw.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\InvisibleWeapon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechBlade.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkClaw.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowbeastJaws.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SnowbeastPaw.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TubeStrikerTongue.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieClaw.uc
# End Source File
# End Group
# End Group
# Begin Group "Effects"

# PROP Default_Filter ""
# Begin Group "DarkDwarfEffects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DarkDwarfBlast.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfBolt.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfChargeup.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfConcentrator.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfExplosion.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfLightning.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DarkDwarfLightning2.uc
# End Source File
# End Group
# Begin Group "Blood"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Blood.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Blooddrips.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Blooddrips2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodMist.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodSpot.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Bloodspot2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodSpray.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodSpurt.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodUnderwater.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodWaterSurface.uc
# End Source File
# End Group
# Begin Group "Mud"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\MudBubble.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MudGlob.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OtherBubble.uc
# End Source File
# End Group
# Begin Group "debris"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DebrisFlesh.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DebrisIce.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DebrisStone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DebrisWood.uc
# End Source File
# End Group
# Begin Group "RunePower"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BlastGlow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BlastRadius.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodlustEnd.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BloodlustStart.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\coronared.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireRadius.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireSwordEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LightningSwordEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ProtectionSphere.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ProtSphereDamage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SonicBlast.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SonicBlastHighlight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SonicClubEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrialPitFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VampireReplenish.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VampireTrail.uc
# End Source File
# End Group
# Begin Group "DiscardedHealth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DiscardedHealth.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EatenLegBone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EatenLizard.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EmptyStein.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Fruit_Core.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Fruit_Core2.uc
# End Source File
# End Group
# Begin Group "SarkEyes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\sarkeye.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\sarkeyeaxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkEyeConrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkEyeFlame.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\sarkeyehammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\sarkeyenone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkEyeRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkEyeRagnarRed.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkEyeSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkRagnarEyeFlame.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkRagnarEyeFlameRed.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieEye.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieEyeFlame.uc
# End Source File
# End Group
# Begin Group "Sigil"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Sigil.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigilFlameSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SigilLightningSword.uc
# End Source File
# End Group
# Begin Group "EffectSkeleton"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\EffectSkelAvalancheSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelBlast.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelEmpathyAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkeleton.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelFlameSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelGibAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelGroundHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelIceAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelLightningSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelSonicClub.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelStoneHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelTrialMace.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\EffectSkelVampire.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\DanglerLight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DealieLight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FlashCycle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FlashFade.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiHealthTrail.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ManowarEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OdinEyeBlast.uc
# End Source File
# End Group
# Begin Group "Player"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\PlayerAlric.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerBerserker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerConrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerDarkViking.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerDarkWarrior.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerElder.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerKarl.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerLokiGuard.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkAxe.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkConrack.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkHammer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkSpawn.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSigurd.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSven.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerUlf.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerValkyrie.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerWolfgar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerZombie.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerZombie2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Ragnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarFlight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarOnBeetle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RagnarSnow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RunePlayer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RunePlayerProxy.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ShipWreckRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TownRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrialPitRagnar.uc
# End Source File
# End Group
# Begin Group "Info"

# PROP Default_Filter ""
# Begin Group "CostumeChanges"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\SinglePlayerBeetle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerLeather.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerSark.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerShipwreck.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerShipwreckSword.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerSnow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerSRNMHSHKTGPANGFH.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerTown.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerTrial.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SinglePlayerTrialNoWeapons.uc
# End Source File
# End Group
# Begin Group "Mutators"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\MutatorAxeMatch.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorFatboy.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorInfinitePowerups.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorLefty.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorLowDamage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorLowGrav.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorNoPowerups.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorNoRunes.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorRandWeapon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorTemplate.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MutatorVampireMatch.uc
# End Source File
# End Group
# Begin Group "Messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\GenericMessage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\NoRunePowerMessage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PickupMessage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SayMessage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SubtitleMessage.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SubtitleRed.uc
# End Source File
# End Group
# Begin Group "ZoneInfo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\CloudZone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiBloodZone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ShredderZone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SkyZoneInfoOsc.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TeleporterZone.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterZone.uc
# End Source File
# End Group
# Begin Group "GameInfo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\RuneFlight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneGameInfo.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneMultiPlayer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSinglePlayer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Teamgame.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\DMmaplist.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneGameReplicationInfo.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TeamInfo.uc
# End Source File
# End Group
# Begin Group "HUD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\RuneHUD.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneScoreboard.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneScoreboardTeam.uc
# End Source File
# End Group
# Begin Group "Triggers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\CineTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DamageTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DestroyTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DispatcherFaucet.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FearSpot.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Lever.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OrdersDispatcher.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OrdersTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\pump.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SightTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpecialEventRune.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\StochasticTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZoneTemplateTrigger.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZoneVelocityTrigger.uc
# End Source File
# End Group
# Begin Group "Keypoint"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\ActorGenerator.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CineCamera.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CineInterpolationPoint.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\CineInterpolationPointTemp.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LookTarget.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ProjectileGenerator.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\VerticalCurrent.uc
# End Source File
# End Group
# Begin Group "Imports"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Imports\Classes\CreatureImports.uc
# End Source File
# Begin Source File

SOURCE=..\Imports\Classes\Imports.uc
# End Source File
# Begin Source File

SOURCE=..\Imports\Classes\ObjectImports.uc
# End Source File
# Begin Source File

SOURCE=..\Imports\Classes\PlantImports.uc
# End Source File
# Begin Source File

SOURCE=..\Imports\Classes\PlayerImports.uc
# End Source File
# Begin Source File

SOURCE=..\Imports\Classes\WeaponImports.uc
# End Source File
# End Group
# Begin Group "Brush"

# PROP Default_Filter ""
# Begin Group "Movers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\AttachMover.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Door.uc
# End Source File
# End Group
# Begin Group "Polyobjs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\AttachPolyobj.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DDObstacle.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RopePolyobj.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RunePolyobj.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WindmillPolyobj.uc
# End Source File
# End Group
# End Group
# Begin Group "NavPoint"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\ScriptPoint.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TeleporterTriggered.uc
# End Source File
# End Group
# Begin Group "Accelerators"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\treebark.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TreeFern.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TreeGrow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TreePt.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TreeVine.uc
# End Source File
# End Group
# Begin Group "Menus"

# PROP Default_Filter ""
# End Group
# Begin Group "Decals"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DecalBlood.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalBlood2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalBlood3.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalBlood4.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalBlood5.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalChar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DecalSlash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Footprint.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FootprintBloody.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FootprintWet.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerShadow.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WallMark.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WorldDecal.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZTargetDecal.uc
# End Source File
# End Group
# Begin Group "ParticleSystem"

# PROP Default_Filter ""
# Begin Group "Beams"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BeamChain.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BeamRope.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BeamSystem.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BeamVine.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Chain.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ClimbableChain.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ClimbableVine.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\lightningswordbeam.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Rope.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SonicBeam.uc
# End Source File
# End Group
# Begin Group "Weather"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Rain.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\rainsplash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Snow.uc
# End Source File
# End Group
# Begin Group "Clouds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\DebrisCloud.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GroundDust.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitStone.uc
# End Source File
# End Group
# Begin Group "Fire"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BigFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BlackSmoke.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Fire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GraySmoke.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PawnFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SmallFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Smoke.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TorchFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\TrailFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieChangeFire.uc
# End Source File
# End Group
# Begin Group "WaterEffects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\BubbleSystem.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BubbleSystemDelay.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BubbleSystemOneShot.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DropRipple.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ripple.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Ripple2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\splash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterFallFog.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterFallSplash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WaterFallSpray.uc
# End Source File
# End Group
# Begin Group "electricity"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Electricity.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LightningPowerup.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Sparks.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SparkSystem.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SparkSystemHit.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SparkSystemVertical.uc
# End Source File
# End Group
# Begin Group "Geysers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\FireGeyser.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Geyser.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SteamGeyser.uc
# End Source File
# End Group
# Begin Group "Breath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\Breath.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BreathLight.uc
# End Source File
# End Group
# Begin Group "WeaponSwipes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\SwipeEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeBlue.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeGray.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeGreen.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipePurple.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeRed.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeVamp.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeVampHealthTrail.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponSwipeYellow.uc
# End Source File
# End Group
# Begin Group "RuneSpheres"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\RuneSpheres.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSpheresBerserker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSpheresBerserker2.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSpheresHealth.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSpheresPower.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneSpheresPower2.uc
# End Source File
# End Group
# Begin Group "WeaponFX"

# PROP Default_Filter ".uc"
# Begin Source File

SOURCE=.\Classes\EmpathyFlash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FallingRocks.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GroundHammerEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HelixEmpathy.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitIce.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitMetal.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitSpark.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitWeapon.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HitWood.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\IceAxeEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerSarkRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerShipWreckRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\PlayerTownRagnar.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\StoneHammerEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\WeaponHelix.uc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes\BlazeEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BlinkingLight.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\BlueTrail.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\browndust.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\DrippingBlood.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Exploding.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Explosion.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireRays.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\FireSpewer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowBulb.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GlowplantSparks.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\HugeSplash.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\KegLeak.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LokiHealthSystem.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechFireSpewer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechRocketEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechRocketExplosion.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechRocketSmoke.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechRocketTrail.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechSteamSpewer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MushroomPuff.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\OdinEffect.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Powerup.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\RespawnFire.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SarkBloodMist.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SeekerTrail.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpiderBotExplosion.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SteamBlast.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SteamSpewer.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\ZombieBreath.uc
# End Source File
# End Group
# Begin Group "Projectile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Classes\EnergyBall.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\GroundProjectile.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\LightningPowerupBall.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\MechRocket.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\Seeker.uc
# End Source File
# Begin Source File

SOURCE=.\Classes\SpiderBomb.uc
# End Source File
# End Group
# Begin Source File

SOURCE=..\System\buglist.txt
# End Source File
# Begin Source File

SOURCE=.\Classes\RuneI.upkg
# End Source File
# Begin Source File

SOURCE=.\Classes\testactor.uc
# End Source File
# End Group
# Begin Group "INI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\System\Default.ini
# End Source File
# Begin Source File

SOURCE=..\System\defaultforeign.ini
# End Source File
# Begin Source File

SOURCE=..\System\defaulthigh.ini
# End Source File
# Begin Source File

SOURCE=..\System\DefaultHOV.ini
# End Source File
# Begin Source File

SOURCE=..\System\DefaultRussian.ini
# End Source File
# Begin Source File

SOURCE=..\System\DefUser.ini
# End Source File
# Begin Source File

SOURCE=..\System\DefUserRut.ini
# End Source File
# Begin Source File

SOURCE=..\System\Rune.ini
# End Source File
# Begin Source File

SOURCE=..\System\SetupRuneHOV.ini
# End Source File
# Begin Source File

SOURCE=..\System\User.ini
# End Source File
# End Group
# Begin Group "INT"

# PROP Default_Filter "INT"
# Begin Source File

SOURCE=..\System\Core.int
# End Source File
# Begin Source File

SOURCE=..\System\D3DDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\Editor.int
# End Source File
# Begin Source File

SOURCE=..\System\Engine.int
# End Source File
# Begin Source File

SOURCE=..\System\Galaxy.int
# End Source File
# Begin Source File

SOURCE=..\System\GlideDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\IpDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\IpServer.int
# End Source File
# Begin Source File

SOURCE=..\System\Manifest.int
# End Source File
# Begin Source File

SOURCE=..\System\MeTaLDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\OGLDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\OpenGLDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\rbrowser.int
# End Source File
# Begin Source File

SOURCE=..\System\rmenu.int
# End Source File
# Begin Source File

SOURCE=..\System\Rune.int
# End Source File
# Begin Source File

SOURCE=..\System\runei.int
# End Source File
# Begin Source File

SOURCE=..\System\RuneServerAdmin.int
# End Source File
# Begin Source File

SOURCE=..\System\SetupRuneHOV.int
# End Source File
# Begin Source File

SOURCE=..\System\SglDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\SoftDrv.int
# End Source File
# Begin Source File

SOURCE=..\System\Startup.int
# End Source File
# Begin Source File

SOURCE=..\System\UBrowser.int
# End Source File
# Begin Source File

SOURCE=..\System\UWeb.int
# End Source File
# Begin Source File

SOURCE=..\System\uwindow.int
# End Source File
# Begin Source File

SOURCE=..\System\Window.int
# End Source File
# Begin Source File

SOURCE=..\System\WinDrv.int
# End Source File
# End Group
# End Target
# End Project
