//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
//#include "GreyscaleTest_ps20b.inc"
#include "BlurDOF_ps30.inc"

BEGIN_VS_SHADER_FLAGS(BlurDOF, "Help for DoF Blur", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(TEXELSIZE, SHADER_PARAM_TYPE_VEC2, "[0.000520833333 0.000925925926]", "")
SHADER_PARAM(FOCUSSCALE, SHADER_PARAM_TYPE_FLOAT, "0.15", "")
SHADER_PARAM(FOCUSDISTANCE, SHADER_PARAM_TYPE_FLOAT, "0.175", "")
//SHADER_PARAM(BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
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

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		//pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_STATIC_PIXEL_SHADER(BlurDOF_ps30);
		SET_STATIC_PIXEL_SHADER(BlurDOF_ps30);

		//if (params[LINEARWRITE]->GetFloatValue())
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);

		/*float texelSize[2];
		pShaderAPI->GetWorldSpaceCameraPosition(eyePos);
		ShaderViewport_t *viewport;
		pShaderAPI->GetViewports(viewport, 1);
		viewport->m_nWidth;
		pShaderAPI->SetPixelShaderConstant(13, texelSize, true);*/

		// is this right?
		const float flFocusScale = params[FOCUSSCALE]->GetFloatValue();
		const float flFocusDistance = params[FOCUSDISTANCE]->GetFloatValue();

		pShaderAPI->SetPixelShaderConstant(13, params[TEXELSIZE]->GetVecValue(), true );
		pShaderAPI->SetPixelShaderConstant(17, &flFocusScale, true);
		pShaderAPI->SetPixelShaderConstant(18, &flFocusDistance, true);

		//BindTexture(SHADER_SAMPLER1, BLURTEXTURE, -1);
		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_DYNAMIC_PIXEL_SHADER(BlurDOF_ps30);
		SET_DYNAMIC_PIXEL_SHADER(BlurDOF_ps30);
	}
	Draw();
}
END_SHADER
