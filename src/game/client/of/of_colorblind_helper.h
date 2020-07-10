//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a pattern over team-coloured client renderable objects.
// Helps with visibility of players, buildings, and projectiles.
//
//===============================================================================

//#ifndef GLOW_OUTLINE_EFFECT_H
//#define GLOW_OUTLINE_EFFECT_H

#if defined( COMPILER_MSVC )
#pragma once
#endif

#include "utlvector.h"
#include "mathlib/vector.h"

//#ifdef GLOWS_ENABLE

class C_BaseEntity;
class CViewSetup;
class CMatRenderContextPtr;

//static const int GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS = -1;

class CTeamPatternObjectManager
{
public:

	const static float s_rgflStencilTeams[4];

	CTeamPatternObjectManager() :
		m_nFirstFreeSlot(TeamPatternObjectDefinition_t::END_OF_FREE_LIST)
	{
	}

	int RegisterTeamPatternObject(C_BaseEntity *pEntity, int nTeam)
	{
		// early out if no entity is actually registered
		if (!pEntity)
			return NULL;

		int nIndex;
		if (m_nFirstFreeSlot == TeamPatternObjectDefinition_t::END_OF_FREE_LIST)
		{
			nIndex = m_TeamPatternObjectDefinitions.AddToTail();
		}
		else
		{
			nIndex = m_nFirstFreeSlot;
			m_nFirstFreeSlot = m_TeamPatternObjectDefinitions[nIndex].m_nNextFreeSlot;
		}

		m_TeamPatternObjectDefinitions[nIndex].m_hEntity = pEntity;
		m_TeamPatternObjectDefinitions[nIndex].m_nTeam = nTeam;
		m_TeamPatternObjectDefinitions[nIndex].m_nNextFreeSlot = TeamPatternObjectDefinition_t::ENTRY_IN_USE;

		return nIndex;
	}

	void UnregisterTeamPatternObject(int nTeamPatternObjectHandle)
	{
		Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());

		m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
		m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_hEntity = NULL;
		m_nFirstFreeSlot = nTeamPatternObjectHandle;
	}

	void SetEntity(int nTeamPatternObjectHandle, C_BaseEntity *pEntity)
	{
		Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());
		m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_hEntity = pEntity;
	}

	void SetTeam(int nTeamPatternObjectHandle, const int nTeam)
	{
		Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());
		m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_nTeam = nTeam;
	}

	bool HasTeamPatternEffect(C_BaseEntity *pEntity) const
	{
		for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
		{
			if (!m_TeamPatternObjectDefinitions[i].IsUnused() && m_TeamPatternObjectDefinitions[i].m_hEntity.Get() == pEntity)
			{
				return true;
			}
		}

		return false;
	}

	void RenderTeamPatternEffects(const CViewSetup *pSetup, int nSplitScreenSlot);

private:

	void RenderTeamPatternModels(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext);
	void ApplyEntityTeamPatternEffects(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext, int x, int y, int w, int h);

	struct TeamPatternObjectDefinition_t
	{
		bool ShouldDraw(int nSlot) const
		{
			return m_hEntity.Get() &&
				m_hEntity->ShouldDraw() &&
				!m_hEntity->IsDormant();
		}

		bool IsUnused() const { return m_nNextFreeSlot != TeamPatternObjectDefinition_t::ENTRY_IN_USE; }
		void DrawModel();

		EHANDLE m_hEntity;

		// The team that is used for this object in its stencil pass
		int m_nTeam;

		// Linked list of free slots
		int m_nNextFreeSlot;

		// Special values for TeamPatternObjectDefinition_t::m_nNextFreeSlot
		static const int END_OF_FREE_LIST = -1;
		static const int ENTRY_IN_USE = -2;
	};

	CUtlVector< TeamPatternObjectDefinition_t > m_TeamPatternObjectDefinitions;
	int m_nFirstFreeSlot;
};

// TODO is this defined in the .cpp?
extern CTeamPatternObjectManager g_TeamPatternObjectManager;

class CTeamPatternObject
{
public:

	// Teams are encoded in the stencil buffer in the red channel.
	// This enum is used to index the stencil buffer colour to use
	enum COLORBLIND_TEAMS { CB_TEAM_RED, CB_TEAM_BLU, CB_TEAM_GRN, CB_TEAM_YLW };

	CTeamPatternObject(C_BaseEntity *pEntity, const int nTeam = CB_TEAM_RED)
	{
		m_nTeamPatternObjectHandle = g_TeamPatternObjectManager.RegisterTeamPatternObject(pEntity, nTeam);
	}

	~CTeamPatternObject()
	{
		g_TeamPatternObjectManager.UnregisterTeamPatternObject(m_nTeamPatternObjectHandle);
	}

#ifdef OF_CLIENT_DLL
	void Destroy(void)
	{
		g_TeamPatternObjectManager.UnregisterTeamPatternObject(m_nTeamPatternObjectHandle);
	}
#endif

	void SetEntity(C_BaseEntity *pEntity)
	{
		g_TeamPatternObjectManager.SetEntity(m_nTeamPatternObjectHandle, pEntity);
	}

	void SetTeam(const int nTeam)
	{
		g_TeamPatternObjectManager.SetTeam(m_nTeamPatternObjectHandle, nTeam);
	}

	// Add more accessors/mutators here as needed

private:
	int m_nTeamPatternObjectHandle;

	// Assignment & copy-construction disallowed
	CTeamPatternObject(const CTeamPatternObject &other);
	CTeamPatternObject& operator=(const CTeamPatternObject &other);
};

//#endif // GLOWS_ENABLE

//#endif // GLOW_OUTLINE_EFFECT_H