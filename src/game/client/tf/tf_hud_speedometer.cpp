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

// NEW STUFF ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const double M_U_DEG = 360.0 / 65536;
const double M_U_RAD = M_PI / 32768;
#define M_PI_2 M_PI / 2
#define M_PI_4 M_PI / 4

double anglemod_deg(double a) {
	return M_U_DEG * ((int)(a / M_U_DEG) & 0xffff);
}

double anglemod_rad(double a) {
	return M_U_RAD * ((int)(a / M_U_RAD) & 0xffff);
}

static double strafe_theta_opt(double speed, double L, double tauMA) {
	double tmp = L - tauMA;
	if (tmp <= 0)
		return M_PI_2;
	if (tmp < speed)
		return std::acos(tmp / speed);
	return 0;
}

static void strafe_fme_vec(double vel[2], const double avec[2], double L, double tauMA) {
	double tmp = L - vel[0] * avec[0] - vel[1] * avec[1];
	if (tmp < 0)
		return;
	if (tauMA < tmp)
		tmp = tauMA;
	vel[0] += avec[0] * tmp;
	vel[1] += avec[1] * tmp;
}

static void strafe_side(double &yaw, int &Sdir, int &Fdir, double vel[2], double theta, double L, double tauMA, int dir) {
	double phi;
	// This was intende to reduce the overall shaking in TAS use. Could adjust/remove this?
	if (theta >= M_PI_2 * 0.75) {
		Sdir = dir;
		Fdir = 0;
		phi = std::copysign(M_PI_2, dir);
	} else if (M_PI_2 * 0.25 <= theta && theta <= M_PI_2 * 0.75) {
		Sdir = dir;
		Fdir = 1;
		phi = std::copysign(M_PI_4, dir);
	} else {
		Sdir = 0;
		Fdir = 1;
		phi = 0;
	}

	if (std::fabs(vel[0]) > 0.1 || std::fabs(vel[1]) > 0.1) {
		yaw = std::atan2(vel[1], vel[0]);
	}

	yaw += phi - std::copysign(theta, dir);
	double yawcand[2] = {
		anglemod_rad(yaw), anglemod_rad(yaw + std::copysign(M_U_RAD, yaw))
	};

	double avec[2] = { std::cos(yawcand[0] - phi), std::sin(yawcand[0] - phi) };
	double tmpvel[2] = { vel[0], vel[1] };

	strafe_fme_vec(vel, avec, L, tauMA);

	avec[0] = std::cos(yawcand[1] - phi);
	avec[1] = std::sin(yawcand[1] - phi);
	
	strafe_fme_vec(tmpvel, avec, L, tauMA);

	if (tmpvel[0] * tmpvel[0] + tmpvel[1] * tmpvel[1] > vel[0] * vel[0] + vel[1] * vel[1]) {
		vel[0] = tmpvel[0];
		vel[1] = tmpvel[1];
		yaw = yawcand[1];
	} else {
		yaw = yawcand[0];
	}
}

static void strafe_side_opt(double &yaw, int &Sdir, int &Fdir, double vel[2], double L, double tauMA, int dir) {
	double speed = std::hypot(vel[0], vel[1]);
	double theta = strafe_theta_opt(speed, L, tauMA);
	strafe_side(yaw, Sdir, Fdir, vel, theta, L, tauMA, dir);
}

// NEW STUFF ENDS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSpeedometer : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, EditablePanel);

	//friend class CGameMovement;
	//friend class CTFGameMovement;

public:
	CHudSpeedometer(const char *pElementName);

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	void UpdateColours(void);

	Color GetComplimentaryColour( Color color );

	// Callback functions for when certain ConVars are changed - more efficient, and it lets us call hud_reloadscheme automatically!
	// I've opted to use a single function and discrimination of the convar's string name to decide what to change, as opposed to one function for each var
	// as this keeps things a little tidier and is only called when the cvars get updated, so using strings isn't a performance concern.
	// Doesn't work as a member :(((((( 
	//void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);

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


	// Variables and functions for calculating the best air strafing circles.
	Vector optimalDirectionVector;	// Best position for the player's look (yaw only) to be in order to air accelerate at maximum speed
	double optimalYawAngle;			// Best yaw for maximum air acceleration
	bool hasOptimalVector = false; // to stop us drawing a vector when we haven't made one
	void OptimalLookThink();	// Update loop for calculating optimal vectors

	int lastDirectionInput = 0;

	//float GetAirSpeedCap(void); //Shameful....

	void UpdateScreenCentre(void);

	void DrawTextFromNumber(double num, Color col, int xpos, int ypos);

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
int iSpeedometer = -1;
bool bDelta = true;
bool bVectors = true;
float flVectorlength = 0.0f;
float flSpeedometermax = 1000.0f;
float flMaxairspeed = -1.0f; 
float flMaxspeed = -1.0f;
float flAiraccelerate = -1.0f;

int iCentreScreenX = 0;
int iCentreScreenY = 0;

bool bKeepLevel = true;
bool bOptimalAngle = true;
float flOptimalAngleMax = 10.0f;
float flOptimalAngleScreenwidth = 0.4f;
bool bOptimalAngleExponential = false;

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
	engine->ExecuteClientCmd("hud_reloadscheme");
}

// Used to colour certain parts of the UI in code, while still giving users control over it (Not ideal, ought to be in .res)
extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

extern CMoveData *g_pMoveData;
extern IGameMovement *g_pGameMovement;
extern ConVar mp_maxairspeed;
extern ConVar sv_maxspeed;
extern ConVar sv_airaccelerate;

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

	flMaxairspeed = mp_maxairspeed.GetFloat();
	flMaxspeed = sv_maxspeed.GetFloat();
	flAiraccelerate = sv_airaccelerate.GetFloat();

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

	flMaxairspeed = mp_maxairspeed.GetFloat();
	flMaxspeed = sv_maxspeed.GetFloat();
	flAiraccelerate = sv_maxspeed.GetFloat();

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

	if (bOptimalAngle) {
		// Do da big thonk about vectors
		OptimalLookThink();
	}
}



void CHudSpeedometer::Paint(void) {
	BaseClass::Paint();

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


		// Draw the optimal vector we need to align to as a magenta thing offset by 10,10
		if (!hasOptimalVector)
			return;

		// Assuming radians. 
		//float angle = atan2f(vecForward.y, vecForward.x) - atan2(optimalDirectionVector.y, optimalDirectionVector.x);
		
		// vec view angles yaw is in deg, ranges -180 to 180
		// convert the optimal to -180 to 180, since that's what translates immediately to the current UI bar and matches World Rotations™
		// ^ NOPE IM CONFUSED SO WE'RE GOING INTO 0-360 space
		// AND THEN BACK INTO +-180 SPACE 
		// These are still very much in world units, which is kinda useless.
		// The difference, "angle", is certainly useful, however!
		double currentYawAngle_standardised = anglemod_deg(g_pMoveData->m_vecViewAngles.y) - 180;
		double optimalYawAngle_standardised = anglemod_deg(RAD2DEG(optimalYawAngle)) - 180;
		double angle = anglemod_deg(currentYawAngle_standardised - optimalYawAngle_standardised);	// get into a readable angle metric ya FUCK
		if (angle > 180)
			angle -= 360;
		
		// Clamp to range.
		double clampedAngle = angle;
		if (abs(angle) > flOptimalAngleMax) {
			clampedAngle = min(flOptimalAngleMax, abs(angle));
			clampedAngle *= (angle > 0) ? 1 : -1;
		}

		// angle should now be -180 to 180 for left and right!


		// Normalise to range -pi + pi. (-180, 180)
		//if (angle > M_PI)        { angle -= 2 * M_PI; }
		//else if (angle <= -M_PI) { angle += 2 * M_PI; }

		//angle = RAD2DEG(angle);

		// ok... so... filled rect is a fuck
		// it doesn't work if the end points are more to the right or bottom than the centre.......
		
		//surface()->DrawFilledRect(iCentreScreenX, iCentreScreenY, iCentreScreenX + angle, iCentreScreenY + 21);

		// signed (-1 to 1) * (1920 * 0.2)
		float angleOnScreen = (clampedAngle / flOptimalAngleMax) * (ScreenWidth() * (flOptimalAngleScreenwidth / 2));
		
		//float pitch;
		//factor in vertical fov, adjust to screenspace (0 to (screentall - thickness)?)
		
		const float thickness = 20;
		if (angleOnScreen >= 0) {
			surface()->DrawSetColor(playerColourComplementary);
			surface()->DrawFilledRect(iCentreScreenX + SHADOW_OFFSET, iCentreScreenY + SHADOW_OFFSET, iCentreScreenX + SHADOW_OFFSET + angleOnScreen, iCentreScreenY + SHADOW_OFFSET + thickness);
			surface()->DrawSetColor(*playerColour);
			surface()->DrawFilledRect(iCentreScreenX, iCentreScreenY, iCentreScreenX + angleOnScreen, iCentreScreenY + thickness);
		}
		else {
			surface()->DrawSetColor(playerColourComplementary);
			surface()->DrawFilledRect(iCentreScreenX + angleOnScreen + SHADOW_OFFSET, iCentreScreenY + SHADOW_OFFSET, iCentreScreenX + SHADOW_OFFSET, iCentreScreenY + SHADOW_OFFSET + thickness);
			surface()->DrawSetColor(*playerColour);
			surface()->DrawFilledRect(iCentreScreenX + angleOnScreen, iCentreScreenY, iCentreScreenX, iCentreScreenY + thickness);
		}
		// a new approach. headache.
		//DrawBox(iCentreScreenX, iCentreScreenY + 25, angle, 10, Color(255, 255, 0, 255), 255.0f, false);
		//DrawBox(0, 0, angle, 10, Color(255, 255, 0, 255), 255.0f, false);

		DrawTextFromNumber(angle, Color(255, 0, 0, 255), 70, 70);

		DrawTextFromNumber(currentYawAngle_standardised, Color(255, 255, 0, 255), 80, 85);
		DrawTextFromNumber(optimalYawAngle_standardised, Color(255, 255, 175, 255), 80, 100);

		// draw the P E R F E C T angle
		//DrawTextFromNumber(estimatedOptimalYaw, Color(255, 255, 175, 255), -25, -60);

		//DrawTextFromNumber(g_pMoveData->m_vecVelocity.x, Color(255, 255, 175, 255), -25, -60);
		//DrawTextFromNumber(g_pMoveData->m_vecVelocity.y, Color(255, 255, 175, 255), -25, -45);
	}
}

void CHudSpeedometer::DrawTextFromNumber(double num, Color col, int x_fromcenter, int y_fromcenter) {
	std::string str = std::to_string((int)num);
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

/*
I have derived some expressions using information about Half Life (1, GoldSrc) Tool-assisted-speedruns.
Most, if not all of the variables used in HL1 for movement etc are the same, and in fact share the same names.
This is not to say Valve mindlessly reused HL1 code for essential things, but rather that they built HL2/TF2's movement
on the same model. I cannot use greek letters :'( 

Cofiwch: peidiwch defnyddio square root os ti'n gallu! Os tisio hynny (||A|| = sqrt(A.x ^ 2 + A.y ^ 2)),
meddwl am gwneud hyn yn lle: ||A||^2 = A.x^2 + A.y^2
Gweithio yn rhywbeth^2 yn lle! :) Mae helpu i osgoi sqrt (inefficient)


v = current velocity
v' = new velocity
â = unit acceleration vector
a = Ff^ + Ss^ , where F is m_flForwardMove and S is m_flSideMove, while f^ and s^ are the forward and rightward vectors respectively.
||a|| = sqrt(F^2 + S^2)

THETA = "yaw angle", calculated as the angle between v and â

L = 30 in HL1. Seems to be a magic number, defined in-line. Poor practice!
::
:: // cap speed
::if (wishspd > 30)
::		wishspd = 30;
::
(from pm_shared.c)

The OF/HL2/TF2 equivalent is:
:: // Cap speed
:: if ( wishspd > GetAirSpeedCap() )
::		wishspd = GetAirSpeedCap();
::
(from gamemovement.cpp)

L = GetAirSpeedCap();
L = either of_shield_charge_speed or mp_maxairspeed based on its logic

TAU = the timestep/frame time, which I think here is best retrieved from gpGlobals->frametime (It's what's used for logic in gamemovement.cpp, but is unrelated to Think times?)

Mm = sv_maxspeed
"It is not always that M = Mm, since M can be affected by duckstate and the values of F, S, and U (upward movement)"
M = min (sv_maxspeed, sqrt(F^2 + S^2)
"Besides, we will assume that ||<F,S>||>=M. If this is not the case, we must replace M->||<F,S>|| for all appearances of M below."

A = sv_airaccelerate (10 in HL1, 500 in OF)

Gamma1 = TAU * M * A

Gamma2	= L - v.a
		= L - ||v||cos(THETA)

Mu =	{ min (Gamma1, Gamma2)  :  if Gamma2 > 0
		{  0					:	otherwise

To maximise  ||v'||:

(Translated to deg from rad for readability)
THETA =	{ 90deg	:	if L - (TAU * M * A) <= 0
		{ ZETA	:	if 0 < L - (TAU * M * A) <= ||v||
		{ 0		:	otherwise

ZETA is obtained by solving Gamma1 = Gamma2
Cos(ZETA) = (L - (TAU * M * A)) / ||v||

These functions attempt to calculate the FME.
This is how I have modelled airstrafing, so any errors here will explain if my indicators are dysfunctional lol.

(Vector velocity, int direction, float L, float tauMA);
(Vector velocity, float theta, float L, float tauMA);
*/

// I could not find a way to access this from CGameMovement nor CTFGameMovement.
// I tried friend classing, I tried externs, I tried includes. They did not work. Forgive me, Christ.
// If you find a way, reader, please update it, so that my soul may rest once more.
/*float CHudSpeedometer::GetAirSpeedCap(void) {
	if (m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) && mp_maxairspeed.GetFloat() < of_shield_charge_speed.GetFloat())
		return of_shield_charge_speed.GetFloat();
	return mp_maxairspeed.GetFloat();
}*/


//1st pass; no optimisation
void CHudSpeedometer::OptimalLookThink() {
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	//g_pMoveData
	//g_GameMovement

	if (!pPlayer || !pPlayerBase)
		return;
	
	// we need CTFGameMovement::GetAirSpeedCap, but practically it's just mp_maxairspeed (unless we're charging, but you can't airstrafe then.)
	float L = flMaxairspeed;

	float tau = gpGlobals->frametime;

	// These are local/relative to view!!!!
	float F = g_pMoveData->m_flForwardMove;
	float S = g_pMoveData->m_flSideMove;

	float M = min(flMaxspeed, hypotf(F, S));

	float A = flAiraccelerate;

	Vector velocity(0, 0, 0);
	//velocity = pPlayerBase->GetLocalVelocity();
	//pPlayerBase->EstimateAbsVelocity(velocity);
	// This is global - not relative to the view.
	velocity = g_pMoveData->m_vecVelocity;

	// Convert to local! - todo use me?
	/*Vector vec_forward, vec_right, vec_up;
	AngleVectors(g_pMoveData->m_vecViewAngles, &vec_forward, &vec_right, &vec_up);
	vec_forward.z = 0.0f;
	vec_right.z = 0.0f;
	VectorNormalize(vec_forward);
	VectorNormalize(vec_right);

	// Find the direction,velocity in the x,y plane. I think?
	Vector vec_velocity_worldspace(((vec_forward.x * F) + (vec_right.x * S)),
		((vec_forward.y * F) + (vec_right.y * S)),
		0.0f);*/
	
	int	direction = S > 0 ? 1 : -1;

	// We're not strafing
	if (S == 0)
		return;
	
	// New Method
	double yaw = g_pMoveData->m_vecViewAngles.y;
	double tauMA = tau * M * A;
	int Sdir = 0, Fdir = 0; // why was this originally left at 0? should it not just be S^ and F^? maybe because it is for automatically generating S and F afterward in TAS?
	
	// Added this
	if(F!=0){
		Fdir = Sign(F);
	}
	if(S!=0){
		Sdir = Sign(S);
	}
	
	double vel_double[2] = { velocity.x, velocity.y };
	strafe_side_opt(yaw, Sdir, Fdir, vel_double, L, tauMA, direction);

	optimalYawAngle = yaw;
	hasOptimalVector = true; // shit code lmao

	// If we're not strafing, just use the last strafe input
	/*if (S == 0) {
		direction = lastDirectionInput;
	} else {
		lastDirectionInput = direction;
	}*/

	/*hasOptimalVector = CalculateOptimal(velocity, direction, L, tau * M * A, mostEfficientVector);
	if (hasOptimalVector) {
		optimalDirectionVector = mostEfficientVector;
	

		// Assuming radians. 
		float angle = atan2f(vec_forward.y, vec_forward.x) - atan2(optimalDirectionVector.y, optimalDirectionVector.x);
	
		// Normalise to range -pi + pi. (-180, 180)
		if (angle > M_PI)        { angle -= 2 * M_PI; }
		else if (angle <= -M_PI) { angle += 2 * M_PI; }
		angle = RAD2DEG(angle); // needs to be in deg?

		// "Greater Precision" - theta is angle
		double phi = atan(abs(S / F));
		double alpha = atan2(velocity.y, velocity.x);
		double beta = alpha +  ((phi - angle) * (direction > 0) ? 1 : -1);
		double theta1 = anglemod(beta);
		double theta2 = anglemod(beta + (Sign(beta) * u));

		// now we decide which of the thetax gives the higher speed
		Vector velocityTheta1 = Vector(0,0,0);
		Vector velocityTheta2 = Vector(0,0,0);
		CalculateOptimalFromTheta(velocity, theta1, L, tau * M * A, velocityTheta1);
		CalculateOptimalFromTheta(velocity, theta2, L, tau * M * A, velocityTheta2);

		estimatedOptimalYaw = (velocityTheta1.Length() > velocityTheta2.Length()) ? theta1 : theta2;
	}*/
}