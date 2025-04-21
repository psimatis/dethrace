// Header for opponent_ai.c
#ifndef OPPONENT_AI_H
#define OPPONENT_AI_H

#include "dr_types.h"
#include "opponent.h"
#include "dr_types.h"
#include <stdio.h>
#include <stdarg.h>
#include "globvars.h"
#include "utility.h"
#include "car.h"
#include "harness/trace.h"

void PrintObjectiveIfChanged(tOpponent_spec* pOpponent_spec);
void ProcessCurrentObjective(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void NewObjective(tOpponent_spec* pOpponent_spec, tOpponent_objective_type pObjective_type, ...);
void ProcessCompleteRace(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessPursueAndTwat(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessRunAway(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessWaitForSomeHaplessSod(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessReturnToStart(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessLevitate(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessGetNearPlayer(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ProcessFrozen(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand);
void ObjectiveComplete(tOpponent_spec* pOpponent_spec);
void ChooseNewObjective(tOpponent_spec* pOpponent_spec, int pMust_choose_one);
void ProcessThisOpponent(tOpponent_spec* pOpponent_spec);

#endif // OPPONENT_AI_H
