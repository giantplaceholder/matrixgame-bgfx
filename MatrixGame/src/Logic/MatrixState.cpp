// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "MatrixState.h"
#include "MatrixRule.h"
#include "../MatrixMap.hpp"
#include "MatrixLogicSlot.h"

// CMatrixState::~CMatrixState()
//{
//	DTRACE();
//	CMatrixRule *Rule = m_FirstRule;
//
//	while(Rule != NULL){
//		if(Rule->m_NextRule)
//			Rule = Rule->m_NextRule;
//		else {
//			HDelete(CMatrixRule, Rule, g_MatrixHeap);
//			Rule = NULL;
//			m_FirstRule = NULL;
//			m_LastRule = NULL;
//
//		}
//
//		if(Rule)
//			HDelete(CMatrixRule, Rule->m_PrevRule, g_MatrixHeap);
//	}
//}
//
// void CMatrixState::Load(CBlockPar &bp)
//{
//    CWStr filename;
//    //filename.Add(L"#ST_");
//    //filename.Add(GetName());
//    //filename.Add(L"_");
//    //filename.Add(((CMatrixLogicSlot*)GetParent())->GetName());
//    //filename.Add(L"_");
//    //filename.Add(((CMatrixTactics*)((CMatrixLogicSlot*)GetParent())->GetParent())->GetName());
//    //filename.Add(L".txt");
//    //bp.SaveInTextFile(filename);
//
//    //
//    int rules_cnt = bp.BlockCount();
//    for(int cnt = 0; cnt < rules_cnt; cnt++){
//        CMatrixRule *rule = HNew(g_MatrixHeap) CMatrixRule();
//        rule->SetName(bp.BlockGetName(cnt));
//        rule->SetParent(this);
//        rule->Load(*bp.BlockGet(cnt));
//        LIST_ADD(rule, m_FirstRule, m_LastRule, m_PrevRule, m_NextRule);
//    }
//}