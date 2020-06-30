// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#ifndef GREYSCALETEST_PS30_H
#define GREYSCALETEST_PS30_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class GreyscaleTest_ps30_Static_Index
{
public:
	GreyscaleTest_ps30_Static_Index( )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_GreyscaleTest_ps30 1


class GreyscaleTest_ps30_Dynamic_Index
{
public:
	GreyscaleTest_ps30_Dynamic_Index( )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_GreyscaleTest_ps30 1


#endif	// GREYSCALETEST_PS30_H