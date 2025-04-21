#ifndef _AI_PROFILES_CUSTOM_H_
#define _AI_PROFILES_CUSTOM_H_

#include "opponent.h"
#include "harness/trace.h"

void ChooseNewObjective_Default(tOpponent_spec* pOpponent_spec, int pMust_choose_one);
void ChooseNewObjective_LapCompleter(tOpponent_spec* pOpponent_spec, int pMust_choose_one);
void ChooseNewObjective(tOpponent_spec* pOpponent_spec, int pMust_choose_one);

#endif