//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Medium file for the colourblind pattern shader.
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
#include "RedToPattern_ps30.inc"

BEGIN_VS_SHADER_FLAGS(RedToPattern, "Red channel to pattern index shader.", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(SCRATCHBUFFER, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallFB1", "")
SHADER_PARAM(PATTERNTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "effects/colourblind/16x16_Patterns_RGBA", "")
SHADER_PARAM(UVSCALING, SHADER_PARAM_TYPE_FLOAT, "15.0", "")
SHADER_PARAM(TEXELSIZE, SHADER_PARAM_TYPE_VEC2, "[0.000520833333 0.000925925926]", "")
SHADER_PARAM(SATURATIONMAXIMA, SHADER_PARAM_TYPE_VEC4, "[0.9 0.9 0.9 0.9]", "")
SHADER_PARAM(SATURATIONMINIMA, SHADER_PARAM_TYPE_VEC4, "[0.4 0.4 0.4 0.4]", "")
SHADER_PARAM(INTENSITYMULTIPLIER, SHADER_PARAM_TYPE_FLOAT, "0.75", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
	if (params[SCRATCHBUFFER]->IsDefined())
	{
		LoadTexture(SCRATCHBUFFER);
	}
	if (params[PATTERNTEXTURE]->IsDefined())
	{
		LoadTexture(PATTERNTEXTURE);
	}
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
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true); // Pattern Image RGBA
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true); // Scratch buffer

		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_STATIC_PIXEL_SHADER(RedToPattern_ps30);
		SET_STATIC_PIXEL_SHADER(RedToPattern_ps30);

		//if (params[LINEARWRITE]->GetFloatValue())
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);
		BindTexture(SHADER_SAMPLER1, PATTERNTEXTURE, -1);
		BindTexture(SHADER_SAMPLER2, SCRATCHBUFFER, -1);


		// TODO fix pattern not being square on other resolutions
		/*
		float texelSize[2];
		//pShaderAPI->GetWorldSpaceCameraPosition(eyePos);
		ShaderViewport_t *viewport;
		pShaderAPI->GetViewports(viewport, 1);
		texelSize[0] = viewport->m_nWidth;
		texelSize[1] = viewport->m_nHeight;
		pShaderAPI->SetPixelShaderConstant(13, texelSize, true);
		*/

		pShaderAPI->SetPixelShaderConstant(17, params[TEXELSIZE]->GetVecValue(), true);
		float uvScaling = params[UVSCALING]->GetFloatValue();
		pShaderAPI->SetPixelShaderConstant(18, &uvScaling, true);
		pShaderAPI->SetPixelShaderConstant(19, params[INTENSITYMULTIPLIER]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(20, params[SATURATIONMINIMA]->GetVecValue(), true);
		pShaderAPI->SetPixelShaderConstant(21, params[SATURATIONMAXIMA]->GetVecValue(), true);

		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_DYNAMIC_PIXEL_SHADER(RedToPattern_ps30);
		SET_DYNAMIC_PIXEL_SHADER(RedToPattern_ps30);
	}
	Draw();
}
END_SHADER
