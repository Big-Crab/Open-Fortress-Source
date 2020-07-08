//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Medium file for the ToxicOverlay shader used for the TFC Medkit
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
#include "ToxicOverlay_ps30.inc"

BEGIN_VS_SHADER_FLAGS(ToxicOverlay, "Help for Toxic Overlay shader", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(CIRCLEOVERLAY, SHADER_PARAM_TYPE_TEXTURE, "effects/circle_altered", "")
SHADER_PARAM(VEINRBAMAP, SHADER_PARAM_TYPE_TEXTURE, "effects/poison/bloodshot_veins_bluetimemap", "")
SHADER_PARAM(VEINNORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "effects/berserk_overlay_normal", "")
SHADER_PARAM(TIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
	if (params[CIRCLEOVERLAY]->IsDefined())
	{
		LoadTexture(CIRCLEOVERLAY);
	}
	if (params[VEINRBAMAP]->IsDefined())
	{
		LoadTexture(VEINRBAMAP);
	}
	if (params[VEINNORMALMAP]->IsDefined())
	{
		LoadTexture(VEINNORMALMAP);
	}
	/*if (params[BLURTEXTURE]->IsDefined())
	{
	LoadTexture(BLURTEXTURE);
	}*/
}

SHADER_FALLBACK
{
	// Requires DX9 + above (In the year of our Lord 2020? Who the fuck is running this on Windows 98? My phone probably does DX11. My fuckin pacemaker probably runs DX9 at least.)
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Assert(0);
		return "Wireframe";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		

		pShaderShadow->EnableDepthWrites(false);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true); // FB
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true); // Vein Normal map
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true); // Vein RBA
		pShaderShadow->EnableTexture(SHADER_SAMPLER3, true); // Dusty circle

		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_STATIC_PIXEL_SHADER(ToxicOverlay_ps30);
		SET_STATIC_PIXEL_SHADER(ToxicOverlay_ps30);

		//if (params[LINEARWRITE]->GetFloatValue())
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);
		BindTexture(SHADER_SAMPLER1, VEINNORMALMAP, -1);
		BindTexture(SHADER_SAMPLER2, VEINRBAMAP, -1);
		BindTexture(SHADER_SAMPLER3, CIRCLEOVERLAY, -1);

		/*float texelSize[2];
		pShaderAPI->GetWorldSpaceCameraPosition(eyePos);
		ShaderViewport_t *viewport;
		pShaderAPI->GetViewports(viewport, 1);
		viewport->m_nWidth;
		pShaderAPI->SetPixelShaderConstant(13, texelSize, true);*/

		// is this right?
//		const float flTime = params[TIME]->GetFloatValue();
		//const float flFocusDistance = params[FOCUSDISTANCE]->GetFloatValue();

//		pShaderAPI->SetPixelShaderConstant(13, params[TEXELSIZE]->GetVecValue(), true );
//		pShaderAPI->SetPixelShaderConstant(17, &flFocusScale, true);
		
		// Pass time in
		float curtime = pShaderAPI->CurrentTime();
		pShaderAPI->SetPixelShaderConstant(17, &curtime, true);

		//BindTexture(SHADER_SAMPLER1, BLURTEXTURE, -1);
		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_DYNAMIC_PIXEL_SHADER(ToxicOverlay_ps30);
		SET_DYNAMIC_PIXEL_SHADER(ToxicOverlay_ps30);
	}
	Draw();
}
END_SHADER
