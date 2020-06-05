//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A comprehensive bunnyhop-helper UI system by Hogyn Melyn (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589)
// Shows the player's horizontal speed (more clearly than cl_showpos_xy), changes in speed between jumps, input and velocity directions mapped onto the screen.
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>
#include "view.h"
#include <string>
#include <../shared/gamemovement.h>
#include <string>
#include <cstring>


// neither should be needed for friending the class
//#include "../c_baseplayer.h"
//#include <../shared/tf/tf_gamemovement.cpp>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define SHADOW_OFFSET 2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSpeedometer : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, EditablePanel);

public:
	CHudSpeedometer(const char *pElementName);

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	void UpdateColours(void);

	Color GetComplimentaryColour( Color color );
private:
	// The speed bar 
	ContinuousProgressBar *m_pSpeedPercentageMeter;

	// The speed delta text label
	Label *m_pDeltaTextLabel;
	Label *m_pDeltaTextLabelDropshadow;

	// The horizontal speed number readout
	Label *m_pSpeedTextLabel;
	Label *m_pSpeedTextLabelDropshadow;

	// Keeps track of the changes in speed between jumps.
	float cyflymder_ddoe = 0.0f;
	float cyflymder_echdoe = 0.0f;
	bool groundedInPreviousFrame = false;

	float FOVScale = 1.0f;

	Color colourDefault = Color(251, 235, 202, 255); // "Off-white"
	Color playerColourBase;						// The player's chosen Merc colour
	Color playerColourComplementary;			// The colour that compliments the player's colour - pls allow this for cosmetics <3
	Color *playerColour = &colourDefault;	// The colour we'll use if the player-colour convar is 1 (This references one of the above two)
	
	Color playerColourShadowbase = Color(0, 0, 0, 255);
	Color *playerColourShadow = &playerColourShadowbase;

	Color defaultVelVectorCol = Color(255, 0, 0, 255);
	Color defaultInputVectorCol = Color(0, 0, 255, 255);
	Color *vectorColor_vel = &defaultVelVectorCol;
	Color *vectorColor_input = &defaultInputVectorCol;

	void UpdateScreenCentre(void);

	void DrawTextFromNumber(std::string, double num, Color col, int xpos, int ypos);

	void QStrafeJumpHelp(void);
};

DECLARE_HUDELEMENT(CHudSpeedometer);

void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);

// Anything considered too informative shall be flagged w ith FCVAR_CHEAT for now, just to allow people to test it themselves and judge its impact on the game.

// Cofiwch! Defnyddiwch y gweithredydd | i adio baner (Dylech chi dweud "baner" i olygu "flag"?) e.g. FCVAR_ARCHIVE | FCVAR_REPLICATED
// Hefyd, Peidiwch BYTH â rhoi FCVAR_ARCHIVE gyda FCVAR_CHEAT - Byddwch chi cadw y cheat yn config.cfg - TERRIBLE idea

// const char *name, const char *defaultvalue, int flags (use pipe operator to add), const char *helpstring, bool bMin, float fMin, bool bMax, float fMax
// Ar ôl i valve dev wiki, default value doesn't set the value. Instead you have to actually set it with pName->SetVale([value]); , if I understand?
ConVar hud_speedometer("hud_speedometer", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows horizontal speed as a number under the crosshair. 2: Shows the number as well as a meter ranging from 0 to the maximum BHop speed.", SpeedometerConvarChanged);
ConVar hud_speedometer_maxspeed("hud_speedometer_maxspeed", "1000", FCVAR_ARCHIVE, "The maximum speed to use when drawing the speedometer bar with hud_speedometer set to 2.", true, 400.0f, true, 2000.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_delta("hud_speedometer_delta", "1", FCVAR_ARCHIVE, "0: Off, 1: Shows the change in speed between each jump.", SpeedometerConvarChanged);
ConVar hud_speedometer_opacity("hud_speedometer_opacity", "150", FCVAR_ARCHIVE, "Sets the opacity of the speedometer overlay.", true, 0.0f, true, 255.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_useplayercolour("hud_speedometer_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer UI uses default colours. 1: Speedometer UI uses the player's colour. 2: Uses complimentary colour.", SpeedometerConvarChanged);
//ConVar hud_speedometer_normaliseplayercolour("hud_speedometer_normaliseplayercolour", "0", FCVAR_ARCHIVE, "0: If using player colour, use the raw colour. 1: Use the normalised colour (Forces a level of saturation and brightness), allowing dark user colours over the dropshadow to be visible.");


ConVar hud_speedometer_vectors("hud_speedometer_vectors", "1", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.", SpeedometerConvarChanged);
ConVar hud_speedometer_vectors_length("hud_speedometer_vectors_length", "0.2f", FCVAR_ARCHIVE, "Sets the length of the velocity and input lines.", true, 0.01f, true, 1.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_vectors_useplayercolour("hud_speedometer_vectors_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer vectors use default colours. 1: Speedometer vectors use the player's colour and complimentary colour.", SpeedometerConvarChanged);

ConVar hud_speedometer_keeplevel("hud_speedometer_keeplevel", "1", FCVAR_ARCHIVE, "0: Speedometer is centred on screen. 1: Speedometer shifts up and down to keep level with the horizon.", SpeedometerConvarChanged);
ConVar hud_speedometer_optimalangle("hud_speedometer_optimalangle", "1", FCVAR_ARCHIVE, "Enables the optimal angle indicator for airstrafing.", SpeedometerConvarChanged);
ConVar hud_speedometer_optimalangle_max("hud_speedometer_optimalangle_max", "10", FCVAR_ARCHIVE, "The maximum range of the optimal angle indicator. The smaller, the easier it is to see the final few degrees as you get closer to perfection.", true, 5.0f, true, 90.0f, SpeedometerConvarChanged);
// This effectively gets halved (0.4 -> 0.2 or 20% of either side of centre)
ConVar hud_speedometer_optimalangle_screenwidth("hud_speedometer_optimalangle_screenwidth", "0.4", FCVAR_ARCHIVE, "The proportion of the screen the optimal angle indicator that will fill, at most.", true, 0.01f, true, 1.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_optimalangle_exponential("hud_speedometer_optimalangle_exponential", "0", FCVAR_ARCHIVE, "Enables exponential scaling on the optimal angle indicator.", SpeedometerConvarChanged);


// Cached versions of the ConVars that get used every frame/draw update (More efficient).
// These shouldn't be members, as their ConVar counterparts are static and global anyway
int iSpeedometer = hud_speedometer.GetInt();
bool bDelta = hud_speedometer_delta.GetBool();
bool bVectors = hud_speedometer_vectors.GetBool();
float flVectorlength = hud_speedometer_vectors_length.GetFloat();
float flSpeedometermax = hud_speedometer_maxspeed.GetFloat();
float flMaxspeed = -1.0f;

int iCentreScreenX = 0;
int iCentreScreenY = 0;

bool bKeepLevel = hud_speedometer_keeplevel.GetBool();
bool bOptimalAngle = hud_speedometer_optimalangle.GetBool();
float flOptimalAngleMax = hud_speedometer_optimalangle_max.GetFloat();
float flOptimalAngleScreenwidth = hud_speedometer_optimalangle_screenwidth.GetFloat();
bool bOptimalAngleExponential = hud_speedometer_optimalangle_exponential.GetBool();

// Used to colour certain parts of the UI in code, while still giving users control over it (Not ideal, ought to be in .res)
extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

extern CMoveData *g_pMoveData;
extern IGameMovement *g_pGameMovement;
extern ConVar mp_maxairspeed;
extern ConVar sv_maxspeed;
extern ConVar sv_airaccelerate;
extern ConVar sv_stopspeed;

// This has to be a non-member/static type of thing otherwise it doesn't work
void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue) {
	// I know this might look like YandereDev levels of if-else, but switching ain't possible on strings
	// (I could have enums and a function to resolve enums to strings but that's just as long-winded for a few strings.)
	// Assumably, the == operator is OK for comparing C++ strings. Send sternly worded emails to alexjames0011@gmail.com if not.
	iSpeedometer = hud_speedometer.GetInt();
	bDelta = hud_speedometer_delta.GetInt() > 0;

	// Only draw vectors only if we're also drawing the speedometer - this isn't entirely necessary - if users want it to be separate it's easy enough to change.
	bVectors = (hud_speedometer_vectors.GetInt() > 0) && (iSpeedometer > 0);
	flVectorlength = hud_speedometer_vectors_length.GetFloat();

	flSpeedometermax = hud_speedometer_maxspeed.GetFloat();
	
	bKeepLevel = hud_speedometer_keeplevel.GetBool();
	bOptimalAngle = hud_speedometer_optimalangle.GetBool();
	flOptimalAngleMax = hud_speedometer_optimalangle_max.GetFloat();
	flOptimalAngleScreenwidth = hud_speedometer_optimalangle_screenwidth.GetFloat();
	bOptimalAngleExponential = hud_speedometer_optimalangle_exponential.GetBool();
	
	// Attempt to automatically reload the HUD and scheme each time
	// Ought to add a ConVar to prevent this optionally
	engine->ExecuteClientCmd("hud_reloadscheme");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudSpeedometer::CHudSpeedometer(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudSpeedometer") {
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// This sets up the callbacks for updating some of the ConVars, as it is poor performance practice to access their values through the convar each update.
	/*hud_speedometer.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_delta.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_vectors.InstallChangeCallback(SpeedometerConvarChanged);
	hud_speedometer_vectors_length.InstallChangeCallback(SpeedometerConvarChanged);*/

	m_pSpeedPercentageMeter = new ContinuousProgressBar(this, "HudSpeedometerBar");

	// text with change in speed on it is HudSpeedometerDelta - Text gets overwritten. You'll know if it fails lol.
	m_pDeltaTextLabel = new Label(this, "HudSpeedometerDelta", "CYMRUAMBYTH");
	m_pDeltaTextLabelDropshadow = new Label(this, "HudSpeedometerDeltaDropshadow", "CYMRUAMBYTH");
	
	// Text gets overwritten 
	m_pSpeedTextLabel = new Label(this, "HudSpeedometerText", "CYMRUAMBYTH");
	m_pSpeedTextLabelDropshadow = new Label(this, "HudSpeedometerTextDropshadow", "CYMRUAMBYTH");	

	//playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	//playerColourNormalised = NormaliseColour(playerColourBase);

	/*if (hud_speedometer_normaliseplayercolour.GetInt() > 0) {
		// Colour to use is now Normlised Colour
		playerColour = &playerColourNormalised;
	}*/
	/*
	if (hud_speedometer_vectors_useplayercolour.GetInt() > 0) {
		playerColourComplementary = GetComplimentaryColour(playerColourBase);

		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplementary;
	}

	if (hud_speedometer_useplayercolour.GetInt() > 0) {

		// If 2, use the complimentary!
		if (hud_speedometer_useplayercolour.GetInt() > 1) {
			playerColour = &playerColourComplementary;
		}

		m_pSpeedTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabel->SetFgColor(*playerColour);
		m_pDeltaTextLabelDropshadow->SetFgColor();

		// Cannot be set in .res? Need to do it here :( sorry HUD Modders
		// If anyone knows how to have this in the .res file instead, that'd be ideal.
		m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	}*/
	SetDialogVariable("speeddelta", "~0");

	// Calls the ApplySchemeSettings
	//engine->ExecuteClientCmd("hud_reloadscheme");

	flMaxspeed = sv_maxspeed.GetFloat();

	UpdateColours();

	UpdateScreenCentre();

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

void CHudSpeedometer::UpdateScreenCentre(void){
	int width, height;
	//surface()->GetScreenSize(width, tall);
	GetSize(width, height);
	iCentreScreenX = width / 2;
	iCentreScreenY = height / 2;

	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	if (!pPlayerBase)
		return;
	FOVScale = iCentreScreenX / pPlayerBase->GetFOV();

	//std::string str = "CENTER SCREEN: X:" + std::to_string(iCentreScreenX) + ", Y:" + std::to_string(iCentreScreenY);
	//Msg(str.c_str());
}

Color CHudSpeedometer::GetComplimentaryColour(Color colorIn) {
	int white = 0xFFFFFF;
	// Save the alpha for output
	int alpha = colorIn.a();

	// Use RGB only, no A!
	colorIn = Color(colorIn.r(), colorIn.g(), colorIn.b(), 0 );
	
	Color out;
	// Calculate complimentary using RGB only
	out.SetRawColor( white - colorIn.GetRawColor() );

	// Add the alpha back in
	out = Color(out.r(), out.g(), out.b(), alpha);

	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudSpeedometer::ApplySchemeSettings(IScheme *pScheme) {
	// load control settings...
	LoadControlSettings("resource/UI/HudSpeedometer.res");
	SetDialogVariable("speeddelta", "~0");

	BaseClass::ApplySchemeSettings(pScheme);

	flMaxspeed = sv_maxspeed.GetFloat();

	UpdateColours();
	UpdateScreenCentre();
}

void CHudSpeedometer::UpdateColours() {

	// Grab the colours
	playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	playerColourComplementary = GetComplimentaryColour(playerColourBase);

	// Update the offwhite's and black's opacity regardless
	colourDefault = Color(colourDefault.r(), colourDefault.g(), colourDefault.b(), hud_speedometer_opacity.GetFloat());
	playerColourShadowbase = Color(playerColourShadowbase.r(), playerColourShadowbase.g(), playerColourShadowbase.b(), hud_speedometer_opacity.GetFloat());
	// By default, off white foreground
	playerColour = &colourDefault;
	// By default, it's just a black shadow
	playerColourShadow = &playerColourShadowbase;


	if (hud_speedometer_useplayercolour.GetInt() >= 1) {

		// If we're using the player's colour, dropshadow with the complement
		playerColour = &playerColourBase;
		playerColourShadow = &playerColourComplementary;

		// If 2, flip them around complimentary! (Why was >1 working for 1 ??? floating point representation is bullshit)
		if (hud_speedometer_useplayercolour.GetInt() >= 2) {
			playerColour = &playerColourComplementary;
			playerColourShadow = &playerColourBase;
		}

		/*Log("Color of Vector Input is %s", *vectorColor_input);
		Log("Color of Vector Velocity is %s", *vectorColor_vel);
		Log("Color of Vector Input is %s", *vectorColor_input);
		Log("Color of Vector Velocity is %s", *vectorColor_vel);*/
	}

	// If we're colouring the on-screen vectors according to the player, recalculate the complimentary colour
	if (hud_speedometer_vectors_useplayercolour.GetInt() >= 1) {
		// We use the player's colour and complimentary colour
		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplementary;
	}
	else {
		// Switch back to the defaults
		vectorColor_input = &defaultInputVectorCol;
		vectorColor_vel = &defaultVelVectorCol;
	}

	// Vector colours need not be set, they're used in the Paint function down below.

	// Update all our HUD elements accordingly
	m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	m_pSpeedTextLabel->SetFgColor(*playerColour);
	m_pSpeedTextLabelDropshadow->SetFgColor(*playerColourShadow);
	m_pDeltaTextLabel->SetFgColor(*playerColour);
	m_pDeltaTextLabelDropshadow->SetFgColor(*playerColourShadow);
} 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudSpeedometer::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist?
	if (!pPlayer)
	{
		return false;
	}

	// Dead men inspect no elements
	if (!pPlayer->IsAlive()) {
		return false;
	}
	// Check convar! If 1 or 2, we draw like cowboys. Shit joke TODO deleteme
	if (iSpeedometer <= 0)
		return false;
	
	// Meter shows only when hud_speedometer is 2
	m_pSpeedPercentageMeter->SetVisible(iSpeedometer >= 2);
	
	// Delta between jumps text only shows if convar is 1 or above
	m_pDeltaTextLabel->SetVisible(bDelta);
	m_pDeltaTextLabelDropshadow->SetVisible(bDelta);

	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	return CHudElement::ShouldDraw();
}


/*Analytically, acos(x) = pi / 2 - asin(x).However if | x | is
* near 1, there is cancellation error in subtracting asin(x)
* from pi / 2.  Hence if x < -0.5,
*
*    acos(x) = pi - 2.0 * asin(sqrt((1 + x) / 2));
*
* or if x > +0.5,
*
*    acos(x) = 2.0 * asin(sqrt((1 - x) / 2)).
*/
// Might fix the stock maths library acos issue
float acos_zdoom(float x) {
	if (x < -0.5f) {
		return M_PI - 2.0f * asin(sqrt((1 + x) / 2));
	}
	else if (x > 0.5f) {
		return 2.0f * asin(sqrt((1 - x) / 2));
	}
	else {
		return M_PI / 2 - asin(x);
	}
}

// Gives the SIGNED radian difference between Start and Target
/*float DeltaAngle(float startAngle, float targetAngle) {
float a = targetAngle - startAngle;
return mod((a + M_PI), (M_PI * 2)) - M_PI;
//return atan2(sin(targetAngle - startAngle), cos(targetAngle - startAngle));
}*/

// Needed over the default % operator
float mod(float a, float n) {
	return a - floor(a / n) * n;
}

float NormalizedPI(float angle) {
	while (angle > M_PI) {
		angle -= (M_PI * 2);
	}
	while (angle < -M_PI) {
		angle += (M_PI * 2);
	}
	return angle;
}

// Taken from ZDoom's source code @ https://github.com/rheit/zdoom
// Modified
// might need to % 360 the return value?
float DeltaAngleRad(float a1, float a2) {
	/*a1 = (int) RAD2DEG(a1) % 360;
	a2 = (int) RAD2DEG(a2) % 360;
	return (a2 - a1);*/

	// rad equivalent of Normalized180
	return NormalizedPI(a2 - a1);

	//Source code:
	// return (a2 - a1).Normalized180();
	// ^ wraps to -180 to 180
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudSpeedometer::OnTick(void) {
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	
	if (!(pPlayer && pPlayerBase))
		return;

	float maxSpeed = pPlayer->MaxSpeed(); // Should get the maximum player move speed respective of class
	Vector velHor(0, 0, 0);
	velHor = pPlayerBase->GetLocalVelocity() * Vector(1, 1, 0); // Player's horizontal velocity.
	float horSpeed = velHor.Length();

	if (m_pSpeedPercentageMeter) {
		// Draw speed text only
		if (iSpeedometer >= 1) {		
			SetDialogVariable("speedhorizontal", RoundFloatToInt(horSpeed) );
		}

		// Draw speed bar, one might call it a "speedometer"... patent pending.
		if (iSpeedometer >= 2) {
			// Set the bar completeness.
			m_pSpeedPercentageMeter->SetProgress(clamp(horSpeed / flSpeedometermax, 0.0f, 1.0f));
		}
	}
	if (m_pDeltaTextLabel) {
		if (bDelta) {
			// Are we grounded?
			bool isGrounded = pPlayerBase->GetGroundEntity();

			if (!isGrounded && groundedInPreviousFrame) {
				cyflymder_echdoe = cyflymder_ddoe;
				cyflymder_ddoe = horSpeed;

				int difference = RoundFloatToInt(cyflymder_ddoe - cyflymder_echdoe);

				// Set the sign (More clarity, keeps width nice and consistent :)
				// If negative, continue as usual, but otherwise prepend a + or ~ if we're >0 or ==0
				std::string s = std::to_string(difference);
				if (difference >= 0) {
					s = (difference == 0 ? "~" : "+") + s;
				}

				char const *pchar = s.c_str();

				SetDialogVariable("speeddelta", pchar);

				groundedInPreviousFrame = false;
			} else {
				groundedInPreviousFrame = isGrounded;
			}
		}
	}
}



void CHudSpeedometer::Paint(void) {
	BaseClass::Paint();

	if (bOptimalAngle) {
		QStrafeJumpHelp();
	}

	if (bVectors) {
		C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

		if (!pPlayerBase)
			return;

		Vector velGlobal(0, 0, 0);
		//velGlobal = pPlayerBase->GetAbsVelocity();
		pPlayerBase->EstimateAbsVelocity(velGlobal);

		// Get the movement angles.
		Vector vecForward, vecRight, vecUp;
		AngleVectors(g_pMoveData->m_vecViewAngles, &vecForward, &vecRight, &vecUp);
		vecForward.z = 0.0f;
		vecRight.z = 0.0f;
		VectorNormalize(vecForward);
		VectorNormalize(vecRight);

		// Copy movement amounts
		float fl_forwardmove_global = velGlobal.x;
		float fl_sidemove_global = velGlobal.y;

		// Find the direction,velocity in the x,y plane.
		Vector vecVelocityDirection(((vecForward.x * fl_forwardmove_global) + (vecRight.x * fl_sidemove_global)),
			((vecForward.y * fl_forwardmove_global) + (vecRight.y * fl_sidemove_global)),
			0.0f);

		float velocityLongitudinalGlobal = -vecVelocityDirection.x * flVectorlength;
		float velocityLateralGlobal = vecVelocityDirection.y * flVectorlength;


		// Draw the input vectors (The player's WASD, as a line)
		float inputLongitudinal = -g_pMoveData->m_flForwardMove * flVectorlength;
		float inputLateral = g_pMoveData->m_flSideMove * flVectorlength;
		surface()->DrawSetColor(*vectorColor_input);
		surface()->DrawLine(iCentreScreenX, iCentreScreenY, iCentreScreenX + inputLateral, iCentreScreenY + inputLongitudinal);


		// Draw the velocity vectors (The player's actual velocity, horizontally, relative to the view direction)
		surface()->DrawSetColor(*vectorColor_vel);
		surface()->DrawLine(iCentreScreenX, iCentreScreenY, iCentreScreenX + velocityLateralGlobal, iCentreScreenY + velocityLongitudinalGlobal);
	}
}

void CHudSpeedometer::DrawTextFromNumber(std::string prefix,  double num, Color col, int x_fromcenter, int y_fromcenter) {
	std::string str = prefix + std::to_string((int)num);
	const char* charlist = str.c_str();
	size_t size = strlen(charlist) + 1;
	wchar_t* iconText = new wchar_t[size];
	mbstowcs(iconText, charlist, size);

	//shadow
	surface()->DrawSetTextColor(0, 0, 0, 255);
	surface()->DrawSetTextPos(iCentreScreenX + x_fromcenter + 2, iCentreScreenY + y_fromcenter + 2);
	surface()->DrawPrintText(iconText, wcslen(iconText));
	//text readout of num
	surface()->DrawSetTextColor(col.r(), col.g(), col.b(), 255);
	surface()->DrawSetTextPos(iCentreScreenX + x_fromcenter, iCentreScreenY + y_fromcenter);
	surface()->DrawPrintText(iconText, wcslen(iconText));
}

// Working in Radians, outputting in degrees
// DeltaAngle is implemented above.
// VectorAngle is just atan2 according to https://zdoom.org/wiki/VectorAngle, hence atan2 has been used (radians) whereby VectorAngle(x,y) = atan2(y,x)
// TICRATE -> integer representing the number of TICKS per second?
void CHudSpeedometer::QStrafeJumpHelp() {

	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	if (!pPlayerBase)
		return;
	//===============================================
	//Gather info
	// Unit directional input - LOCAL, angle from atan2 is 0 at East, so use (forwardmove,-sidemove)
	Vector vel = g_pMoveData->m_vecVelocity;
	vel = Vector(vel.x, vel.y, 0);

	float speed = vel.Length();

	float forwardMove = g_pMoveData->m_flForwardMove;
	float sideMove = g_pMoveData->m_flSideMove;

	Vector wishDir = Vector(forwardMove, -sideMove, 0);
	float wishSpeed = VectorNormalize(wishDir);
	float PAngle = DEG2RAD(g_pMoveData->m_vecViewAngles.y) + atan2(wishDir.y, wishDir.x);
	float velAngle = atan2(vel.y, vel.x);

	if (!g_pMoveData->m_flForwardMove && !g_pMoveData->m_flSideMove) { return; }
		
	// Don't try if the player is holding W.
	if (forwardMove) {
		return;
	}

	// CTFGameMovement::AirMove does this clamp so we do it too (But without the pointless "wishvel" line)
	if (wishSpeed != 0 && (wishSpeed > g_pMoveData->m_flMaxSpeed)) {
		wishSpeed = g_pMoveData->m_flMaxSpeed;
	}
	if (wishSpeed > mp_maxairspeed.GetFloat()) {
		wishSpeed = mp_maxairspeed.GetFloat();
	}


	float maxAccel = min(sv_airaccelerate.GetFloat() * wishSpeed * gpGlobals->interval_per_tick, wishSpeed); //often 30
	float maxCurSpeed = wishSpeed - maxAccel; // often 0


	// Now, AirAccelerate would calculate the "veer" amount, which describes how far off from the current velocity the acceleration is.
	// The more of a veer it is, the influence it has. Same direction = 0 gain, right angles = 1x gain, opposite directions = 2x gain (braking!)
	// Instead of doing that, and then calculating the angular difference between v and v', we will use atan(a / horizontalSpeed)
	// (In radians per second, as velocity is in units per second)
	// atan(30/320) = 0.09348rad = 5.36 degrees
	//float optimalDeltaAnglePerSecond = atan(wishAccel / speed);
	float minAngle = acosf(wishSpeed / speed);
	float optimalAngle = acosf(maxCurSpeed / speed);

	if (DeltaAngleRad(PAngle, velAngle) >= 0) {
		minAngle *= -1;
		optimalAngle *= -1;
	}

	minAngle = RAD2DEG(DeltaAngleRad(PAngle, minAngle + velAngle)) * FOVScale;


	optimalAngle = RAD2DEG(DeltaAngleRad(PAngle, optimalAngle + velAngle))  * FOVScale;

	/*if (bOptimalAngleExponential) {
		float maxAngleOnScreen = flOptimalAngleMax * FOVScale;
		float clampedAngle = min(maxAngleOnScreen, optimalAngle);

		//float exponentialAngle = 1 / (clampedAngle / maxAngleOnScreen);
		//exponentialAngle = min(exponentialAngle, 50);//hardlimit @ 50

		float scaledAngle = (ScreenWidth() * flOptimalAngleScreenwidth / 2.0f) * (clampedAngle / maxAngleOnScreen);

		optimalAngle = scaledAngle;
	}*/

	const float thickness = 20;
	int xOpt = iCentreScreenX - (optimalAngle);
	int xMin = iCentreScreenX - (minAngle);


	int yHorizon = iCentreScreenY;
	if (bKeepLevel) {
		int iX, iY;
		Vector vecTarget = MainViewOrigin() + Vector(MainViewForward().x, MainViewForward().y, 0.0f);
		bool bOnscreen = GetVectorInScreenSpace(vecTarget, iX, iY);
		yHorizon = iY;
	}

	int yTop = yHorizon;
	int yBottom = yHorizon + thickness;
	yTop = clamp(yTop, 0, ScreenHeight() - thickness);
	yBottom = clamp(yBottom, 0 + thickness, ScreenHeight());

	// Gotta do it top left to bottom right
	if (xMin >= xOpt) {
		surface()->DrawSetColor(playerColourComplementary);
		surface()->DrawFilledRect(xOpt + SHADOW_OFFSET, yTop + SHADOW_OFFSET, xMin + SHADOW_OFFSET, yBottom + SHADOW_OFFSET);
		surface()->DrawSetColor(*playerColour);
		surface()->DrawFilledRect(xOpt, yTop, xMin, yBottom);
	}
	else {
		surface()->DrawSetColor(playerColourComplementary);
		surface()->DrawFilledRect(xMin + SHADOW_OFFSET, yTop + SHADOW_OFFSET, xOpt + SHADOW_OFFSET, yBottom + SHADOW_OFFSET);
		surface()->DrawSetColor(*playerColour);
		surface()->DrawFilledRect(xMin, yTop, xOpt, yBottom);
	}

	DrawTextFromNumber("MIN: ", minAngle / FOVScale, Color(200, 255, 200, 25), 150, -20);
	DrawTextFromNumber("OPTIMAL: ", optimalAngle / FOVScale, Color(255, 200, 200, 25), 150, -10);
}