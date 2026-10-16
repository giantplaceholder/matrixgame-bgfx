// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include <cstdio>
#include <cmath>

#include <algorithm>

#include "MatrixCamera.hpp"
#include "MatrixMap.hpp"
#include "MatrixFlyer.hpp"
#include "MatrixRobot.hpp"
#include "MatrixObjectCannon.hpp"

#include "Effects/MatrixEffectBigBoom.hpp"

float MAX_VIEW_DISTANCE = 4000.0f;

void SetMaxCameraDistance(float perc) {
    MAX_VIEW_DISTANCE = 4000.0 * (1.0 + 0.01 * perc);
}

void SAutoFlyData::Release(void) {
    KillTrajectory();
    for (int i = 0; i < m_WarPairsCnt; ++i) {
        m_WarPairs[i].target->Release();
        m_WarPairs[i].attacker->Release();
    }
    m_WarPairsCnt = 0;

    if (m_WarPairCurrent.target) {
        m_WarPairCurrent.attacker->Release();
        m_WarPairCurrent.target->Release();

        m_WarPairCurrent.attacker = NULL;
        m_WarPairCurrent.target = NULL;
    }
}

void SAutoFlyData::Stat(void) {
    DTRACE();

    if ((g_MatrixMap->GetTime() - m_LastStatTime) < 60000)
        return;

    if (!g_MatrixMap->IsPaused()) {
        m_LastStatTime = g_MatrixMap->GetTime();
        SETFLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG_D);
    }
}

void SAutoFlyData::KillTrajectory(void) {
    DTRACE();
    // if (m_Traj)
}

void SAutoFlyData::AddWarPair(CMatrixMapStatic *tgt, CMatrixMapStatic *attacker) {
    DTRACE();

    SObjectCore *core0 = tgt->GetCore(DEBUG_CALL_INFO);
    SObjectCore *core1 = attacker->GetCore(DEBUG_CALL_INFO);

    for (int i = 0; i < m_WarPairsCnt;) {
        if (m_WarPairs[i].target == core0 && m_WarPairs[i].attacker == core1) {
            core0->Release();
            core1->Release();
            m_WarPairs[i].ttl = WAR_PAIR_TTL;
            return;
        }
        if (m_WarPairs[i].target->m_Object == NULL || m_WarPairs[i].attacker->m_Object == NULL) {
            m_WarPairs[i].target->Release();
            m_WarPairs[i].attacker->Release();

            m_WarPairs[i] = m_WarPairs[--m_WarPairsCnt];
            continue;
        }
        ++i;
    }

    if (m_WarPairsCnt < MAX_WAR_PAIRS) {
        m_WarPairs[m_WarPairsCnt].target = core0;
        m_WarPairs[m_WarPairsCnt].attacker = core1;
        m_WarPairs[m_WarPairsCnt].ttl = WAR_PAIR_TTL;
        ++m_WarPairsCnt;
        return;
    }

    core0->Release();
    core1->Release();
}

// void SAutoFlyData::BuildTrajectory(ETrajectoryStatus status)

// D3DXVECTOR3 to[128];

// int curi_t = 0;

// if (m_Traj == NULL)
// {

//    to[0] = m_Cur;

//    // slowly start

// }

// if (status == TRJS_MOVING_TO)
// {

//    //CHelper::DestroyByGroup(8080);
//    //CHelper::Create(1000,8080)->Cone(m_New, m_New + D3DXVECTOR3(0,0,100), 30,10,0xFFFFFFFF,0xFF00FF00,10);

//    // calc to

//    }

//    for (int idx = 1; idx < 3;++idx)

//        //CHelper::Create(10000,9090)->Cone(p, p + D3DXVECTOR3(0,0,90), 30,35,0xFFFFFFFF,0xFFFF0000, 10);

//    }

//    to[curi_t++] = m_New;

//    if (m_Traj == NULL)

//    }

//    m_OnTrajectorySpeedNeed = 0.05f * m_Traj->CalcLength() / 1000.0f;
//    if (m_Traj->CalcLength() > 500) m_OnTrajectorySpeedNeed *= 2;

// }

// CHelper::DestroyByGroup(123);

// }

static bool SeekCamObjects(
    [[maybe_unused]] const D3DXVECTOR2 &center,
    CMatrixMapStatic *ms,
    uintptr_t user)
{
    DTRACE();

    D3DXVECTOR3 *vecs = (D3DXVECTOR3 *)user;

    D3DXVECTOR3 dir(ms->GetGeoCenter() - vecs[0]);
    D3DXVec3Normalize(&dir, &dir);

    if (ms->IsCannon()) {
        vecs[1] += dir * (1.1f - ms->AsCannon()->GetHitPoint() * ms->AsCannon()->GetMaxHitPointInversed());
    }
    else if (ms->IsRobot()) {
        vecs[1] += dir * (1.1f - ms->AsRobot()->GetHitPoint() / ms->AsRobot()->GetMaxHitPoint());
    }

    return true;
}

void SAutoFlyData::Takt(float ms) {
    DTRACE();

    for (int i = 0; i < m_WarPairsCnt;) {
        m_WarPairs[i].ttl -= ms;
        if (m_WarPairs[i].ttl < 0 || m_WarPairs[i].target->m_Object == NULL ||
            m_WarPairs[i].attacker->m_Object == NULL) {
            m_WarPairs[i].target->Release();
            m_WarPairs[i].attacker->Release();

            m_WarPairs[i] = m_WarPairs[--m_WarPairsCnt];
            continue;
        }
        ++i;
    }

    m_ObzorTime -= ms;
    if (m_ObzorTime <= 0) {
        m_ObzorTime = 0;
        FindAutoFlyTarget();
    }

    m_CalcDestAngZTime -= ms;
    if (m_CalcDestAngZTime < 0) {
        m_CalcDestAngZTime += RECALC_DEST_ANGZ_PERIOD;
        if (m_CalcDestAngZTime < 0)
            m_CalcDestAngZTime = RECALC_DEST_ANGZ_PERIOD;

        D3DXVECTOR3 vecs[2];
        vecs[0] = m_Cur;
        vecs[1] = D3DXVECTOR3(0, 0, 0);

        g_MatrixMap->FindObjects(*(D3DXVECTOR2 *)&m_Cur, 700, 1, TRACE_ROBOT | TRACE_CANNON, NULL, SeekCamObjects,
                                 (uintptr_t)&vecs);

        float dl = D3DXVec3LengthSq(vecs + 1);
        if (dl > 0.00001f) {
            // vecs[1] *= INVERT(dl);

            m_NewAngZ = float(atan2(-vecs[1].x, vecs[1].y)) + M_PI_MUL(1);
        }
    }

    // if (m_Traj->NeedContinue())

    // float ds = (m_OnTrajectorySpeedNeed-m_OnTrajectorySpeedCurrent);

    float mul = float(1.0f - pow(0.999f, ms));
    // m_OnTrajectorySpeedCurrent += ds * mul;

    // m_Traj->Move(ms * m_OnTrajectorySpeedCurrent);

    D3DXVECTOR3 tovec(m_New - m_Cur);
    float len = D3DXVec3Length(&tovec);

    m_Speed += ms * 0.1f;
    if (m_Speed > len)
        m_Speed = len;

    if (m_Speed > 400)
        m_Speed = 400;

    float ilen = 0;
    if (len > 0.0001f)
        ilen = INVERT(len);

    m_Cur += tovec * (ilen * m_Speed * mul);

    // if (len > 0.0001f)

    //    if (D3DXVec3LengthSq(&m_Speed) > 0.1f)

    // m_Cur += m_Speed * ms;

    // if (m_ObzorTime > 0)

    //    float aac = m_NewAngZ;

    //    D3DXVECTOR3 pos,dir(g_MatrixMap->m_Camera.GetDir());

    //    pos = g_MatrixMap->m_Camera.GetFrustumCenter() + dir * 500;

    //    for (int i=0;i<20;++i)

    //        pos = g_MatrixMap->m_Camera.GetFrustumCenter() + dir * 500;
    //        g_MatrixMap->Trace(&pos, g_MatrixMap->m_Camera.GetFrustumCenter(), pos, TRACE_LANDSCAPE, NULL);

    //        float dist2 = D3DXVec3LengthSq(&(pos-g_MatrixMap->m_Camera.GetFrustumCenter()));

    //        if (dist2 > dist)

    //    m_NewAngZ = aac;

    //    float mul = float(1.0f - pow(0.9999f, ms));

    // }

    // //float mul = (float)(1.0 - pow(0.9996, double(ms)));
    static float prev_da = 0;
    float da = (float)AngleDist(m_CurAngZ, m_NewAngZ);

    static float last_da = 0;
    if (m_ObzorTime > 0) {
        CMatrixMapStatic *ts = g_MatrixMap->Trace(
                NULL, g_MatrixMap->m_Camera.GetFrustumCenter(),
                g_MatrixMap->m_Camera.GetFrustumCenter() + g_MatrixMap->m_Camera.GetDir() * 200, TRACE_LANDSCAPE, NULL);
        if (ts != TRACE_STOP_NONE)
            m_ObzorTime = 0;

        da = last_da;
        last_da *= (float)pow(0.9995f, ms);
    }
    else {
        last_da = da;
    }

    da *= (1.0f - fabs(prev_da - da) / M_PI_MUL(2));
    prev_da = da;
    // if (da > 1) da = 1;

    // m_RotSpeedZ += da * ms * 0.001f;

    m_CurAngZ += da * mul;

    if (m_ObzorTime > 0) {
        float k = (1.0f - m_ObzorTime * INVERT(OBZOR_TIME)) * 2;
        if (k > 1)
            k = 1;
        m_CurAngX = LERPFLOAT(k, m_BeforeObzorAngX, m_NewAngX);
        m_RotSpeedX = 0;

        m_Cur.z += ms * 0.0001f;
    }
    else {
        da = (float)AngleDist(m_CurAngX, m_NewAngX);
        if (da > 1)
            da = 1;
        if (da < -1)
            da = -1;

        m_RotSpeedX += da * ms * 0.001f;
        if (m_RotSpeedX > 0.1f)
            m_RotSpeedX = 0.1f;
        if (m_RotSpeedX < -0.1f)
            m_RotSpeedX = -0.1f;

        m_CurAngX += m_RotSpeedX * ms * 0.001f;

        float z = g_MatrixMap->GetZ(m_Cur.x + TableSin(-m_CurAngZ) * m_CurDist,
                                    m_Cur.y + TableCos(-m_CurAngZ) * m_CurDist);

        m_NewAngX = GRAD2RAD(50) - (z * 0.002f) * GRAD2RAD(70);
        if (m_NewAngX < GRAD2RAD(10))
            m_NewAngX = GRAD2RAD(10);
    }

    // if (m_Traj->NeedContinue())

    //    KillTrajectory();

    // m_Traj->GetCurPos(m_Cur);

    // CHelper::Create(1,0)->Sphere(m_Cur, 10, 10, 0xFFFFFFFF);

    CMatrixCamera *cam = &g_MatrixMap->m_Camera;

    cam->m_LinkPoint = m_Cur;

    cam->m_Dist = m_CurDist;
    cam->m_AngleZ = m_CurAngZ;

    cam->m_AngleX = m_CurAngX;

    // ///////////////////////////////////////////////////
    // D3DXVECTOR3 dir(m_CurTO - m_CurFROM);

    // cam->m_Dist = D3DXVec3Length(&dir);
    // cam->m_AngleZ = float(atan2(-dir.x,dir.y)) + M_PI_MUL(1);

    // float d2 = D3DXVec2Length((D3DXVECTOR2 *)&dir);

    // cam->m_AngleX = float(atan2(d2, -dir.z));
}

void SAutoFlyData::FindAutoFlyTarget(void) {
    DTRACE();

    if (FLAG(g_MatrixMap->m_Flags, MMFLAG_DIALOG_MODE)) {
        CMatrixCamera *cam = &g_MatrixMap->m_Camera;
        if (!FLAG(cam->m_Flags, CAM_NEED2END_DIALOG_MODE)) {
            SETFLAG(cam->m_Flags, CAM_NEED2END_DIALOG_MODE);

            CMatrixEffect *ef = g_MatrixMap->m_EffectsFirst;
            for (; ef; ef = ef->m_Next) {
                if (ef->GetType() == EFFECT_BIG_BOOM) {
                    m_New = ((CMatrixEffectBigBoom *)ef)->GetPos();
                    return;
                }
            }
        }

        auto tmp = m_Cur - m_New;
        if (POW2(100) > D3DXVec3LengthSq(&tmp))
            g_MatrixMap->LeaveDialogMode();
        else
            return;
    }

    CMatrixCamera *cam = &g_MatrixMap->m_Camera;
    if (m_WarPairsCnt == 0 && m_WarPairCurrent.target == NULL) {
    seek_nothing:

        if (!FLAG(cam->m_Flags, CAM_SELECTED_TARGET)) {
            if (((g_MatrixMap->GetTime() - m_LastObzorTime) > 15000) && FRND(1) < 0.8f) {
                m_ObzorTime = OBZOR_TIME;
                m_New = m_Cur;
                m_NewAngX = GRAD2RAD(70);
                m_BeforeObzorAngX = m_CurAngX;
                m_LastObzorTime = g_MatrixMap->GetTime();
                return;
            }

            SETFLAG(cam->m_Flags, CAM_SELECTED_TARGET);

            int cnt = 0;
            CMatrixMapStatic *ms = CMatrixMapStatic::GetFirstLogic();
            for (; ms; ms = ms->GetNextLogic())
                ++cnt;
            cnt = IRND(cnt);
            ms = CMatrixMapStatic::GetFirstLogic();
            for (; ms && cnt > 0; ms = ms->GetNextLogic(), --cnt)
                ;
            ASSERT(ms);
            m_New = ms->GetGeoCenter();

            m_NewDist = m_CurDist;
            m_NewAngZ = m_CurAngZ;
            m_NewAngX = m_CurAngX;

            // float a = FSRND(M_PI);
            // m_NewFROM = m_NewTO + D3DXVECTOR3(TableSin(a) * 100, TableCos(a) * 100, FSRND(100) + 100);
        }
        else {
            auto tmp = m_Cur - m_New;
            if (POW2(10) > D3DXVec3LengthSq(&tmp)) {
                RESETFLAG(cam->m_Flags, CAM_SELECTED_TARGET);
            }
        }
    }
    else {
        RESETFLAG(cam->m_Flags, CAM_SELECTED_TARGET);

        if (m_WarPairCurrent.target) {
        check_again:
            if (m_WarPairCurrent.target->m_Object && m_WarPairCurrent.attacker->m_Object) {
                m_New = (m_WarPairCurrent.target->m_Object->GetGeoCenter() +
                         m_WarPairCurrent.attacker->m_Object->GetGeoCenter()) *
                        0.5f;
                return;
            }

            SObjectCore *seek_for = NULL;
            if (m_WarPairCurrent.target->m_Object)
                seek_for = m_WarPairCurrent.target;
            else if (m_WarPairCurrent.attacker->m_Object)
                seek_for = m_WarPairCurrent.attacker;

            if (seek_for) {
                for (int i = 0; i < m_WarPairsCnt; ++i) {
                    if (m_WarPairs[i].attacker == seek_for || m_WarPairs[i].target == seek_for) {
                        m_WarPairCurrent.attacker->Release();
                        m_WarPairCurrent.target->Release();
                        m_WarPairCurrent = m_WarPairs[i];
                        m_WarPairs[i] = m_WarPairs[--m_WarPairsCnt];
                        goto check_again;
                    }
                }
            }

            m_WarPairCurrent.attacker->Release();
            m_WarPairCurrent.target->Release();

            m_WarPairCurrent.attacker = NULL;
            m_WarPairCurrent.target = NULL;
        }

        if (((g_MatrixMap->GetTime() - m_LastObzorTime) > 15000) && FRND(1) < 0.8f) {
            m_ObzorTime = OBZOR_TIME;
            m_New = m_Cur;
            m_NewAngX = GRAD2RAD(70);
            m_BeforeObzorAngX = m_CurAngX;
            m_LastObzorTime = g_MatrixMap->GetTime();

            return;
        }

        if (m_WarPairsCnt == 0)
            goto seek_nothing;

        int idx = -1;
        float dist = 1E30f;
        for (int i = 0; i < m_WarPairsCnt; ++i) {
            CMatrixMapStatic *ms = m_WarPairs[i].target->m_Object;

            auto tmp = ms->GetGeoCenter() - m_Cur;
            float d2 = D3DXVec3LengthSq(&tmp);

            if (d2 < dist) {
                dist = d2;
                idx = i;
            }
        }

        m_WarPairCurrent = m_WarPairs[idx];
        m_WarPairs[idx] = m_WarPairs[--m_WarPairsCnt];
        goto check_again;

        // D3DXVECTOR3 delta = m_WarPairs[idx].target->m_GeoCenter - m_WarPairs[idx].attacker->m_GeoCenter;

        // m_New = m_WarPairs[idx].attacker->m_GeoCenter + delta * 0.5f;

        // float mm = 1;

        // m_NewPriority = Float2Int((1-mm) * 10000 + 100);
    }
}

// =======================================================================

CMatrixCamera::CMatrixCamera(void) {
    DTRACE();

    m_ModeIndex = CAMERA_STRATEGY;

    m_XY_Strategy = D3DXVECTOR2(200, 200);
    m_Ang_Strategy = g_Config.m_CamBaseAngleZ;

    m_LinkPoint = D3DXVECTOR3(200, 200, 140);
    m_AngleZ = g_Config.m_CamBaseAngleZ;

    for (int i = 0; i < CAMERA_PARAM_CNT; ++i) {
        m_AngleParam[i] = g_Config.m_CamParams[i].m_CamAngleParam;
        m_DistParam[i] = 1.0f;
    }

    m_AngleX = LerpAng();
    m_Dist = LerpDist();

    _res_x_inversed = (float)(1.0 / (double)g_ScreenX);
    _res_y_inversed = (float)(1.0 / (double)g_ScreenY);

    // recalculate projective matrix
    D3DXMatrixPerspectiveFovLH(&m_MatProj, CAM_HFOV, float(g_ScreenX) * _res_y_inversed, 1.0f, MAX_VIEW_DISTANCE);

    _mp_11_inversed = (float)(1.0 / m_MatProj._11);
    _mp_22_inversed = (float)(1.0 / m_MatProj._22);

    m_Flags = 0;

    ZeroMemory(&m_ViewPort, sizeof(D3DVIEWPORT9));

    m_ViewPort.X = 0;
    m_ViewPort.Y = 0;
    m_ViewPort.Width = g_ScreenX;
    m_ViewPort.Height = g_ScreenY;

    m_ViewPort.MinZ = 0.0f;
    m_ViewPort.MaxZ = 1.0f;

    m_AFD = NULL;
}

void CMatrixCamera::ResetAngles(void) {
    m_Ang_Strategy = g_Config.m_CamBaseAngleZ + g_MatrixMap->m_CameraAngle;
    for (int i = 0; i < CAMERA_PARAM_CNT; ++i) {
        m_AngleParam[i] = g_Config.m_CamParams[i].m_CamAngleParam;
    }
    m_AngleX = LerpAng();
    m_Dist = LerpDist();
    if (!g_MatrixMap->GetPlayerSide()->IsArcadeMode())
        m_AngleZ = g_Config.m_CamBaseAngleZ + g_MatrixMap->m_CameraAngle;
}

CMatrixCamera::~CMatrixCamera(void) {
    if (m_AFD) {
        m_AFD->Release();
        HFree(m_AFD, g_MatrixHeap);
    }
}

void CMatrixCamera::CalcSkyMatrix(D3DXMATRIX &m) {
    D3DXMATRIX mz, mx;
    D3DXMatrixRotationY(&mz, -m_AngleZ + g_MatrixMap->m_SkyAngle);
    D3DXMatrixRotationX(&mx, m_AngleX - M_PI_MUL(0.5));
    m = mz * mx;
    // m = mx;
}

void CMatrixCamera::BeforeDraw(void) {
    DTRACE();

    D3DXMATRIX mt, mz, mx;

    // D3DXMatrixTranslation(&mt,-(m_Target.x+m_TargetDisp.x),-(m_Target.y+m_TargetDisp.y),-(m_Target.z+m_TargetDisp.z));

    {
        D3DXMatrixTranslation(&mt, -(m_LinkPoint.x), -(m_LinkPoint.y), -(m_LinkPoint.z));
        D3DXMatrixRotationZ(&mz, -m_AngleZ);
        D3DXMatrixRotationX(&mx, m_AngleX);
        mx._43 = -m_Dist;
        m_MatView = mt * mz * mx;
        NEG_FLOAT(m_MatView._12);
        NEG_FLOAT(m_MatView._13);
        NEG_FLOAT(m_MatView._22);
        NEG_FLOAT(m_MatView._23);
        NEG_FLOAT(m_MatView._32);
        NEG_FLOAT(m_MatView._33);
        NEG_FLOAT(m_MatView._42);
        NEG_FLOAT(m_MatView._43);
        static int once = 0;
        if (once++ < 2) {
            std::printf("cam angX=%g angZ=%g dist=%g link=%g,%g,%g view43=%g\n",
                        m_AngleX, m_AngleZ, m_Dist, m_LinkPoint.x, m_LinkPoint.y, m_LinkPoint.z, m_MatView._43);
        }
    }

    D3DXMatrixInverse(&m_MatViewInversed, NULL, &m_MatView);

    float ex = m_MatViewInversed._41, ey = m_MatViewInversed._42, ez = m_MatViewInversed._43;
    if (!std::isfinite(ex) || !std::isfinite(ey) || !std::isfinite(ez)) { ex = ey = 0; ez = 200; }
    float lz = g_MatrixMap->GetZ(m_MatViewInversed._41, m_MatViewInversed._42) + 10.0f;
    if (ez < lz) {
        m_MatViewInversed._43 = lz;
        D3DXMatrixInverse(&m_MatView, NULL, &m_MatViewInversed);
    }

    // frustum update

    float x1 = _mp_11_inversed * m_MatViewInversed._11;
    float x2 = _mp_11_inversed * m_MatViewInversed._12;
    float x3 = _mp_11_inversed * m_MatViewInversed._13;

    float y1 = _mp_22_inversed * m_MatViewInversed._21;
    float y2 = _mp_22_inversed * m_MatViewInversed._22;
    float y3 = _mp_22_inversed * m_MatViewInversed._23;

    m_FrustumDirLT.x = m_MatViewInversed._31 - x1 + y1;
    m_FrustumDirLT.y = m_MatViewInversed._32 - x2 + y2;
    m_FrustumDirLT.z = m_MatViewInversed._33 - x3 + y3;
    D3DXVec3Normalize(&m_FrustumDirLT, &m_FrustumDirLT);

    m_FrustumDirLB.x = m_MatViewInversed._31 - x1 - y1;
    m_FrustumDirLB.y = m_MatViewInversed._32 - x2 - y2;
    m_FrustumDirLB.z = m_MatViewInversed._33 - x3 - y3;
    D3DXVec3Normalize(&m_FrustumDirLB, &m_FrustumDirLB);

    m_FrustumDirRT.x = x1 + y1 + m_MatViewInversed._31;
    m_FrustumDirRT.y = x2 + y2 + m_MatViewInversed._32;
    m_FrustumDirRT.z = x3 + y3 + m_MatViewInversed._33;
    D3DXVec3Normalize(&m_FrustumDirRT, &m_FrustumDirRT);

    m_FrustumDirRB.x = x1 - y1 + m_MatViewInversed._31;
    m_FrustumDirRB.y = x2 - y2 + m_MatViewInversed._32;
    m_FrustumDirRB.z = x3 - y3 + m_MatViewInversed._33;
    D3DXVec3Normalize(&m_FrustumDirRB, &m_FrustumDirRB);

    m_FrustumCenter.x = m_MatViewInversed._41;
    m_FrustumCenter.y = m_MatViewInversed._42;
    m_FrustumCenter.z = m_MatViewInversed._43;

    auto tmp = D3DXVECTOR3(GetFrustumLB().y * GetFrustumRB().z - GetFrustumLB().z * GetFrustumRB().y,
                           GetFrustumLB().z * GetFrustumRB().x - GetFrustumLB().x * GetFrustumRB().z,
                           GetFrustumLB().x * GetFrustumRB().y - GetFrustumLB().y * GetFrustumRB().x);

    D3DXVec3Normalize(&m_FrustPlaneB.norm, &tmp);
    m_FrustPlaneB.dist = -D3DXVec3Dot(&m_FrustPlaneB.norm, &m_FrustumCenter);
    m_FrustPlaneB.UpdateSignBits();

    tmp = D3DXVECTOR3(GetFrustumLT().z * GetFrustumRT().y - GetFrustumLT().y * GetFrustumRT().z,
                      GetFrustumLT().x * GetFrustumRT().z - GetFrustumLT().z * GetFrustumRT().x,
                      GetFrustumLT().y * GetFrustumRT().x - GetFrustumLT().x * GetFrustumRT().y);

    D3DXVec3Normalize(&m_FrustPlaneT.norm, &tmp);
    m_FrustPlaneT.dist = -D3DXVec3Dot(&m_FrustPlaneT.norm, &m_FrustumCenter);
    m_FrustPlaneT.UpdateSignBits();

    tmp = D3DXVECTOR3(GetFrustumLB().z * GetFrustumLT().y - GetFrustumLB().y * GetFrustumLT().z,
                      GetFrustumLB().x * GetFrustumLT().z - GetFrustumLB().z * GetFrustumLT().x,
                      GetFrustumLB().y * GetFrustumLT().x - GetFrustumLB().x * GetFrustumLT().y);

    D3DXVec3Normalize(&m_FrustPlaneL.norm, &tmp);
    m_FrustPlaneL.dist = -D3DXVec3Dot(&m_FrustPlaneL.norm, &m_FrustumCenter);
    m_FrustPlaneL.UpdateSignBits();

    tmp = D3DXVECTOR3(GetFrustumRT().z * GetFrustumRB().y - GetFrustumRT().y * GetFrustumRB().z,
                      GetFrustumRT().x * GetFrustumRB().z - GetFrustumRT().z * GetFrustumRB().x,
                      GetFrustumRT().y * GetFrustumRB().x - GetFrustumRT().x * GetFrustumRB().y);

    D3DXVec3Normalize(&m_FrustPlaneR.norm, &tmp);
    m_FrustPlaneR.dist = -D3DXVec3Dot(&m_FrustPlaneR.norm, &m_FrustumCenter);
    m_FrustPlaneR.UpdateSignBits();

    // D3DXVECTOR3 dir(m_LinkPoint - m_FrustumCenter);

    // g_MatrixMap->m_DI.T(L"...1",std::wstring(m_AngleZ));
}

float CMatrixCamera::GetFrustPlaneDist(EFrustumPlane plane, const D3DXVECTOR3 &pos, const D3DXVECTOR3 &dir) {
    const SPlane *pl = nullptr;

    switch (plane) {
        case FPLANE_LEFT:
            pl = &GetFrustPlaneL();
            break;
        case FPLANE_RIGHT:
            pl = &GetFrustPlaneR();
            break;
        case FPLANE_BOTTOM:
            pl = &GetFrustPlaneB();
            break;
        case FPLANE_TOP:
            pl = &GetFrustPlaneT();
            break;
    }

    float t;
    bool f = pl->FindIntersect(pos, dir, t);
    if (f && t >= 0)
        return t;
    return 1E30f;
}

float CMatrixCamera::GetFrustPlaneMinDist(const D3DXVECTOR3 &pos) {
    float d0 = GetFrustPlaneL().Distance(pos);
    float d1 = GetFrustPlaneR().Distance(pos);
    float d2 = GetFrustPlaneT().Distance(pos);
    float d3 = GetFrustPlaneB().Distance(pos);
    return std::min(std::min(d0, d1), std::min(d2, d3));
}

// void  CMatrixCamera::ModeChanged(void)

void CMatrixCamera::CalcLinkPoint(D3DXVECTOR3 &lp, float &angz) {
    DTRACE();

    // RESETFLAG(m_Flags,CAM_LAST_TGTFROM_PRESENT);

    if (g_MatrixMap->GetPlayerSide()->IsArcadeMode()) {
        DCP();
        CMatrixMapStatic *arcade_object = g_MatrixMap->GetPlayerSide()->GetArcadedObject();
        if (arcade_object->IsRobot()) {
            DCP();
            CMatrixRobotAI *bot = arcade_object->AsRobot();

            lp.x = bot->m_PosX;
            lp.y = bot->m_PosY;
            lp.z = bot->Z_From_Pos() + g_Config.m_CamParams[CAMERA_INROBOT].m_CamHeight;

            float bot_speed = (bot->m_Speed / bot->GetMaxSpeed());
            lp += bot->m_Forward * LERPFLOAT(bot_speed, g_Config.m_CamInRobotForward0, g_Config.m_CamInRobotForward1);

            angz = (float)(atan2(-bot->m_Forward.x, bot->m_Forward.y) + M_PI_MUL(1));
        }
    }
    else {
        DCP();
        lp.x = m_XY_Strategy.x;
        lp.y = m_XY_Strategy.y;
        lp.z = g_Config.m_CamParams[CAMERA_STRATEGY].m_CamHeight +
               g_MatrixMap->GetZInterpolatedLand(m_XY_Strategy.x, m_XY_Strategy.y);

        angz = m_Ang_Strategy;
    }
}

void CMatrixCamera::Takt(float ms) {
    DTRACE();

    if (FLAG(g_MatrixMap->m_Flags, MMFLAG_FLYCAM))
    // if (g_MatrixMap->m_KeyDown && g_MatrixMap->m_KeyScan == KEY_F2)
    {
        // g_MatrixMap->m_KeyDown = false;

        if (m_AFD == NULL) {
            m_AFD = (SAutoFlyData *)HAllocClear(sizeof(SAutoFlyData), g_MatrixHeap);

            m_AFD->m_Cur = m_LinkPoint;
            m_AFD->m_CurDist = m_Dist;
            m_AFD->m_CurAngZ = m_AngleZ;
            m_AFD->m_CurAngX = m_AngleX;
        }

        m_AFD->Takt(ms);

        return;
    }
    else {
        if (m_AFD) {
            m_AFD->Release();
            HFree(m_AFD, g_MatrixHeap);
            m_AFD = NULL;
        }
    }

    int index;
    if (g_MatrixMap->GetPlayerSide()->IsArcadeMode()) {
        DCP();
        index = CAMERA_INROBOT;
    }
    else {
        DCP();
        index = CAMERA_STRATEGY;
    }

    DCP();

    if (index != m_ModeIndex) {
        DCP();
        // mode changed
        SETFLAG(m_Flags, CAM_LINK_POINT_CHANGED);
        RESETFLAG(m_Flags, CAM_XY_LERP_OFF);
        m_ModeIndex = index;

        if (index == CAMERA_STRATEGY) {
            DCP();
            float sn = float(sin(m_Ang_Strategy));
            float cs = float(cos(m_Ang_Strategy));

            m_XY_Strategy.x = m_LinkPoint.x - sn * 80;
            m_XY_Strategy.y = m_LinkPoint.y + cs * 80;
        }
    }

    D3DXVECTOR3 newlp;
    float newangz;
    float newdist = LerpDist();
    float newangx = LerpAng();

    DCP();
    CalcLinkPoint(newlp, newangz);
    DCP();

    // #ifdef _DEBUG

    if (FLAG(m_Flags, CAM_LINK_POINT_CHANGED)) {
        // move camera from current pos to new lp
        DCP();

        float mul = (float)(1.0 - pow(0.995, double(ms)));

        D3DXVECTOR3 dlp(newlp - m_LinkPoint);
        float daz = (float)AngleNorm(AngleDist(m_AngleZ, newangz));
        float dd = (newdist - m_Dist);
        float dax = (float)AngleDist(m_AngleX, newangx);

        m_LinkPoint += dlp * mul;
        m_AngleZ += daz * mul;

        m_Dist += dd * mul;
        m_AngleX += dax * mul;

        if (FLAG(m_Flags, CAM_XY_LERP_OFF)) {
            dlp.x = 0;
            dlp.y = 0;

            m_LinkPoint.x = newlp.x;
            m_LinkPoint.y = newlp.y;
        }

        if (m_ModeIndex == CAMERA_STRATEGY && (D3DXVec3LengthSq(&dlp) < 0.25f) && (daz < GRAD2RAD(0.5f)) &&
            (dax < GRAD2RAD(0.5f)) && (dd < 0.5f)) {
            RESETFLAG(m_Flags, CAM_LINK_POINT_CHANGED);
        }
    }
    else {
        // just copy values
        DCP();

        m_LinkPoint = newlp;
        m_AngleZ = newangz;

        m_Dist = newdist;
        m_AngleX = newangx;
    }

    if (FLAG(m_Flags, CAM_ACTION_MOVE_LEFT | CAM_ACTION_MOVE_RIGHT | CAM_ACTION_MOVE_UP | CAM_ACTION_MOVE_DOWN)) {
        DCP();

        if (m_ModeIndex == CAMERA_STRATEGY) {
            SETFLAG(m_Flags, CAM_XY_LERP_OFF);

            D3DXVECTOR2 dir(GetFrustPlaneB().norm.x, GetFrustPlaneB().norm.y);
            D3DXVec2Normalize(&dir, &dir);

            dir *= g_Config.m_CamMoveSpeed * ms;

            D3DXVECTOR2 lDir(dir.y, -dir.x);
            D3DXVECTOR2 rDir(-dir.y, dir.x);
            D3DXVECTOR2 tDir(dir.x, dir.y);
            D3DXVECTOR2 bDir(-dir.x, -dir.y);

            // D3DXVECTOR3 tgt(0,0,0);

            if (FLAG(m_Flags, CAM_ACTION_MOVE_LEFT)) {
                m_XY_Strategy += lDir;
            }
            if (FLAG(m_Flags, CAM_ACTION_MOVE_RIGHT)) {
                m_XY_Strategy += rDir;
            }
            if (FLAG(m_Flags, CAM_ACTION_MOVE_UP)) {
                m_XY_Strategy += tDir;
            }
            if (FLAG(m_Flags, CAM_ACTION_MOVE_DOWN)) {
                m_XY_Strategy += bDir;
            }

            D3DXVECTOR2 ccc(g_MatrixMap->m_Size.x * GLOBAL_SCALE / 2, g_MatrixMap->m_Size.y * GLOBAL_SCALE / 2);
            D3DXVECTOR2 bd(ccc - m_XY_Strategy);
            float r = D3DXVec2Length(&bd);
            float rlim = 3 * std::max(ccc.x, ccc.y);
            if (r > rlim) {
                m_XY_Strategy += bd * INVERT(r) * (r - rlim);
            }
        }
        else {
            RESETFLAG(m_Flags,
                      CAM_ACTION_MOVE_LEFT | CAM_ACTION_MOVE_RIGHT | CAM_ACTION_MOVE_UP | CAM_ACTION_MOVE_DOWN);
        }
    }

    if (FLAG(m_Flags, CAM_ACTION_ROT_LEFT)) {
        DCP();
        ASSERT(m_ModeIndex == CAMERA_STRATEGY);
        m_Ang_Strategy -= g_Config.m_CamParams[m_ModeIndex].m_CamRotSpeedZ * ms;
    }
    if (FLAG(m_Flags, CAM_ACTION_ROT_RIGHT)) {
        DCP();
        ASSERT(m_ModeIndex == CAMERA_STRATEGY);
        m_Ang_Strategy += g_Config.m_CamParams[m_ModeIndex].m_CamRotSpeedZ * ms;
    }

    if (FLAG(m_Flags, CAM_ACTION_ROT_UP)) {
        DCP();
        m_AngleParam[m_ModeIndex] += g_Config.m_CamParams[m_ModeIndex].m_CamRotSpeedX * ms;
        if (m_AngleParam[m_ModeIndex] > 1.0f)
            m_AngleParam[m_ModeIndex] = 1.0f;
    }
    if (FLAG(m_Flags, CAM_ACTION_ROT_DOWN)) {
        DCP();
        m_AngleParam[m_ModeIndex] -= g_Config.m_CamParams[m_ModeIndex].m_CamRotSpeedX * ms;
        if (m_AngleParam[m_ModeIndex] < 0.0f)
            m_AngleParam[m_ModeIndex] = 0.0f;
    }

    RESETFLAG(m_Flags, CAM_ACTION_ROT_LEFT | CAM_ACTION_ROT_RIGHT | CAM_ACTION_ROT_UP | CAM_ACTION_ROT_DOWN |
                               CAM_ACTION_MOVE_LEFT | CAM_ACTION_MOVE_RIGHT | CAM_ACTION_MOVE_UP |
                               CAM_ACTION_MOVE_DOWN);
}

void CMatrixCamera::RestoreCameraParams(void) {
    ASSERT_DX(g_D3DD->SetViewport(&m_ViewPort));
    // ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &OldMatWorld));
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_VIEW, &m_MatView));
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_PROJECTION, &m_MatProj));
}
