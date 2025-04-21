#include "ai_profiles_custom.h"
#include "opponent.h"
#include "dr_types.h"
#include "globvars.h"
#include "utility.h"

void ChooseNewObjective_Default(tOpponent_spec* pOpponent_spec, int pMust_choose_one) {
    char str[255];
    tS16 players_section;
    br_vector3 wank;
    br_vector3 player_to_oppo_v;
    br_vector3 section_v;
    br_vector3 intersect;
    br_scalar dot;
    br_scalar distance;
    int do_it;
    int i;
    int j;
    int pursuit_percentage;
    int percentage;
    int general_grudge_increase;
    LOG_TRACE("(%p, %d)", pOpponent_spec, pMust_choose_one);

    // v3 = pMust_choose_one;
    if (pOpponent_spec->current_objective == eOOT_knackered_and_freewheeling || pOpponent_spec->knackeredness_detected) {
        return;
    }
    if (gTime_stamp_for_this_munging > pOpponent_spec->next_out_of_world_check) {
        pOpponent_spec->next_out_of_world_check = gTime_stamp_for_this_munging + 500;
        if (HasCarFallenOffWorld(pOpponent_spec->car_spec)) {
            if (pOpponent_spec->car_spec->last_time_we_touched_a_player <= gTime_stamp_for_this_munging - 7000) {
                TeleportOpponentToNearestSafeLocation(pOpponent_spec);
                NewObjective(pOpponent_spec, eOOT_complete_race);
            } else {
                TurnOpponentPhysicsOff(pOpponent_spec);
                pOpponent_spec->finished_for_this_race = 1;
                KnackerThisCar(pOpponent_spec->car_spec);
                pOpponent_spec->car_spec->car_master_actor->t.t.mat.m[3][1] -= 1000.0f;
            }
            return;
        }
    }
    if (pOpponent_spec->car_spec->knackered && !pOpponent_spec->knackeredness_detected) {
        pOpponent_spec->knackeredness_detected = 1;
        dr_dprintf("%s: Knackered - dealing with appropriately", pOpponent_spec->car_spec->driver_name);
        if (pOpponent_spec->car_spec->has_been_stolen) {
            NewObjective(pOpponent_spec, eOOT_levitate);
        } else {
            NewObjective(pOpponent_spec, eOOT_knackered_and_freewheeling);
        }
        return;
    }
    if (pOpponent_spec->current_objective == eOOT_frozen) {
        if (CAR_SPEC_GET_SPEED_FACTOR(pOpponent_spec->car_spec) != 0.0f) {
            dr_dprintf("%s: Time to unfreeze", pOpponent_spec->car_spec->driver_name);
            if (pOpponent_spec->pursuing_player_before_freeze == 1) {
                NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
            } else {
                NewObjective(pOpponent_spec, eOOT_get_near_player);
            }
        }
        return;
    } else {
        if (CAR_SPEC_GET_SPEED_FACTOR(pOpponent_spec->car_spec) == 0.0f) {
            dr_dprintf("%s: Decided to freeze", pOpponent_spec->car_spec->driver_name);
            if (pOpponent_spec->current_objective == eOOT_pursue_and_twat && pOpponent_spec->pursue_car_data.pursuee == &gProgram_state.current_car) {
                pOpponent_spec->pursuing_player_before_freeze = 1;
            } else {
                pOpponent_spec->pursuing_player_before_freeze = 0;
            }
            NewObjective(pOpponent_spec, eOOT_frozen);
            return;
        }
        if (!gFirst_frame) {
            general_grudge_increase = (pOpponent_spec->nastiness * 40.0f + 10.0f);
            if (pOpponent_spec->car_spec->scary_bang && pOpponent_spec->player_to_oppo_d < 10.0f) {
                if (pOpponent_spec->current_objective == eOOT_pursue_and_twat) {
                    percentage = 40;
                } else {
                    percentage = 0;
                }
                if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
                    if (PercentageChance(20)) {
                        dr_dprintf("%s: Decided to run away", pOpponent_spec->car_spec->driver_name);
                        NewObjective(pOpponent_spec, eOOT_run_away);
                        return;
                    }
                } else if (PercentageChance((percentage + 60) - pOpponent_spec->nastiness * 50.0)) {
                    dr_dprintf("%s: Decided to run away", pOpponent_spec->car_spec->driver_name);
                    NewObjective(pOpponent_spec, eOOT_run_away);
                    return;
                }
            }
            if (!gMellow_opponents && (pOpponent_spec->current_objective != eOOT_run_away || pOpponent_spec->time_this_objective_started + 15000 <= gTime_stamp_for_this_munging)) {
                if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec) && pOpponent_spec->murder_reported && pOpponent_spec->player_to_oppo_d < 20.0f && !AlreadyPursuingCar(pOpponent_spec, &gProgram_state.current_car)) {
                    gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
                    sprintf(str, "%s: Furderous melon!", pOpponent_spec->car_spec->driver_name);
                    dr_dprintf("%s: Decided to pursue after MURDER", pOpponent_spec->car_spec->driver_name);
                    NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
                    return;
                }
                if (pOpponent_spec->car_spec->big_bang && LastTwatterAPlayer(pOpponent_spec) && !AlreadyPursuingCar(pOpponent_spec, pOpponent_spec->car_spec->last_person_to_hit_us)) {
                    // v4 = gOpponents[pOpponent_spec->index].psyche.grudge_against_player;
                    // if (v4 <= 20) {
                    //     v4 = 20;
                    // }
                    // v5 = general_grudge_increase + v4;
                    // if (v5 >= 100) {
                    //     LOBYTE(v5) = 100;
                    // }
                    gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
                    sprintf(str, "%s: Christ! What was that?", pOpponent_spec->car_spec->driver_name);
                    dr_dprintf("%s: Decided to pursue after big bang; last person to twat us was %s", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_to_hit_us->driver_name);
                    NewObjective(pOpponent_spec, eOOT_pursue_and_twat, pOpponent_spec->car_spec->last_person_to_hit_us);
                    return;
                }
                if (LastTwatteeAPlayer(pOpponent_spec) && !AlreadyPursuingCar(pOpponent_spec, pOpponent_spec->car_spec->last_person_we_hit)) {
                    // v6 = gOpponents[pOpponent_spec->index].psyche.grudge_against_player;
                    // if (v6 <= 20) {
                    //     v6 = 20;
                    // }
                    // v7 = general_grudge_increase + v6;
                    // if (v7 >= 100) {
                    //     LOBYTE(v7) = 100;
                    // }
                    gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
                    sprintf(str, "%s: Ha! Bet you weren't expecting that!", pOpponent_spec->car_spec->driver_name);
                    dr_dprintf("%s: Decided to pursue %s after accidentally hitting them", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_we_hit->driver_name);
                    NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
                    return;
                }
                if (!AlreadyPursuingCar(pOpponent_spec, &gProgram_state.current_car)) {
                    if (pOpponent_spec->car_spec->grudge_raised_recently && (!CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec) || pOpponent_spec->player_to_oppo_d <= 20.0) && LastTwatterAPlayer(pOpponent_spec) && gOpponents[pOpponent_spec->index].psyche.grudge_against_player > 20) {
                        // v8 = gOpponents[pOpponent_spec->index].psyche.grudge_against_player;
                        // if (v8 <= 20) {
                        //     v8 = 20;
                        // }
                        // v9 = general_grudge_increase + v8;
                        // if (v9 >= 100) {
                        //     LOBYTE(v9) = 100;
                        // }
                        gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
                        sprintf(str, "%s: Right! That's enough, %s!", pOpponent_spec->car_spec->driver_name, gProgram_state.current_car.driver_name);
                        dr_dprintf("%s: Decided to pursue after grudginess raised; last person to twat us was %s", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_to_hit_us->driver_name);
                        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
                        return;
                    }

                    if ((pOpponent_spec->player_in_view_now) != 0 && (pOpponent_spec->acknowledged_piv) == 0) {
                        pOpponent_spec->acknowledged_piv = 1;
                        if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
                            pursuit_percentage = (BrVector3Length(&gProgram_state.current_car.v) - gDefinite_no_cop_pursuit_speed) * gCop_pursuit_speed_percentage_multiplier;
                        } else if (gProgram_state.skill_level + 3 > gNum_of_opponents_pursuing) {
                            pursuit_percentage = gOpponents[pOpponent_spec->index].psyche.grudge_against_player - 20 + pOpponent_spec->nastiness * 30.f;
                        } else {
                            pursuit_percentage = 0;
                        }

                        pursuit_percentage += 50 * HeadOnWithPlayerPossible(pOpponent_spec);
                        do_it = PercentageChance(pursuit_percentage);
                        dr_dprintf("%s: Spotted player; chance of pursuing %d%%: %s", pOpponent_spec->car_spec->driver_name, pursuit_percentage, do_it ? "YES, Decided to pursue" : "NO, Decided NOT to pursue");
                        if (do_it) {
                            gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
                            sprintf(str, "%s: I've decided to kill you for the fun of it", pOpponent_spec->car_spec->driver_name);
                            NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
                            return;
                        }
                    }
                }
            }
        }
        if (!pMust_choose_one) {
            return;
        }
        dr_dprintf("%s: Choosing new objective because we have to...", pOpponent_spec->car_spec->driver_name);
        if (pOpponent_spec->has_moved_at_some_point) {
            if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
                NewObjective(pOpponent_spec, eOOT_return_to_start);
                return;
            }
            if (gNum_of_opponents_pursuing + gNum_of_opponents_getting_near >= 3 || pOpponent_spec->player_to_oppo_d <= 10.0) {
                if (gNum_of_opponents_completing_race >= 2) {
                    pursuit_percentage = pOpponent_spec->player_to_oppo_d - 15.0f;
                    if (PercentageChance(pursuit_percentage)) {
                        dr_dprintf("%s: Choosing to get_near because chance dictated it (%d%%)", pOpponent_spec->car_spec->driver_name, pursuit_percentage);
                        NewObjective(pOpponent_spec, eOOT_get_near_player);
                        return;
                    } else {
                        dr_dprintf("%s: Choosing to complete_race because chance dictated it (%d%%)", pOpponent_spec->car_spec->driver_name, pursuit_percentage);
                    }
                } else {
                    dr_dprintf("%s: Choosing to complete_race because not enough oppos are yet (%d/%d)", pOpponent_spec->car_spec->driver_name, gNum_of_opponents_completing_race, 2);
                }
                NewObjective(pOpponent_spec, eOOT_complete_race);
                return;
            }
            dr_dprintf("%s: Choosing to get_near because not enough oppos are yet (%d/%d)", pOpponent_spec->car_spec->driver_name, gNum_of_opponents_pursuing + gNum_of_opponents_getting_near, 3);
            NewObjective(pOpponent_spec, eOOT_get_near_player);
            return;
        }
        if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
            NewObjective(pOpponent_spec, eOOT_wait_for_some_hapless_sod);
            return;
        }
        if (!pOpponent_spec->pursue_from_start || gMellow_opponents) {
            NewObjective(pOpponent_spec, eOOT_complete_race);
            return;
        }

        gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, pOpponent_spec->nastiness * 40.0 + (MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + 20));
        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
    }
}


void ChooseNewObjective_LapCompleter(tOpponent_spec* pOpponent_spec, int pMust_choose_one) {
    char str[255];
    LOG_TRACE("(%p, %d)", pOpponent_spec, pMust_choose_one);

    // Handle knackered
    if (pOpponent_spec->current_objective == eOOT_knackered_and_freewheeling || pOpponent_spec->knackeredness_detected)
	return;

    // Handle out of bounds
    if (gTime_stamp_for_this_munging > pOpponent_spec->next_out_of_world_check) {
	pOpponent_spec->next_out_of_world_check = gTime_stamp_for_this_munging + 500;
	if (HasCarFallenOffWorld(pOpponent_spec->car_spec)) {
	    if (pOpponent_spec->car_spec->last_time_we_touched_a_player <= gTime_stamp_for_this_munging - 7000) {
	        TeleportOpponentToNearestSafeLocation(pOpponent_spec);
	        NewObjective(pOpponent_spec, eOOT_complete_race);
	    } else {
	        TurnOpponentPhysicsOff(pOpponent_spec);
	        pOpponent_spec->finished_for_this_race = 1;
	        KnackerThisCar(pOpponent_spec->car_spec);
	        pOpponent_spec->car_spec->car_master_actor->t.t.mat.m[3][1] -= 1000.0f;
	    }
	    return;
	}
    }

    // Handle newly knackered
    if (pOpponent_spec->car_spec->knackered && !pOpponent_spec->knackeredness_detected) {
	pOpponent_spec->knackeredness_detected = 1;
	dr_dprintf("%s: Knackered - dealing with appropriately", pOpponent_spec->car_spec->driver_name);
	if (pOpponent_spec->car_spec->has_been_stolen)
	    NewObjective(pOpponent_spec, eOOT_levitate);
	else
	    NewObjective(pOpponent_spec, eOOT_knackered_and_freewheeling);
	return;
    }

    // Handle frozen
    if (pOpponent_spec->current_objective == eOOT_frozen) {
	if (CAR_SPEC_GET_SPEED_FACTOR(pOpponent_spec->car_spec) != 0.0f) {
	    dr_dprintf("%s: Time to unfreeze", pOpponent_spec->car_spec->driver_name);
	    if (pOpponent_spec->pursuing_player_before_freeze == 1)
	        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
	    else
	        NewObjective(pOpponent_spec, eOOT_get_near_player);
	}
	return;
    }

    // if not moving, switch to frozen, if it was pursuing before freezing it resumes.
    int pursuit_percentage;
    if (CAR_SPEC_GET_SPEED_FACTOR(pOpponent_spec->car_spec) == 0.0f) {
	dr_dprintf("%s: Decided to freeze", pOpponent_spec->car_spec->driver_name);
	if (pOpponent_spec->current_objective == eOOT_pursue_and_twat && pOpponent_spec->pursue_car_data.pursuee == &gProgram_state.current_car) {
	    pOpponent_spec->pursuing_player_before_freeze = 1;
	} else
	    pOpponent_spec->pursuing_player_before_freeze = 0;
	NewObjective(pOpponent_spec, eOOT_frozen);
	return;
    }

    //  Handles decision to run away,pursue or ignore, based on factors like proximity, aggression, and recent events.
    if (!gFirst_frame) {
	int general_grudge_increase = (pOpponent_spec->nastiness * 40.0f + 10.0f);
	if (pOpponent_spec->car_spec->scary_bang && pOpponent_spec->player_to_oppo_d < 10.0f) {
	    int percentage;
	    if (pOpponent_spec->current_objective == eOOT_pursue_and_twat)
	        percentage = 40;
	    else
	        percentage = 0;
	    if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
	        if (PercentageChance(20)) {
	            dr_dprintf("%s: Decided to run away", pOpponent_spec->car_spec->driver_name);
	            NewObjective(pOpponent_spec, eOOT_run_away);
	            return;
	        }
	    } else if (PercentageChance((percentage + 60) - pOpponent_spec->nastiness * 50.0)) {
	        dr_dprintf("%s: Decided to run away", pOpponent_spec->car_spec->driver_name);
	        NewObjective(pOpponent_spec, eOOT_run_away);
	        return;
	    }
	}
	if (!gMellow_opponents && (pOpponent_spec->current_objective != eOOT_run_away || pOpponent_spec->time_this_objective_started + 15000 <= gTime_stamp_for_this_munging)) {
	    if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec) && pOpponent_spec->murder_reported && pOpponent_spec->player_to_oppo_d < 20.0f && !AlreadyPursuingCar(pOpponent_spec, &gProgram_state.current_car)) {
	        gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
	        sprintf(str, "%s: Furderous melon!", pOpponent_spec->car_spec->driver_name);
	        dr_dprintf("%s: Decided to pursue after MURDER", pOpponent_spec->car_spec->driver_name);
	        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
	        return;
	    }
	    if (pOpponent_spec->car_spec->big_bang && LastTwatterAPlayer(pOpponent_spec) && !AlreadyPursuingCar(pOpponent_spec, pOpponent_spec->car_spec->last_person_to_hit_us)) {
	        gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
	        sprintf(str, "%s: Christ! What was that?", pOpponent_spec->car_spec->driver_name);
	        dr_dprintf("%s: Decided to pursue after big bang; last person to twat us was %s", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_to_hit_us->driver_name);
	        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, pOpponent_spec->car_spec->last_person_to_hit_us);
	        return;
	    }
	    if (LastTwatteeAPlayer(pOpponent_spec) && !AlreadyPursuingCar(pOpponent_spec, pOpponent_spec->car_spec->last_person_we_hit)) {
	        gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
	        sprintf(str, "%s: Ha! Bet you weren't expecting that!", pOpponent_spec->car_spec->driver_name);
	        dr_dprintf("%s: Decided to pursue %s after accidentally hitting them", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_we_hit->driver_name);
	        NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
	        return;
	    }
	    if (!AlreadyPursuingCar(pOpponent_spec, &gProgram_state.current_car)) {
	        if (pOpponent_spec->car_spec->grudge_raised_recently && (!CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec) || pOpponent_spec->player_to_oppo_d <= 20.0) && LastTwatterAPlayer(pOpponent_spec) && gOpponents[pOpponent_spec->index].psyche.grudge_against_player > 20) {
	            gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
	            sprintf(str, "%s: Right! That's enough, %s!", pOpponent_spec->car_spec->driver_name, gProgram_state.current_car.driver_name);
	            dr_dprintf("%s: Decided to pursue after grudginess raised; last person to twat us was %s", pOpponent_spec->car_spec->driver_name, pOpponent_spec->car_spec->last_person_to_hit_us->driver_name);
	            NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
	            return;
	        }

	        if ((pOpponent_spec->player_in_view_now) != 0 && (pOpponent_spec->acknowledged_piv) == 0) {
	            pOpponent_spec->acknowledged_piv = 1;
	            if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec))
	                pursuit_percentage = (BrVector3Length(&gProgram_state.current_car.v) - gDefinite_no_cop_pursuit_speed) * gCop_pursuit_speed_percentage_multiplier;
	            else if (gProgram_state.skill_level + 3 > gNum_of_opponents_pursuing)
	                pursuit_percentage = gOpponents[pOpponent_spec->index].psyche.grudge_against_player - 20 + pOpponent_spec->nastiness * 30.f;
	            else
	                pursuit_percentage = 0;

	            pursuit_percentage += 50 * HeadOnWithPlayerPossible(pOpponent_spec);
	            int do_it = PercentageChance(pursuit_percentage);
	            dr_dprintf("%s: Spotted player; chance of pursuing %d%%: %s", pOpponent_spec->car_spec->driver_name, pursuit_percentage, do_it ? "YES, Decided to pursue" : "NO, Decided NOT to pursue");
	            if (do_it) {
	                gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + general_grudge_increase);
	                sprintf(str, "%s: I've decided to kill you for the fun of it", pOpponent_spec->car_spec->driver_name);
	                NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
	                return;
	            }
	        }
	    }
	}
    }

    if (!pMust_choose_one)
	return;

    dr_dprintf("%s: Choosing new objective because we have to...", pOpponent_spec->car_spec->driver_name);

    //  Pick next objective (cop or not), depending on #opponents pursuing/near, #completed the race, #distance.
    if (pOpponent_spec->has_moved_at_some_point) {
	if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
	    NewObjective(pOpponent_spec, eOOT_return_to_start);
	    return;
	}
	if (gNum_of_opponents_pursuing + gNum_of_opponents_getting_near >= 3 || pOpponent_spec->player_to_oppo_d <= 10.0) {
	    // if (gNum_of_opponents_completing_race >= 2) {
	        // pursuit_percentage = pOpponent_spec->player_to_oppo_d - 15.0f;
	        // if (PercentageChance(pursuit_percentage)) {
	        //     dr_dprintf("%s: Choosing to get_near because chance dictated it (%d%%)", pOpponent_spec->car_spec->driver_name, pursuit_percentage);
	        //     NewObjective(pOpponent_spec, eOOT_get_near_player);
	        //     return;
	        // }
	        // dr_dprintf("%s: Choosing to complete_race because chance dictated it (%d%%)", pOpponent_spec->car_spec->driver_name, pursuit_percentage);
	        // NewObjective(pOpponent_spec, eOOT_complete_race);
	        // return;
	    // } else
	        dr_dprintf("%s: Choosing to complete_race because not enough oppos are yet (%d/%d)", pOpponent_spec->car_spec->driver_name, gNum_of_opponents_completing_race, 2);
	    NewObjective(pOpponent_spec, eOOT_complete_race);
	    return;
	}
	// dr_dprintf("%s: Choosing to get_near because not enough oppos are yet (%d/%d)", pOpponent_spec->car_spec->driver_name, gNum_of_opponents_pursuing + gNum_of_opponents_getting_near, 3);
	// NewObjective(pOpponent_spec, eOOT_get_near_player);
	return;
    }

    if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec)) {
	NewObjective(pOpponent_spec, eOOT_wait_for_some_hapless_sod);
	return;
    }

    if (!pOpponent_spec->pursue_from_start || gMellow_opponents) {
	printf("Opponent %s is gonna complete the race\n", pOpponent_spec->car_spec->driver_name);
	NewObjective(pOpponent_spec, eOOT_complete_race);
	return;
    }

    gOpponents[pOpponent_spec->index].psyche.grudge_against_player = MIN(100, pOpponent_spec->nastiness * 40.0 + (MAX(20, gOpponents[pOpponent_spec->index].psyche.grudge_against_player) + 20));
    NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
}

void ChooseNewObjective(tOpponent_spec* pOpponent_spec, int pMust_choose_one) {
    switch (pOpponent_spec->ai_profile) {
    case AI_PROFILE_LAP_COMPLETER:
        ChooseNewObjective_LapCompleter(pOpponent_spec, pMust_choose_one);
        break;
    case AI_PROFILE_DEFAULT:
    default:
        ChooseNewObjective_Default(pOpponent_spec, pMust_choose_one);
        break;
    }
}