//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render pattern over team-coloured client renderable objects.
// Assist colour-blind/hard-of-sight individuals with identifying objects coded with
// team colours.
//===============================================================================

#include "cbase.h"
#include "of_colorblind_helper.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"

// This is typically the un-processed framebuffer after the main rendering loop - the order of post processing effects
// will affect this framebuffer.
#define FULL_FRAME_TEXTURE "_rt_FullFrameFB"
#define SCRATCH_FULLFRAME "_rt_FullFrameFB1"

//#ifdef GLOWS_ENABLE

ConVar of_colorblind_pattern_enable("of_colorblind_pattern_enable", "0", FCVAR_ARCHIVE, "Enable patterns for team-coloured entities to assist with colourblindness.");
//ConVar glow_outline_effect_width("glow_outline_width", "10.0f", FCVAR_CHEAT, "Width of glow outline effect in screen space.");

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CTeamPatternObjectManager g_TeamPatternObjectManager;

// The 4 "teams" (literally teams in TF2C, teamplay, but in OF DM just a way of dividing up players)
const float CTeamPatternObjectManager::s_rgflStencilTeams[] = {1.0f, 0.75f, 0.5f, 0.25f};

/*	#############################################################################################
	#							How the Pattern Effect Works									#
	#___________________________________________________________________________________________#
	#	5 texture files exist, a plain white image, and 4 patterns (checkers, lines, dots etc)	#
	#	The stencil will include each object with the effect applied, writing a value to the	#
	#	red channel based on its team (0.25 to 1.0). This value is multiplied by 4 (number of	#
	#	teams), and rounded down. This allows the 5 texture files to be indexed based on the	#
	#	team a pixel belongs to. Branching (if statements) is expensive in shaders, hence why	#
	#	I have designed it this way. Additional teams can be added, but 4 is the current limit	#
	#	in the shader. The colours on-screen are also processed according to the VMT parameters #
	#	allowing control of which parts of each entity are patterned - it can be limited to		#
	#	certain hues + a hue offset (e.g. reds = 0 +- 15 degrees of hue), a saturation minimum	#
	#	(e.g. colour must be at least 50% saturated = 0.5), and a value minimum (e.g. colour	#
	#	must be at least 60% in luminanace/value = 0.6).										#
	#																							#
	#	The red-channel stencil pass is written to a scratch buffer (_rt_SmallFB1?), which		#
	#	should be passed to the shader as 														#
	#																							#
	#############################################################################################
*/

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState(CMatRenderContextPtr &pRenderContext)
	{
		pRenderContext->SetStencilEnable(m_bEnable);
		pRenderContext->SetStencilFailOperation(m_FailOp);
		pRenderContext->SetStencilZFailOperation(m_ZFailOp);
		pRenderContext->SetStencilPassOperation(m_PassOp);
		pRenderContext->SetStencilCompareFunction(m_CompareFunc);
		pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
		pRenderContext->SetStencilTestMask(m_nTestMask);
		pRenderContext->SetStencilWriteMask(m_nWriteMask);
	}
};

void CTeamPatternObjectManager::RenderTeamPatternEffects(const CViewSetup *pSetup)
{
	if (g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0())
	{
		if (of_colorblind_pattern_enable.GetBool())
		{
			CMatRenderContextPtr pRenderContext(materials);

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport(nX, nY, nWidth, nHeight);

			//PIXEvent _pixEvent(pRenderContext, "EntityGlowEffects");
			PIXEvent _pixEvent(pRenderContext, "EntityTeamPatternEffects");
			ApplyEntityTeamPatternEffects(pSetup, pRenderContext, nX, nY, nWidth, nHeight);
		}
	}
}

static void SetRenderTargetAndViewPort(ITexture *rt, int w, int h)
{
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->SetRenderTarget(rt);
	pRenderContext->Viewport(0, 0, w, h);
}

void CTeamPatternObjectManager::RenderTeamPatternModels(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext)
{
	//==========================================================================================//
	// This renders solid pixels with the correct coloring for each object that needs the glow.	//
	// After this function returns, this image will then be blurred and added into the frame	//
	// buffer with the objects stenciled out.													//
	//==========================================================================================//
	pRenderContext->PushRenderTargetAndViewport();

	// Save modulation color and blend
	Vector vOrigColor;
	render->GetColorModulation(vOrigColor.Base());
	float flOrigBlend = render->GetBlend();

	// Get pointer to FullFrameFB // OLD
//	ITexture *pRtFullFrame = NULL;
//	pRtFullFrame = materials->FindTexture(FULL_FRAME_TEXTURE, TEXTURE_GROUP_RENDER_TARGET);
//	SetRenderTargetAndViewPort(pRtFullFrame, pSetup->width, pSetup->height);

	// Get pointer to our render target
	ITexture *pRtScratch = NULL;
	pRtScratch = materials->FindTexture(SCRATCH_FULLFRAME, TEXTURE_GROUP_RENDER_TARGET);
	SetRenderTargetAndViewPort(pRtScratch, pSetup->width, pSetup->height);

	// Clear colour but not depth or stencil
	pRenderContext->ClearColor3ub(0, 0, 0);
	pRenderContext->ClearBuffers(true, false, false);

	// Set override material for glow color
	IMaterial *pMatGlowColor = NULL;
	pMatGlowColor = materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true);
	g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);

	// Disabled stencil? Doesn't write I guess
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = false;
	stencilState.m_nReferenceValue = 0;
	stencilState.m_nTestMask = 0xFF;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_KEEP;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;

	stencilState.SetStencilState(pRenderContext);

	//==================//
	// Draw the objects //
	//==================//
	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
	{
		if (m_TeamPatternObjectDefinitions[i].IsUnused())
			continue;

//		render->SetBlend(m_TeamPatternObjectDefinitions[i].m_flGlowAlpha);
//		Vector vGlowColor = m_GlowObjectDefinitions[i].m_vGlowColor * m_GlowObjectDefinitions[i].m_flGlowAlpha;
//		render->SetColorModulation(&vGlowColor[0]); // This only sets rgb, not alpha
		
		render->SetBlend(1.0f);
		float vTeamColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		//float vTeamColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		switch (m_TeamPatternObjectDefinitions[i].m_nTeam)
		{
		case CTeamPatternObject::CB_TEAM_RED:
			//vTeamColor = Vector(1.0f, 0.0f, 0.0f);
			//s_rgflStencilTeams
			vTeamColor[0] = 1.0f;
			break;
		case CTeamPatternObject::CB_TEAM_BLU:
			//vTeamColor[2] = 1.0f;
			vTeamColor[0] = 0.75f;
			break;
		case CTeamPatternObject::CB_TEAM_GRN:
			//vTeamColor[1] = 1.0f;
			vTeamColor[0] = 0.5f;
			break;
		case CTeamPatternObject::CB_TEAM_YLW:
			vTeamColor[0] = 0.25f;
			//vTeamColor[0] = 1.0f;
			//vTeamColor[1] = 1.0f;
			break;
		case CTeamPatternObject::CB_TEAM_NONE:
			vTeamColor[0] = 0.0f; // no effect
			break;
		default:
			vTeamColor[0] = 0.0f; // no effect
			break;
		}

		//render->SetColorModulation(&vTeamColor[0]); // This only sets rgb, not alpha
		render->SetColorModulation(vTeamColor); // This only sets rgb, not alpha ?

		m_TeamPatternObjectDefinitions[i].DrawModel();
	}

	if (g_bDumpRenderTargets)
	{
		DumpTGAofRenderTarget(pSetup->width, pSetup->height, "TeamPatternRedModels");
	}

	g_pStudioRender->ForcedMaterialOverride(NULL);
	render->SetColorModulation(vOrigColor.Base());
	render->SetBlend(flOrigBlend);

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	stencilStateDisable.SetStencilState(pRenderContext);

	pRenderContext->PopRenderTargetAndViewport();
}

void CTeamPatternObjectManager::ApplyEntityTeamPatternEffects(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext, int x, int y, int w, int h)
{
	//=======================================================//
	// Render objects into stencil buffer					 //
	//=======================================================//
	// Set override shader to the same simple shader we use to render the glow models
	IMaterial *pMatGlowColor = materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true);
	g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	float flSavedBlend = render->GetBlend();

	// Set alpha to 0 so we don't touch any color pixels
	render->SetBlend(0.0f);
	pRenderContext->OverrideDepthEnable(true, false);

	int iNumTeamPatternObjects = 0;

	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
	{
		if (m_TeamPatternObjectDefinitions[i].IsUnused())
			continue;

/*			if (m_TeamPatternObjectDefinitions[i].m_bRenderWhenOccluded && m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded)
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 1;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_REPLACE;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState(pRenderContext);

				m_GlowObjectDefinitions[i].DrawModel();
			}
			else if (m_GlowObjectDefinitions[i].m_bRenderWhenOccluded)
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 1;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_KEEP;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState(pRenderContext);

				m_GlowObjectDefinitions[i].DrawModel();
			}
			else if (m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded) */
//			{
			ShaderStencilState_t stencilState;
			stencilState.m_bEnable = true;
			stencilState.m_nReferenceValue = 2;
			stencilState.m_nTestMask = 0x1;
			stencilState.m_nWriteMask = 0x3;
			stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
			stencilState.m_PassOp = STENCILOPERATION_INCRSAT;	// Increment the stencil-buffer entry, clamping to the maximum value. (I think it's min(++buffer, maxval) ???)
			stencilState.m_FailOp = STENCILOPERATION_KEEP;		// Does not update the entry in the stencil buffer
			stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;	// Replace the stencil-buffer entry with a reference value.

			stencilState.SetStencilState(pRenderContext);

			m_TeamPatternObjectDefinitions[i].DrawModel();
			//}
		
			iNumTeamPatternObjects++;
	}

	/*	// Need to do a 2nd pass to warm stencil for objects which are rendered only when occluded
	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
	{
		if (m_TeamPatternObjectDefinitions[i].IsUnused())
			continue;

		if (m_TeamPatternObjectDefinitions[i].m_bRenderWhenOccluded && !m_TeamPatternObjectDefinitions[i].m_bRenderWhenUnoccluded)
		{
			ShaderStencilState_t stencilState;
			stencilState.m_bEnable = true;
			stencilState.m_nReferenceValue = 2;
			stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
			stencilState.m_PassOp = STENCILOPERATION_REPLACE;
			stencilState.m_FailOp = STENCILOPERATION_KEEP;
			stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
			stencilState.SetStencilState(pRenderContext);

			m_GlowObjectDefinitions[i].DrawModel();
		}
	}
	*/

	pRenderContext->OverrideDepthEnable(false, false);
	render->SetBlend(flSavedBlend);
	stencilStateDisable.SetStencilState(pRenderContext);
	g_pStudioRender->ForcedMaterialOverride(NULL);

	// If there aren't any objects to glow, don't do all this other stuff
	// this fixes a bug where if there are glow objects in the list, but none of them are glowing,
	// the whole screen blooms.
	if (iNumTeamPatternObjects <= 0)
		return;

	//=============================================
	// Render the glow colors to SCRATCH_FULLFRAME (was _rt_FullFrameFB)
	//=============================================
	{
		PIXEvent pixEvent(pRenderContext, "RenderGlowModels");
		RenderTeamPatternModels(pSetup, pRenderContext);
	}

	// Get viewport
	int nSrcWidth = pSetup->width;
	int nSrcHeight = pSetup->height;
	int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
	pRenderContext->GetViewport(nViewportX, nViewportY, nViewportWidth, nViewportHeight);
	
	// Get material and texture pointers
	ITexture *pRtFullSize1 = materials->FindTexture(SCRATCH_FULLFRAME, TEXTURE_GROUP_RENDER_TARGET);

	{


		//=======================================================================================================//
		// I don't think we need this part? All it does is get rid of the glow where the original models are
		// it seems to call the haloaddoutline_ps20 shader
		// now we use _rt_SmallFB1 as the scratch for the stencil
		//
		// At this point, pRtQuarterSize0 is filled with the fully colored glow around everything as solid glowy //
		// blobs. Now we need to stencil out the original objects by only writing pixels that have no            //
		// stencil bits set in the range we care about.                                                          //
		//=======================================================================================================//
		//IMaterial *pMatHaloAddToScreen = materials->FindMaterial("dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true);
		IMaterial *pMatPattern = materials->FindMaterial("ColourBlindRedToPattern", TEXTURE_GROUP_OTHER, true);
		//IMaterial *pMatHaloAddToScreen = materials->FindMaterial("dofblur", TEXTURE_GROUP_OTHER, true);

		// Do not fade the glows out at all (weight = 1.0)
		//IMaterialVar *pDimVar = pMatHaloAddToScreen->FindVar("$C0_X", NULL);
		//pDimVar->SetFloatValue(1.0f);
		//pDimVar->SetFloatValue(0.5f);

		
		// Set stencil state
		ShaderStencilState_t stencilState;
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0x0; // We're not changing stencil
		stencilState.m_nTestMask = 0xFF;
		stencilState.m_nReferenceValue = 0x0;
		//stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;	// is (stencilRef & stencilMask) == (stencilBufferValue & stencilMask)?
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_NOTEQUAL;
		stencilState.m_PassOp = STENCILOPERATION_KEEP;	// keeps the buffer data
		stencilState.m_FailOp = STENCILOPERATION_KEEP;	// keeps the buffer data
		stencilState.m_ZFailOp = STENCILOPERATION_KEEP;	// keeps the buffer data
		stencilState.SetStencilState(pRenderContext);
		

		// Draw quad (quarter size?)
		/*pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreen, 0, 0, nViewportWidth, nViewportHeight,
			0.0f, -0.5f, nSrcWidth / 4 - 1, nSrcHeight / 4 - 1,
			pRtQuarterSize1->GetActualWidth(),
			pRtQuarterSize1->GetActualHeight());*/
		pRenderContext->DrawScreenSpaceRectangle(pMatPattern, 0, 0, nViewportWidth, nViewportHeight,
			0.0f, -0.5f, nSrcWidth - 1, nSrcHeight - 1,
			pRtFullSize1->GetActualWidth(),
			pRtFullSize1->GetActualHeight());

		stencilStateDisable.SetStencilState(pRenderContext);
	}
}

void CTeamPatternObjectManager::TeamPatternObjectDefinition_t::DrawModel()
{
	if (m_hEntity.Get())
	{
		m_hEntity->DrawModel(STUDIO_RENDER);
		C_BaseEntity *pAttachment = m_hEntity->FirstMoveChild();

		while (pAttachment != NULL)
		{
			if (!g_TeamPatternObjectManager.HasTeamPatternEffect(pAttachment) && pAttachment->ShouldDraw())
			{
				pAttachment->DrawModel(STUDIO_RENDER);
			}
			pAttachment = pAttachment->NextMovePeer();
		}
	}
}

//#endif // GLOWS_ENABLE