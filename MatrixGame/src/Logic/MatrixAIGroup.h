// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "MatrixTactics.h"
#include "3g.hpp"

// Groups
#define DIF_GROUP_R    200
#define REGROUP_PERIOD 100
#define JUST_PERIOD    100

class CMatrixRobotAI;
class CMatrixSideUnit;
class CMatrixGroup;
class CMatrixTactics;

/*enum Team
{
    TEAM_UNDEF,
    SYSTEM,
    ALPHA,
    BRAVO,
    CHARLIE
};*/

/**
 * @brief Envelope over CMatrixMapStatic object.
 *
 * CMatrixGroup consists of CMatrixGroupObject objects.
 */
class CMatrixGroupObject : public CMain {
public:
    int m_Team;
    CMatrixGroup *m_ParentGroup;
    CMatrixMapStatic *m_Object;
    CMatrixGroupObject *m_NextObject;
    CMatrixGroupObject *m_PrevObject;

public:
    void SetObject(CMatrixMapStatic *object) { m_Object = object; }

    CMatrixMapStatic *GetObject() { return m_Object; }

    CMatrixGroup *GetParentGroup() { return m_ParentGroup; }
    void SetParentGroup(CMatrixGroup *group) { m_ParentGroup = group; }

    void SetTeam(int team) { m_Team = team; }

    int GetTeam() { return m_Team; }

    CMatrixGroupObject() {
        m_Object = NULL;
        m_NextObject = NULL;
        m_PrevObject = NULL;
        m_ParentGroup = NULL;
        m_Team = -1;
    }
    ~CMatrixGroupObject();
};

/**
 * @brief Group of objects united by something. Used both by player side and computer sides.
 *
 * Calculates the center position and the average velocity of the objects consisting the group once every JUST_PERIOD time.
 *
 * Is a double linked list element by the way. So multiple groups can be stored as list elements.
 */
class CMatrixGroup : public CMain {
public:
    int m_Team;
    int m_ObjectsCnt;
    int m_RobotsCnt;
    int m_FlyersCnt;
    int m_BuildingsCnt;
    // CMatrixTactics*     m_Tactics;
    D3DXVECTOR3 m_GroupPosition;
    int m_Id;
    float m_GroupSpeed;
    int m_SimpleTimer;

    CMatrixGroupObject *m_FirstObject;
    CMatrixGroupObject *m_LastObject;
    CMatrixGroup *m_NextGroup;
    CMatrixGroup *m_PrevGroup;

public:
    void AddObject(CMatrixMapStatic *object, int team);
    void RemoveObject(CMatrixMapStatic *object);
    bool RemoveObject(int num);
    void RemoveAll();
    void RemoveBuildings();
    void SortFlyers();

    CMatrixMapStatic *GetObjectByN(int num);

    int GetObjectsCnt() { return m_ObjectsCnt; }

    int GetRobotsCnt() { return m_RobotsCnt; }
    int GetFlyersCnt() { return m_FlyersCnt; }
    int GetBuildingsCnt() { return m_BuildingsCnt; }

    void SetTeam(int team) { m_Team = team; }
    int GetTeam() { return m_Team; }

    void FindNearObjects(CMatrixGroupObject *object);
    bool FindObject(CMatrixMapStatic *object);

    // void InstallTactics(TacticsType type, CBlockPar* par);
    // void DeInstallTactics();

    void LogicTakt(CMatrixSideUnit *side);
    void CalcGroupPosition();
    void CalcGroupSpeed();
    void SetGroupId(int id) { m_Id = id; }
    int GetGroupId() { return m_Id; }
    // CMatrixTactics* GetTactics()                                    { return m_Tactics; }
    D3DXVECTOR3 GetGroupPos() { return m_GroupPosition; }
    float GetGroupSpeed() { return m_GroupSpeed; }
    void SetGroupSpeed(float speed) { m_GroupSpeed = speed; }

    int GetBombersCnt();
    int GetRepairsCnt();

    CMatrixGroup();
    ~CMatrixGroup();
};

// class CMatrixGroupList : public CMain
//{
//    public:
//        int m_GroupsCnt;
//
//        int m_ReGroupPeriod;
//        CMatrixGroup*   m_FirstGroup;
//        CMatrixGroup*   m_LastGroup;
//
//    public:
//
//        int GetGroupsCnt()                                                  { return m_GroupsCnt; }
//
//        CMatrixGroup* AddNewGroup(int team, int id);
//        void RemoveGroup(CMatrixGroup* group);
//        void RemoveGroup(int team);
//
//        void ReGroup(CMatrixSideUnit* side);
//
//        //void AddRobotToTeam(Team team, CMatrixRobotAI* robot);
//        //void RemoveRobotFromTeam(Team team, CMatrixRobotAI* robot);
//
//        void AddObject(int team, int id, CMatrixMapStatic* object);
//        void RemoveObject(int team, int id, CMatrixMapStatic* object);
//
//        void LogicTakt(CMatrixSideUnit* side);
//
//        CMatrixGroupList()                                                  { m_GroupsCnt=0; m_FirstGroup=NULL;
//        m_LastGroup=NULL; m_ReGroupPeriod=0; } ~CMatrixGroupList();
//};
