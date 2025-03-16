//=============================================================================
// UMenuAdvancedDriverClientWindow
//
// Advanced Video Settings window's content class.
// This object queries the property registry for a render device's properties.
// It then handles the results to dynamically populate the Settings window.
//
// This class is based on XC_Engine's Dynamic Driver Settings window.
//
// This class was implemented in Unreal Tournament v469d and packages that use 
// it will not load in previous versions of the game.
//=============================================================================
class UMenuAdvancedDriverClientWindow expands UMenuPageWindow
	imports(Registry);

var int ControlOffset;
var bool bChanging;

// Set during instantiation from defaults
var string DriverClassName;
var string ConfirmSettingsRestartTitle;
var string ConfirmSettingsRestartText;

// Controls instanced using the Property Registry
var array<UWindowDialogControl> RegistryControls;
var array<RegistryPropertyInfo> RegistryProperties;
var RegistryPropertyInfo RegPropInfo;

// Higor: the UWindow system has no way to notify the parent/owner
// about a child window being created, therefore the initialization
// parameters must be set in advance, prior to creation.
static function SetNextDriver( string DriverClassName, optional string ConfirmSettingsRestartTitle, optional string ConfirmSettingsRestartText)
{
	class'UMenuAdvancedDriverClientWindow'.default.DriverClassName = DriverClassName;
	class'UMenuAdvancedDriverClientWindow'.default.ConfirmSettingsRestartTitle = ConfirmSettingsRestartTitle;
	class'UMenuAdvancedDriverClientWindow'.default.ConfirmSettingsRestartText = ConfirmSettingsRestartText;
}


function Created()
{
	local int CenterWidth, CenterPos;
	local int i;
	local UWindowDialogControl NewControl;
	local string Used, Check;
	const UsedSeparator = "~~~~";
		
	Super.Created();
	
	CenterWidth = (WinWidth/5)*4;
	CenterPos = (WinWidth - CenterWidth)/2;
	ControlOffset = 10;
	
	// Enumerate all config properties from the Property Registry
	RegistryProperties = GetRegistryProperties(DriverClassName);
	RegistryControls.Length = RegistryProperties.Length;
	
	// TODO: handle known properties if Property Registry yields no results
	
	// Filter out non-config Properties
	Used = UsedSeparator;
	for ( i=0 ; i<RegistryProperties.Length ; i++ )
	{
		if ( !RegistryProperties[i].Config )
			continue;
		RegPropInfo = RegistryProperties[i];
		Check = GetPropertyText("RegPropInfo") $ UsedSeparator;
		if (InStr(Used, UsedSeparator $ Check) != -1)
			continue;
		Used $= Check;
		
		NewControl = CreatePropertyControl(RegistryProperties[i],CenterPos,ControlOffset,CenterWidth,16);
		if ( NewControl != none )
		{
			RegistryControls[i] = NewControl;
			UpdateValue( NewControl, GetPlayerOwner().ConsoleCommand("get"@DriverClassName@RegistryProperties[i].Object));
		}
		else
		{
			Log("Unhandled property"@RegistryProperties[i].Object);
			continue;
		}
		
		ControlOffset += 21;
	}
	i = RegistryControls.Length;
	while ( --i >= 0 )
		if ( RegistryControls[i] == none )
		{
			RegistryControls.Remove(i);
			RegistryProperties.Remove(i);
		}

	ControlOffset += 10;
}

function AfterCreate()
{
	DesiredWidth = 220;
	DesiredHeight = ControlOffset;
}

function BeforePaint( Canvas C, float X, float Y)
{
	local int i;
	local int CenterWidth, CenterPos;
	local UWindowDialogControl Control;

	CenterWidth = (WinWidth/4)*3;
	CenterPos = (WinWidth - CenterWidth)/2;

	for ( i=0; i<RegistryControls.Length; i++ )
		if ( RegistryControls[i] != None )
		{
			Control = RegistryControls[i];
			
			Control.WinLeft = CenterPos;
	
			if ( UWindowCheckbox(Control) != None )
				Control.SetSize(CenterWidth-100+16, 1);
			else
				Control.SetSize(CenterWidth, 1);
				
			if ( UWindowComboControl(Control) != none )
				UWindowComboControl(Control).EditBoxWidth = 100;
			else if ( UWindowHSliderControl(Control) != none )
				UWindowHSliderControl(Control).SliderWidth = 100;
			else if ( UWindowEditControl(Control) != none )
				UWindowEditControl(Control).EditBoxWidth = 100;
		}
	
}

function UWindowDialogControl CreatePropertyControl( out RegistryPropertyInfo RPI, int X, int Y, int XL, int YL)
{
	local int ArrayDim;
	local UWindowDialogControl NewControl;
	local Property P;
	local int i;
	
	P = LoadRegistryProperty(RPI);
	if (P == None)
		return None;

	// Higor: currently not implemented
	ArrayDim = int(P.GetPropertyText("ArrayDim"));
	if ( ArrayDim > 1 ) //Handle later
		return none;
		
	// Use a Combo control and add Values (and descriptions) as options
	if ( RPI.Values.Length > 0 )
	{
		NewControl = CreateControl(class'UWindowComboControl', X, Y, XL, YL);
		for ( i=0 ; i<RPI.Values.Length ; i++ )
		{
			if ( RPI.ValueDescriptions[i] != "" )
				UWindowComboControl(NewControl).AddItem( RPI.ValueDescriptions[i], RPI.Values[i]);
			else
				UWindowComboControl(NewControl).AddItem( RPI.Values[i], RPI.Values[i]);
		}
		UWindowComboControl(NewControl).SetEditable(false);
	}
	// Use a slider for a custom range
	else if ( RPI.RangeMax != 0 || RPI.RangeMin != 0 )
	{
		NewControl = CreateControl(class'UWindowHSliderControl', X, Y, XL, YL);
		UWindowHSliderControl(NewControl).SetRange(RPI.RangeMin, RPI.RangeMax, RPI.Step);
		UWindowHSliderControl(NewControl).FloatStep = RPI.Step;
	}
	else if ( P.IsA('BoolProperty') )
	{
		NewControl = CreateControl(class'UWindowCheckBox', X, Y, XL, YL);
		NewControl.Align = TA_Left;
	}
	else if ( P.IsA('ByteProperty') )
	{
		// TODO: Handle enums
		NewControl = CreateControl(class'UWindowHSliderControl', X, Y, XL, YL);
		if ( RPI.RangeMax == 0 )
		{
			RPI.RangeMax = 255;
			if ( RPI.Step == 0 )
				RPI.Step = 16;
		}
		UWindowHSliderControl(NewControl).SetRange(RPI.RangeMin, RPI.RangeMax, RPI.Step);
	}
	else if ( P.IsA('IntProperty') || P.IsA('FloatProperty') || P.IsA('StrProperty') )
	{
		// TODO: Handle short ranges without sliders?
		NewControl = CreateControl(class'UWindowEditControl', X, Y, XL, YL);
		if ( !P.IsA('StrProperty') )
			UWindowEditControl(NewControl).SetNumericOnly( true );
		if ( P.IsA('FloatProperty') && ((RPI.Step == 0) || (float(int(RPI.Step)) != RPI.Step)) )
			UWindowEditControl(NewControl).SetNumericFloat( true );
		UWindowEditControl(NewControl).SetDelayedNotify( true );
		UWindowEditControl(NewControl).EditBox.Align = TA_Left;
	}
	
	if ( NewControl != none )
	{
		NewControl.SetText(RPI.Title);
		NewControl.SetHelpText(RPI.Description);
		NewControl.SetFont(F_Normal);
	}
	
	return NewControl;
}

function Notify(UWindowDialogControl C, byte E)
{
	local int i;

	Super.Notify(C, E);
	
	// Internal correction in progress, ignore
	if ( bChanging )
		return;
	
	switch(E)
	{
	case DE_Change:
		
		for ( i=0; i<RegistryControls.Length; i++)
		{
			if ( C == RegistryControls[i] )
			{
				if ( UpdateOption( C, RegistryProperties[i].Object) && RegistryProperties[i].RequireRestart )
					MessageBox(ConfirmSettingsRestartTitle, ConfirmSettingsRestartText, MB_OK, MR_OK, MR_OK);
				break;
			}
		}
		break;
	}
}

// Menu -> Object
function bool UpdateOption( UWindowDialogControl Control, string PropertyName)
{
	local string Value, OldValue;
	
	OldValue = GetPlayerOwner().ConsoleCommand("get" @ DriverClassName @ PropertyName);
	
	// Get Value
	if ( UWindowCheckBox(Control) != None )
		Value = Control.GetPropertyText("bChecked");
	else if ( UWindowComboControl(Control) != None )
		Value = UWindowComboControl(Control).GetValue2();
	else if ( UWindowHSliderControl(Control) != None )
		Value = Control.GetPropertyText("Value");
	else if ( UWindowEditControl(Control) != None )
		Value = UWindowEditControl(Control).GetValue();

	GetPlayerOwner().ConsoleCommand("set" @ DriverClassName @ PropertyName @ Value);
	
	// Fix Value
	Value = GetPlayerOwner().ConsoleCommand("get" @ DriverClassName @ PropertyName);
	UpdateValue( Control, Value);
	
	return OldValue != Value;
}

// Object -> Menu
function UpdateValue( UWindowDialogControl Control, string Value)
{
	local int i;

	bChanging = true;
	
	// Update checkbox
	if ( UWindowCheckBox(Control) != None )
		Control.SetPropertyText("bChecked",Value);
	// Update combo control
	else if ( UWindowComboControl(Control) != None )
	{
		i = UWindowComboControl(Control).FindItemIndex2( Value, true);
		if ( (i == -1) && (string(float(Value)) == Value) && (float(int(Value)) == float(Value)) ) //Try float -> int
			i = UWindowComboControl(Control).FindItemIndex2( string(int(Value)), true);
		if ( i == -1 )
			UWindowComboControl(Control).SetValue( Value, Value);
		else
			UWindowComboControl(Control).SetSelectedIndex(i);
	}
	// Update slider control
	else if ( UWindowHSliderControl(Control) != None )
		UWindowHSliderControl(Control).SetValue( float(Value) );
	// Update edit control
	else if ( UWindowEditControl(Control) != None )
	{
		if ( !UWindowEditControl(Control).EditBox.bChangePending )
		{
			if (UWindowEditControl(Control).EditBox.bNumericOnly && InStr(Value, ".") != -1)
			{
				while (Mid(Value, Len(Value) - 1, 1) == "0" && Mid(Value, Len(Value) - 2, 1) != ".")
					Value = Left(Value, Len(Value) - 1);
			}
			UWindowEditControl(Control).SetValue(Value);
		}
	}
		
	bChanging = false;
}





