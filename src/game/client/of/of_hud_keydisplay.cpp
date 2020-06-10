//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A key display system by Terradice ( discord: Terradice#9125 )
// Shows what movement keys are currently being pressed.
//
//
// $NoKeywords: $
//=============================================================================


#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudKeyDisplay : public CHudElement, public EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudKeyDisplay, EditablePanel);

public:
    CHudKeyDisplay(const char *pElementName);
    
    virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	// virtual void	Paint(void);
	void UpdateColours(void);

private:
    const char* buttonForward;
    const char* buttonBackward;
    const char* buttonLeft;
    const char* buttonRight;
    const char* buttonDuck;
    const char* buttonJump;

    Label* m_pForwardTextLabel;
    Label* m_pBackwardTextLabel;
    Label* m_pLeftTextLabel;
    Label* m_pRightTextLabel;
    Label* m_pJumpTextLabel;
    Label* m_pDuckTextLabel;

    Color inactiveColor;
    Color activeColor;
};

DECLARE_HUDELEMENT(CHudKeyDisplay);

void KeyDisplayConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);
ConVar hud_keydisplay("hud_keydisplay", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows movement keys being pressed.", KeyDisplayConvarChanged);
int iDisplayKeys =  hud_keydisplay.GetInt();

void KeyDisplayConvarChanged(IConVar *var, const char *pOldValue, float flOldValue) {
	iDisplayKeys = hud_keydisplay.GetInt();
	// Attempt to automatically reload the HUD and scheme each time
	engine->ExecuteClientCmd("hud_reloadscheme");
}


CHudKeyDisplay::CHudKeyDisplay(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudKeyDisplay") 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

    m_pForwardTextLabel = new Label(this, "ForwardKey", "W");
    m_pBackwardTextLabel = new Label(this, "BackKey", "A");
    m_pLeftTextLabel = new Label(this, "LeftKey", "S");
    m_pRightTextLabel = new Label(this, "RightKey", "D");
    m_pJumpTextLabel = new Label(this, "JumpKey", "SPACE");
    m_pDuckTextLabel = new Label(this, "CrouchKey", "CTRL");

    inactiveColor = Color(100, 100, 100, 255);
    activeColor = Color(255, 255, 100, 255);

    SetHiddenBits(HIDEHUD_MISCSTATUS);
    ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudKeyDisplay::ApplySchemeSettings(IScheme *pScheme) 
{
	// load control settings...
	LoadControlSettings("resource/UI/HudKeyDisplay.res");

    buttonForward = engine->Key_LookupBinding( "+forward" );
    buttonBackward = engine->Key_LookupBinding( "+back" );
    buttonDuck = engine->Key_LookupBinding( "+duck" );
    buttonLeft = engine->Key_LookupBinding( "+moveleft" );
    buttonRight = engine->Key_LookupBinding( "+moveright" );
    buttonJump = engine->Key_LookupBinding( "+jump" );
    
    SetDialogVariable( "forwardkey", buttonForward );
    SetDialogVariable( "rightkey", buttonRight );
    SetDialogVariable( "backkey", buttonBackward );
    SetDialogVariable( "leftkey", buttonLeft);
    SetDialogVariable( "crouchkey", buttonForward );
    SetDialogVariable( "jumpkey", buttonForward );

	m_pForwardTextLabel->SetFgColor(inactiveColor);
	m_pBackwardTextLabel->SetFgColor(inactiveColor);
	m_pLeftTextLabel->SetFgColor(inactiveColor);
	m_pRightTextLabel->SetFgColor(inactiveColor);
	m_pJumpTextLabel->SetFgColor(inactiveColor);
	m_pDuckTextLabel->SetFgColor(inactiveColor);

	BaseClass::ApplySchemeSettings(pScheme);
}


bool CHudKeyDisplay::ShouldDraw(void) 
{
    C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

    // Do I even exist?
    if (!pPlayer)
    {
        return false;
    }

    // not big surpris
    if (!pPlayer->IsAlive()) {
        return false;
    }

    if(iDisplayKeys > 0)
    {
        return CHudElement::ShouldDraw();
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudKeyDisplay::OnTick(void) 
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	
	if (!(pPlayer && pPlayerBase))
		return;

    m_pForwardTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_FORWARD) ? activeColor : inactiveColor );
    m_pBackwardTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_BACK) ? activeColor : inactiveColor );
    m_pLeftTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_MOVELEFT) ? activeColor : inactiveColor );
    m_pRightTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_MOVERIGHT) ? activeColor : inactiveColor );
    m_pJumpTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_JUMP) ? activeColor : inactiveColor );
    m_pDuckTextLabel->SetFgColor( (pPlayerBase->m_nButtons & IN_DUCK) ? activeColor : inactiveColor );
}