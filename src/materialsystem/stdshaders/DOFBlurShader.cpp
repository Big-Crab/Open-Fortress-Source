
#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
#include "include/postproc_dofblursmart_ps30.inc"
//#include "include/postproc_dofblursmart_vs30.inc"

BEGIN_VS_SHADER(DOFBlurSmart, "Help for DOF Blur Smart" )
	BEGIN_SHADER_PARAMS
	// TODO add shader params to the shader and add them here
	SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
	//	SHADER_PARAM(BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "base texture")
	//	SHADER_PARAM(BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap")
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		/*if (!params[BUMPFRAME]->IsDefined())
		{
			params[BUMPFRAME]->SetIntValue(0);
		}*/
		if (params[FBTEXTURE]->IsDefined())
		{
			LoadTexture(FBTEXTURE);
		}
	}

	SHADER_FALLBACK
	{
		return "Wireframe";
	}

	// Note: You can create member functions inside the class definition.
	/*int SomeMemberFunction()
	{
		return 0;
	}*/

	// Is this right???
	virtual bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame = true)
	{
		return true;
	}

	SHADER_INIT
	{
		//LoadTexture(BASETEXTURE);
		LoadTexture(FRAME); // I think this is for framebuffer?	
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
		/*	// Enable the texture for base texture and lightmap.
			pShaderShadow->EnableTexture(SHADER_TEXTURE_STAGE0, true);
			pShaderShadow->EnableTexture(SHADER_TEXTURE_STAGE1, true);

			sdk_lightmap_vs20_Static_Index vshIndex;
			pShaderShadow->SetVertexShader("sdk_lightmap_vs20", vshIndex.GetIndex());

			sdk_lightmap_ps20_Static_Index pshIndex;
			pShaderShadow->SetPixelShader("sdk_lightmap_ps20", pshIndex.GetIndex());

			DefaultFog();			*/

			pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

			//postproc_dofblursmart_vs30_Static_Index vshIndex(pShaderShadow, params);
			//pShaderShadow->SetVertexShader("postproc_dofblursmart_vs30", vshIndex.GetIndex());

			postproc_dofblursmart_ps30_Static_Index pshIndex(pShaderShadow, params);
			pShaderShadow->SetPixelShader("postproc_dofblursmart_ps30", pshIndex.GetIndex());
		}
					
		DYNAMIC_STATE
		{

			BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);

			DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

			//BindTexture(SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME);
			//pShaderAPI->BindLightmap(SHADER_TEXTURE_STAGE1);

			postproc_dofblursmart_ps30_Dynamic_Index pshIndex(pShaderAPI);
			pShaderAPI->SetPixelShaderIndex(pshIndex.GetIndex());
			//DECLARE_DYNAMIC_PIXEL_SHADER(postproc_dofblursmart_ps30);
			//SET_DYNAMIC_PIXEL_SHADER(postproc_dofblursmart_ps30);
		}
		Draw();
	}
END_SHADER