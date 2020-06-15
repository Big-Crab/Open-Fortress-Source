//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A weapon wheel selection system by Hogyn Melyn & Terradice (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589 / discord: Terradice#9125)
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Controls.h>
#include "view.h"
#include "iclientmode.h"

#include <vgui_controls/Controls.h>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//8
#define NUM_VERTS_SPOKE 7


class CHudWeaponSpoke;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudWeaponWheel : public CHudElement, public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHudWeaponWheel, vgui::Frame);

public:
	CHudWeaponWheel(const char *pElementName);
	~CHudWeaponWheel();

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	virtual void OnMouseWheeled(int delta);

	void	WeaponSelected(int id);

	void	CheckMousePos();

	class WheelSegment {
	public:
		WheelSegment() {
		
		}
		// Screenspace XY centre
		int centreX, centreY;

		int *outLinesX, *outLinesY;

		vgui::Vertex_t vertices[NUM_VERTS_SPOKE];

		// The angle around the wheel that this segment is centred
		float centreAngle;
	};

	WheelSegment *segments;

	CHudWeaponSpoke **buttons;

private:
	// How many segments do we have? Probably should be dependent on gamemode/mutator (Arsenal = 3, Otherwise = 7)
	int		numberOfSegments = 8;
	// The degrees of margin between each of the wheel segments
	int		marginBetweenSegments = 10;
	// Which segment is currently selected
	int		currentlySelectedSegment = -1;
	// Wheel centre radius (in px)
	float	wheelRadius = 50.0f;
	// Wheel outer spoke radius (in px) (calculated as wheelRadius + outerRadius)
	float	outerRadius = 60.0f;
	// The length extra squared bit we add to the end
	float	segmentExtensionLength = 20.0f;
	// px margin between wheel centre circle and the segments
	int		wheelMargin = 10;
	// The extra "height" added to each segment (helps with low-slot instances)
	float	verticalHeight = 10.0f;

	int		slotSelected = -1;

	// If the player is mousewheeling on a slot, use that instead of the highlighted
	bool	useHighlighted = false;

	void RefreshWheel(void);

	void RefreshCentre(void);
	int iCentreScreenX;
	int iCentreScreenY;

	int iCentreScreenX_const;
	int iCentreScreenY_const;

	bool	lastWheel = false;
	void	CheckWheel();

	Color* weaponColors;
};

DECLARE_HUDELEMENT(CHudWeaponWheel);

//-----------------------------------------------------------------------------
// Purpose: Weapon selection button
//-----------------------------------------------------------------------------
class CHudWeaponSpoke : public CHudElement, public Button {
	DECLARE_CLASS_SIMPLE(CHudWeaponSpoke, Button);

public:
	CHudWeaponSpoke(const char *pElementName, CHudWeaponWheel *parent, int id);

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	int id = -1;
	CHudWeaponWheel* parent;
};

//Button(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget = NULL, const char *pCmd = NULL);
CHudWeaponSpoke::CHudWeaponSpoke(const char *pElementName, CHudWeaponWheel *parent, int id) : CHudElement(pElementName), BaseClass(parent, pElementName, pElementName)
{
	this->parent = parent;
	this->id = id;
}

void CHudWeaponSpoke::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	parent->WeaponSelected(id);

}
void CHudWeaponSpoke::OnCursorExited() 
{
	BaseClass::OnCursorExited();
}

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

ConVar hud_weaponwheel_quickswitch("hud_weaponwheel_quickswitch", "0", FCVAR_ARCHIVE, "Weapon wheel selects as soon as the mouse leaves the centre circle, instead of when the weapon wheel key is lifted.");

bool bWheelActive = false;
void IN_WeaponWheelDown() {
	bWheelActive = true;
}

void IN_WeaponWheelUp() {
	bWheelActive = false;
}

ConCommand hud_weaponwheel_on("+weaponwheel", IN_WeaponWheelDown);
ConCommand hud_weaponwheel_off("-weaponwheel", IN_WeaponWheelUp);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudWeaponWheel::CHudWeaponWheel(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudWeaponwheel")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits(HIDEHUD_MISCSTATUS);
	//SetHiddenBits(HIDEHUD_WEAPONSELECTION);

	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetProportional(true);

	RefreshCentre();
	RefreshWheel();
	
	//SetUseCaptureMouse(true);

	ivgui()->AddTickSignal(GetVPanel());
}

void CHudWeaponWheel::WeaponSelected(int id) 
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	CBaseHudWeaponSelection::GetInstance()->SelectWeaponSlot( id+1 );
	
	char s[24];
	Q_snprintf(s, sizeof(s), "weapon %d selected. \n", id+1);
	Msg(s);

	bWheelActive = false;
}

void CHudWeaponWheel::CheckMousePos()
{
	int x, y;
	vgui::input()->GetCursorPosition(x, y);
	x -= iCentreScreenX;
	y -= iCentreScreenY;
	float distanceSqrd = (x*x) + (y*y);

	// width of each segment
	float pointAngleFromCentre = 360 / (numberOfSegments);

	// If the cursor is outside of the wheel (+ the margin size)
	if (distanceSqrd >= (wheelRadius + wheelMargin)*(wheelRadius + wheelMargin))
	{
		float mousePosAsAngle = RAD2DEG(atan2(-(float)y, (float)x)) + 90.0f - pointAngleFromCentre;
		int selected = RoundFloatToInt( mousePosAsAngle / (360 / numberOfSegments) ) + 1;
		if (selected < 0)
			selected += 8; //+= numberOfSegments
		
		char s[10];
		Q_snprintf(s, sizeof(s), "Angle: %d.\n", mousePosAsAngle);

		if (hud_weaponwheel_quickswitch.GetInt() > 0)
		{
			WeaponSelected( selected );
		}
		else
		{
			if (slotSelected != selected)
			{
				slotSelected = selected;
				useHighlighted = true;
			}
		}
	}
	else
	{
		slotSelected = -1;
		useHighlighted = false;
	}
}

void CHudWeaponWheel::RefreshWheel(void)
{
	if (segments) {
		delete[] segments;
	}
	segments = new WheelSegment[numberOfSegments];

	float pointAngleFromCentre = 360 / (2 * numberOfSegments);

	// From the top, clockwise, in deg
	float currentCentreAngle = 0;
	for (int i = 0; i < numberOfSegments; i++)
	{
		//segments[i] = WheelSegment();
		segments[i].centreAngle = currentCentreAngle;

		segments[i].centreX = iCentreScreenX + ( sin(DEG2RAD(currentCentreAngle) ) * (wheelRadius + wheelMargin));
		segments[i].centreY = iCentreScreenY + ( cos(DEG2RAD(currentCentreAngle) ) * (wheelRadius + wheelMargin));

		segments[i].vertices[0].Init(Vector2D(segments[i].centreX, segments[i].centreY));

		Vector2D centreToPoint = Vector2D(sin(DEG2RAD(currentCentreAngle)), cos(DEG2RAD(currentCentreAngle)));

		segments[i].vertices[1].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin))));

		segments[i].vertices[2].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius))));

		segments[i].vertices[3].Init(Vector2D(segments[i].vertices[2].m_Position.x + (centreToPoint.x * segmentExtensionLength), segments[i].vertices[2].m_Position.y + (centreToPoint.y * segmentExtensionLength)));

		segments[i].vertices[5].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin + outerRadius))));

		segments[i].vertices[4].Init(Vector2D(segments[i].vertices[5].m_Position.x + (centreToPoint.x * segmentExtensionLength), segments[i].vertices[5].m_Position.y + (centreToPoint.y * segmentExtensionLength)));

		segments[i].vertices[6].Init(Vector2D(iCentreScreenX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin)), iCentreScreenY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (marginBetweenSegments / 2))) * (wheelRadius + wheelMargin))));

		//segments[i].vertices[7].Init(Vector2D(segments[i].centreX, segments[i].centreY));

		// then back to [0]
		

		currentCentreAngle += 360 / numberOfSegments;
	}

	if (buttons) {
		for (int i = 0; i<numberOfSegments; i++){
			delete buttons[i];
		}
		delete[] buttons;
	}

	for (int i = 0; i < numberOfSegments; i++){
		if (segments[i].outLinesX) {
			delete[] segments[i].outLinesX;
		}
		if (segments[i].outLinesY) {
			delete[] segments[i].outLinesY;
		}
		segments[i].outLinesX = new int[numberOfSegments];
		segments[i].outLinesY = new int[numberOfSegments];

		for (int j = 0; j < NUM_VERTS_SPOKE; j++)
		{
			segments[i].outLinesX[j] = segments[i].vertices[j].m_Position.x;
			segments[i].outLinesY[j] = segments[i].vertices[j].m_Position.y;
		}
	}

	/*buttons = new CHudWeaponSpoke*[numberOfSegments];
	for (int i = 0; i < numberOfSegments; i++) {
		char s[8];
		Q_snprintf(s, sizeof(s), "%d", i+1);

		// Will callback to CHudWeaponWheel::WeaponSelected(id)
		buttons[i] = new CHudWeaponSpoke(s, this, i);
		buttons[i]->SetBounds(segments[i].centreX - 10, segments[i].centreY - 10, 20, 20);
		buttons[i]->SetBgColor(Color(255, 0, 0, 200));
		buttons[i]->SetFgColor(Color(255, 0, 0, 200));
	}*/

}

void CHudWeaponWheel::RefreshCentre(void)
{
	int width, height;
	GetSize(width, height);
	iCentreScreenX_const = width / 2;
	iCentreScreenY_const = height / 2;
	iCentreScreenX = iCentreScreenX_const;
	iCentreScreenY = iCentreScreenY_const;
}

void CHudWeaponWheel::Paint(void)
{
	if (bWheelActive) {
		// Wheel centre
		surface()->DrawSetColor(Color (255, 255, 0, 255));
		surface()->DrawOutlinedCircle(iCentreScreenX, iCentreScreenY, wheelRadius, 12);
	
		surface()->DrawSetColor(Color(255, 255, 255, 100));

		// Spokes!
		for (int i = 0; i < numberOfSegments; i++) {
			surface()->DrawSetColor(weaponColors[i]); //surface()->DrawSetColor(*weaponColors[i]);
			surface()->DrawSetTexture(-1);
			surface()->DrawTexturedPolygon(NUM_VERTS_SPOKE, segments[i].vertices);
		}

		// Now the outline
		surface()->DrawSetColor(Color(255, 255, 255, 255));
		for (int i = 0; i < numberOfSegments; i++) {
			surface()->DrawSetTexture(-1);
<<<<<<< HEAD
			surface()->DrawPolyLine(segments[i].outLinesX, segments[i].outLinesY, NUM_VERTS_SPOKE);
=======
>>>>>>> 560caf4df5cc4a4a20b39e611d91fdd2dc39e206
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudWeaponWheel::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudWeaponwheel.res");
	
	RefreshCentre();

	RefreshWheel();

	if (weaponColors) 
	{
		delete[] weaponColors;
	}

	weaponColors = new Color[8];
	weaponColors[0] = Color(200, 200, 200, 150);	//melee
	weaponColors[1] = Color(255, 255, 100, 150);	//pistols
	weaponColors[2] = Color(255, 100, 100, 150);	//shotguns
	weaponColors[3] = Color(25, 25, 100, 150);		//automatics
	weaponColors[4] = Color(25, 150, 25, 150);		//rifles
	weaponColors[5] = Color(255, 255, 255, 150);	//specials
	weaponColors[6] = Color(50, 25, 10, 150);		//explosives
	weaponColors[7] = Color(255, 50, 255, 150);		//supers

	BaseClass::ApplySchemeSettings(pScheme);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudWeaponWheel::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist?
	if (!pPlayer)
		return false;

	// Dead men inspect no elements
	if (!pPlayer->IsAlive())
		return false;


	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	if (bWheelActive)
	{
		return CHudElement::ShouldDraw();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudWeaponWheel::OnTick(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

	if (!(pPlayer && pPlayerBase))
		return;

	// If weapon wheel active bool has changed, change mouse input capabilities etc
	if (lastWheel != bWheelActive) {
		CheckWheel();
	}

	if (bWheelActive) 
	{
		CheckMousePos();
	}

	lastWheel = bWheelActive;
}

void CHudWeaponWheel::CheckWheel()
{
	if (bWheelActive) {
		Activate();
		SetMouseInputEnabled(true);
		RequestFocus();

		vgui::input()->SetCursorPos(iCentreScreenX_const, iCentreScreenY_const);
		
		// since Linux can't snap to centre :( we just start the weapon wheel wherever their mouse is
		// On Windows, this should still let us start at iCentreScreenXY
//		int x = 0;
//		int y = 0;
		vgui::input()->GetCursorPos(iCentreScreenX, iCentreScreenY);
		Msg("Pos: %i, %i\n", iCentreScreenX, iCentreScreenY);
	}
	else {
		// Switch to the one the mouse is over
		if (useHighlighted/* && hud_fastswitch.GetInt() <= 0*/)
		{
			// Select it and close
			WeaponSelected( slotSelected );
			char s[16];
			Q_snprintf(s, 16, "slot: %d\n", slotSelected + 1);
			Msg(s);
			Msg("Selected weapon on close (useHighlighted was true!)\n");
		}
		SetMouseInputEnabled(false);
		SetVisible(false);
	}
}


void CHudWeaponWheel::OnMouseWheeled(int delta)
{
	if (bWheelActive) {
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (!pPlayer)
			return;

		if (slotSelected >= 0) {
			CBaseHudWeaponSelection::GetInstance()->SelectWeaponSlot(slotSelected + 1);
			// Until the player highlights another slot, if they release the key then just use whatever was selected with the mousewheel ^
			useHighlighted = false;
		}
	}
}

CHudWeaponWheel::~CHudWeaponWheel()
{
	for (int i = 0; i<numberOfSegments; i++){
		delete buttons[i];
	}
	delete[] buttons;
	if (weaponColors)
	{
		delete[] weaponColors;
	}
}