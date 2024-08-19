//=============================================================================
// Registry: Game registry listing interface.
// This class contains global interfaces used to query information about
// objects, preferences and properties registered in the localization files.
//
// This class was implemented in Unreal Tournament v469d and packages that use 
// it will not load in previous versions of the game.
//
// This is a built-in Unreal class and it shouldn't be modified.
//=============================================================================
class Registry
	expands Object
	native
	abstract;


//=============================================================================
// Registry structures.

// Information about a driver class.
struct RegistryObjectInfo
{
	var() string Object;
	var() string Class;
	var() string MetaClass;
	var() string Description;
	var() string Autodetect;
};

// Information about a preferences menu item.
struct PreferencesInfo
{
	var() string Caption;
	var() string ParentCaption;
	var() string Class;
	var() name Category;
	var() bool Immediate;
};

// Information about a property
struct RegistryPropertyInfo
{
	var() string Object;
	var() string Class;
	var() string Title;
	var() string Description;
	var() array<string> Values;
	var() array<string> ValueDescriptions;
	var() float RangeMin;
	var() float RangeMax;
	var() float Step;
	var() bool Config;            // Editable config property
	var() bool RequireRestart;   // Changing config property requires restart
	var() bool Constant;          // This property does not change
};


//=============================================================================
// Registry query functions.

native static final function array<RegistryObjectInfo> GetRegistryObjects( string Class, optional string MetaClass);
native static final function array<PreferencesInfo> GetPreferences( optional string ParentCaption);
native static final function array<RegistryPropertyInfo> GetRegistryProperties( string Class);

native static final function Property LoadRegistryProperty( RegistryPropertyInfo Info);
