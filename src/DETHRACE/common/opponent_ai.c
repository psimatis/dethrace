// Opponent AI logic separated from opponent.c
#include "opponent_ai.h"

void PrintObjectiveIfChanged(tOpponent_spec* pOpponent_spec){
    static tOpponent_objective_type last_objective = eOOT_none;
    if (pOpponent_spec->current_objective == last_objective)
        return;

    last_objective = pOpponent_spec->current_objective;

    const char* obj_name = NULL;
    switch (pOpponent_spec->current_objective) {
        case eOOT_none: obj_name = "None"; break;
        case eOOT_complete_race: obj_name = "Complete Race"; break;
        case eOOT_pursue_and_twat: obj_name = "Pursue and Twat"; break;
        case eOOT_run_away: obj_name = "Run Away"; break;
        case eOOT_get_near_player: obj_name = "Get Near Player"; break;
        case eOOT_levitate: obj_name = "Levitate"; break;
        case eOOT_knackered_and_freewheeling: obj_name = "Knackered and Freewheeling"; break;
        case eOOT_frozen: obj_name = "Frozen"; break;
        case eOOT_wait_for_some_hapless_sod: obj_name = "Wait For Some Hapless Sod"; break;
        case eOOT_rematerialise: obj_name = "Rematerialise"; break;
        case eOOT_return_to_start: obj_name = "Return To Start"; break;
        default: obj_name = "Unknown"; break;
    }

    printf("%s objective: %s\n", pOpponent_spec->car_spec->driver_name, obj_name);
}

// IDA: void __usercall ProcessCurrentObjective(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessCurrentObjective(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    PrintObjectiveIfChanged(pOpponent_spec);

    switch (pOpponent_spec->current_objective) {
    case eOOT_complete_race:
        ProcessCompleteRace(pOpponent_spec, pCommand);
        break;
    case eOOT_pursue_and_twat:
        ProcessPursueAndTwat(pOpponent_spec, pCommand);
        break;
    case eOOT_run_away:
        ProcessRunAway(pOpponent_spec, pCommand);
        break;
    case eOOT_get_near_player:
        ProcessGetNearPlayer(pOpponent_spec, pCommand);
        break;
    case eOOT_levitate:
        ProcessLevitate(pOpponent_spec, pCommand);
        break;
    case eOOT_knackered_and_freewheeling:
        // FIXME: is keys correct?
        memset(&pOpponent_spec->car_spec->keys, 0, sizeof(pOpponent_spec->car_spec->keys));
        pOpponent_spec->car_spec->acc_force = 0.f;
        pOpponent_spec->car_spec->brake_force = 0.f;
        pOpponent_spec->car_spec->curvature = 0.f;
        break;
    case eOOT_frozen:
        ProcessFrozen(pOpponent_spec, pCommand);
        break;
    case eOOT_wait_for_some_hapless_sod:
        ProcessWaitForSomeHaplessSod(pOpponent_spec, pCommand);
        break;
    case eOOT_rematerialise:
        break;
    case eOOT_return_to_start:
        ProcessReturnToStart(pOpponent_spec, pCommand);
        break;
    default:
        break;
    }
}
// Add other AI-related functions here as needed

// IDA: void __cdecl NewObjective(tOpponent_spec *pOpponent_spec, tOpponent_objective_type pObjective_type, ...)
void NewObjective(tOpponent_spec* pOpponent_spec, tOpponent_objective_type pObjective_type, ...) {
    va_list marker;
    LOG_TRACE("(%p, %d)", pOpponent_spec, pObjective_type);

    if (pOpponent_spec->current_objective != eOOT_none) {
        ProcessCurrentObjective(pOpponent_spec, ePOC_die);
    }
    pOpponent_spec->current_objective = pObjective_type;
    pOpponent_spec->time_this_objective_started = gTime_stamp_for_this_munging;
    pOpponent_spec->time_for_this_objective_to_finish = gTime_stamp_for_this_munging + IRandomBetween(30, 180) * 1000;
    if (pObjective_type == eOOT_pursue_and_twat) {
        pOpponent_spec->time_for_this_objective_to_finish += 90000;
    }
    switch (pObjective_type) {
    case eOOT_complete_race:
        gNum_of_opponents_completing_race++;
        break;
    case eOOT_pursue_and_twat:
        va_start(marker, pObjective_type);
        pOpponent_spec->pursue_car_data.pursuee = va_arg(marker, tCar_spec*);
        va_end(marker);
        break;
    case eOOT_get_near_player:
        gNum_of_opponents_getting_near++;
        break;
    default:
        break;
    }
    dr_dprintf("%s: NewObjective() - type %d", pOpponent_spec->car_spec->driver_name, pObjective_type);
    ProcessCurrentObjective(pOpponent_spec, ePOC_start);
}

// IDA: void __usercall ProcessCompleteRace(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessCompleteRace(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    br_vector3* initial_pos;
    br_actor* car_actor;
    tComplete_race_data* data;
    int res;
    char str[256];
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    switch (pCommand) {
    case ePOC_start:
        dr_dprintf("%s: ProcessCompleteRace() - new objective started", pOpponent_spec->car_spec->driver_name);
        ClearOpponentsProjectedRoute(pOpponent_spec);
        CalcRaceRoute(pOpponent_spec);
        ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
        break;
    case ePOC_run:
        if (pOpponent_spec->follow_path_data.section_no > 20000) {
            // printf("The weird >20000 if statement in ProcessCompleteRace happened for %s\n", pOpponent_spec->car_spec->driver_name);
            ShiftOpponentsProjectedRoute(pOpponent_spec, pOpponent_spec->follow_path_data.section_no - 20000);
            pOpponent_spec->follow_path_data.section_no = 20000;
        }
        res = ProcessFollowPath(pOpponent_spec, ePOC_run, 0, 0, 0);
        if (pOpponent_spec->nnext_sections == 0 || res == eFPR_end_of_path) {
            dr_dprintf("%s: Giving up following race path because ran out of race path", pOpponent_spec->car_spec->driver_name);
            NewObjective(pOpponent_spec, eOOT_get_near_player);
        }
        if (res != eFPR_OK) {
            if (res == eFPR_given_up) {
                dr_dprintf("%s: Giving up complete_race because ProcessFollowPath() gave up", pOpponent_spec->car_spec->driver_name);
            } else {
                dr_dprintf("%s: Giving up complete_race because reached end", pOpponent_spec->car_spec->driver_name);
            }
            ObjectiveComplete(pOpponent_spec);
        }
        if (gTime_stamp_for_this_munging > pOpponent_spec->time_this_objective_started + 20000) {
            dr_dprintf("%s: Time to give up complete_race. Might be back in a sec, though!", pOpponent_spec->car_spec->driver_name);
            ObjectiveComplete(pOpponent_spec);
        }
        if (pOpponent_spec->nnext_sections < 5 && !pOpponent_spec->complete_race_data.finished_calcing_race_route) {
            CalcRaceRoute(pOpponent_spec);
        }
        break;
    default:
        break;
    }
}

// IDA: void __usercall ProcessPursueAndTwat(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessPursueAndTwat(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    tPursue_car_data* data;
    br_vector3 wank;
    br_vector3 section_v;
    br_vector3 intersect;
    br_scalar d;
    br_scalar s;
    br_scalar t;
    br_scalar distance;
    tFollow_path_result res;
    char str[256];
    tS16 section_no;
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    data = &pOpponent_spec->pursue_car_data;
    if (pCommand == ePOC_start) {
        dr_dprintf("%s: ProcessPursueAndTwat() - new objective started", pOpponent_spec->car_spec->driver_name);
        data->direct_line_nodes[0].number_of_sections = 1;
        data->direct_line_nodes[0].sections[0] = 10000;
        data->direct_line_nodes[1].number_of_sections = 1;
        data->direct_line_nodes[1].sections[0] = 10000;
        data->direct_line_section.node_indices[0] = 10000;
        data->direct_line_section.node_indices[1] = 10001;
        data->direct_line_section.min_speed[0] = 0;
        data->direct_line_section.min_speed[1] = 0;
        data->direct_line_section.max_speed[0] = -1;
        data->direct_line_section.max_speed[1] = -1;
        data->direct_line_section.width = 0.5f;
        data->direct_line_section.type = ePST_normal;
        data->start_backup_time = 0;
        data->time_of_next_visibility_check = 0;
        data->time_pursuee_last_visible = 0;
        data->time_last_twatted_em = 0;
        data->time_last_away_from_pursuee = gTime_stamp_for_this_munging;
        data->state = ePCS_what_now;
        return;
    }

    if (pCommand != ePOC_run) {
        return;
    }

    if (CAR_SPEC_IS_ROZZER(pOpponent_spec->car_spec) && pOpponent_spec->distance_from_home > 75.0f) {
        dr_dprintf("%s: Completing pursuit objective because I'm out of my precinct", pOpponent_spec->car_spec->driver_name);
        NewObjective(pOpponent_spec, eOOT_return_to_start);
        return;
    }

    data->direct_line_section.length = MAX(pOpponent_spec->player_to_oppo_d, 3.0f);
    if (pOpponent_spec->player_to_oppo_d > 3.0f) {
        data->time_last_away_from_pursuee = gTime_stamp_for_this_munging;
    }
    if (gOpponents[pOpponent_spec->index].psyche.grudge_against_player < 15u) {
        dr_dprintf("%s: Completing pursuit objective because I'm happy now", pOpponent_spec->car_spec->driver_name);
        ObjectiveComplete(pOpponent_spec);
        return;
    }
    if (data->state != ePCS_backing_up) {
        if (data->time_last_twatted_em + 1000 >= gTime_stamp_for_this_munging || data->time_last_twatted_em + 3000 <= gTime_stamp_for_this_munging || BrVector3Length(&data->pursuee->v) >= 0.3f) {
            if (data->time_last_away_from_pursuee + 7000 >= gTime_stamp_for_this_munging || data->time_last_twatted_em + 7000 >= gTime_stamp_for_this_munging || data->start_backup_time + 10000 >= gTime_stamp_for_this_munging) {
                if (pOpponent_spec->cheating) {
                    if (pOpponent_spec->player_to_oppo_d < 50.0f
                        && PointVisibleFromHere(&data->pursuee->car_master_actor->t.t.translate.t, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t)) {
                        data->time_pursuee_last_visible = gTime_stamp_for_this_munging;
                    } else {
                        data->time_pursuee_last_visible = 0;
                    }
                } else if (pOpponent_spec->player_in_view_now || (data->time_of_next_visibility_check < gTime_stamp_for_this_munging && pOpponent_spec->player_to_oppo_d < 35.0f && PointVisibleFromHere(&data->pursuee->car_master_actor->t.t.translate.t, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t))) {
                    data->time_pursuee_last_visible = gTime_stamp_for_this_munging;
                    data->time_of_next_visibility_check = gTime_stamp_for_this_munging + 600;
                }
                if (data->time_pursuee_last_visible + 3000 <= gTime_stamp_for_this_munging) {
                    if (data->pursuee->my_trail.number_of_nodes < 2) {
                        dr_dprintf("%s: Giving up pursuit - not visible & no trail yet", pOpponent_spec->car_spec->driver_name);
                        NewObjective(pOpponent_spec, eOOT_get_near_player);
                        return;
                    }
                    if (data->state != ePCS_following_trail) {
                        section_no = FindNearestTrailSection(pOpponent_spec, data->pursuee, &section_v, &intersect, &distance);
                        data->state = ePCS_following_trail;
                        if (distance > 20.0f || section_no == -1) {
                            dr_dprintf("%s: Giving up pursuit - not visible & trail ain't close enough (%f)", pOpponent_spec->car_spec->driver_name, distance);
                            NewObjective(pOpponent_spec, eOOT_get_near_player);
                            return;
                        }
                        dr_dprintf("%s: Commencing ePCS_following_trail state", pOpponent_spec->car_spec->driver_name);
                        pOpponent_spec->follow_path_data.section_no = section_no;
                        ProcessFollowPath(pOpponent_spec, ePOC_start, 1, 0, 0);
                    }
                } else if (data->state != ePCS_following_line_of_sight) {
                    dr_dprintf("%s: Commencing ePCS_following_line_of_sight state", pOpponent_spec->car_spec->driver_name);
                    data->state = ePCS_following_line_of_sight;
                    sprintf(str, "%s: I've spotted you!", pOpponent_spec->car_spec->driver_name);
                    ProcessFollowPath(pOpponent_spec, ePOC_start, 1, 1, 0);
                }
            } else {
                dr_dprintf("%s: Backing up because we're too close to pursuee without having twatted him", pOpponent_spec->car_spec->driver_name);
                data->start_backup_time = gTime_stamp_for_this_munging;
                data->state = ePCS_backing_up;
            }
        } else {
            dr_dprintf("%s: Backing up because we're 'stationary' after colliding with pursuee", pOpponent_spec->car_spec->driver_name);
            data->start_backup_time = gTime_stamp_for_this_munging;
            data->state = ePCS_backing_up;
        }
    }
    switch (data->state) {
    case ePCS_what_now:
        PDEnterDebugger("ERROR: what_now state called in ProcessPursueAndTwat()");
        break;
    case ePCS_following_trail:
        if (data->pursuee->my_trail.nodes_shifted_this_frame) {
            if (pOpponent_spec->follow_path_data.section_no <= 15000) {
                data->state = ePCS_following_trail;
                section_no = FindNearestTrailSection(pOpponent_spec, data->pursuee, &section_v, &intersect, &distance);
                dr_dprintf("%s: Trail got away; found new trail section %d", pOpponent_spec->car_spec->driver_name, section_no);
                if (section_no == -1 || distance > 20.0f || !PointVisibleFromHere(&intersect, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t)) {
                    dr_dprintf("%s: ...which unfortunately is too far away (%fBRU) or not visible - end of pursuit", pOpponent_spec->car_spec->driver_name, distance);
                    NewObjective(pOpponent_spec, eOOT_get_near_player);
                    return;
                }
                pOpponent_spec->follow_path_data.section_no = section_no;
                ProcessFollowPath(pOpponent_spec, ePOC_start, 1, 0, 0);
            } else {
                pOpponent_spec->follow_path_data.section_no--;
            }
            dr_dprintf("%s: Following re-jobbied section %d/%d", pOpponent_spec->car_spec->driver_name, pOpponent_spec->follow_path_data.section_no, data->pursuee->my_trail.number_of_nodes - 1);
        }
        sprintf(str, "%s: Trail section %d/%d", pOpponent_spec->car_spec->driver_name, pOpponent_spec->follow_path_data.section_no, data->pursuee->my_trail.number_of_nodes - 1);
        res = ProcessFollowPath(pOpponent_spec, ePOC_run, 1, 0, 0);
        if (res == eFPR_given_up || res == eFPR_end_of_path) {
            NewObjective(pOpponent_spec, eOOT_get_near_player);
        }
        break;
    case ePCS_following_line_of_sight:
        BrVector3Copy(&data->direct_line_nodes[0].p, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t);
        BrVector3Sub(&wank, &data->pursuee->car_master_actor->t.t.translate.t, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t);
        s = BrVector3Length(&wank);
        BrVector3Sub(&wank, &data->pursuee->v, &pOpponent_spec->car_spec->v);
        t = BrVector3Length(&wank);
        if (t >= 1.0f) {
            d = s / t / 2.0;

        } else {
            d = 0.0;
        }
        BrVector3Scale(&data->direct_line_nodes[1].p, &data->pursuee->v, d);
        BrVector3Accumulate(&data->direct_line_nodes[1].p, &data->pursuee->car_master_actor->t.t.translate.t);
        if (s >= 2.0f) {
            ProcessFollowPath(pOpponent_spec, ePOC_run, 1, 1, 0);
        } else {
            ProcessFollowPath(pOpponent_spec, ePOC_run, 1, 1, 1);
        }
        break;
    case ePCS_backing_up:
        if (data->start_backup_time + 2200 >= gTime_stamp_for_this_munging) {
            pOpponent_spec->car_spec->curvature = 0.0f;
            pOpponent_spec->car_spec->brake_force = 0.0f;
            pOpponent_spec->car_spec->acc_force = pOpponent_spec->car_spec->M * -8.0f;
        } else {
            pOpponent_spec->car_spec->acc_force = 0.0;
            pOpponent_spec->car_spec->brake_force = pOpponent_spec->car_spec->M * 15.0f;
            if (data->start_backup_time + 3000 < gTime_stamp_for_this_munging) {
                pOpponent_spec->car_spec->brake_force = 0.0f;
                data->state = ePCS_what_now;
                dr_dprintf("%s: Finished backing up.", pOpponent_spec->car_spec->driver_name);
            }
        }
        break;
    default:
        return;
    }
}

// IDA: void __usercall ProcessRunAway(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessRunAway(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    int res;
    tS16 section_no;
    br_scalar distance;
    br_vector3 intersect;
    br_vector3 direction_v;
    char str[256];
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    switch (pCommand) {

    case ePOC_run:
        if (pOpponent_spec->run_away_data.time_to_stop >= gTime_stamp_for_this_munging) {
            if (pOpponent_spec->follow_path_data.section_no > 20000) {
                ShiftOpponentsProjectedRoute(pOpponent_spec, pOpponent_spec->follow_path_data.section_no - 20000);
                pOpponent_spec->follow_path_data.section_no = 20000;
            }
            if (pOpponent_spec->nnext_sections < 10) {
                TopUpRandomRoute(pOpponent_spec, 10 - pOpponent_spec->nnext_sections);
            }
            if (ProcessFollowPath(pOpponent_spec, ePOC_run, 0, 0, 0) == eFPR_given_up) {
                ClearOpponentsProjectedRoute(pOpponent_spec);
                section_no = FindNearestPathSection(&pOpponent_spec->car_spec->car_master_actor->t.t.translate.t, &direction_v, &intersect, &distance);
                if (BrVector3Dot(&pOpponent_spec->car_spec->direction, &direction_v) < 0.0f) {
                    AddToOpponentsProjectedRoute(pOpponent_spec, section_no, 0);
                } else {
                    AddToOpponentsProjectedRoute(pOpponent_spec, section_no, 1);
                }
                TopUpRandomRoute(pOpponent_spec, -1);
                ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
            }
        } else {
            ObjectiveComplete(pOpponent_spec);
        }
        break;

    case ePOC_start:
        dr_dprintf("%s: ProcessRunAway() - new objective started", pOpponent_spec->car_spec->driver_name);
        pOpponent_spec->run_away_data.time_to_stop = gTime_stamp_for_this_munging + 1000 * IRandomBetween(30, 90);
        ClearOpponentsProjectedRoute(pOpponent_spec);
        section_no = FindNearestPathSection(&pOpponent_spec->car_spec->car_master_actor->t.t.translate.t, &direction_v, &intersect, &distance);
        if (BrVector3Dot(&pOpponent_spec->car_spec->direction, &direction_v) < 0.0f) {
            AddToOpponentsProjectedRoute(pOpponent_spec, section_no, 0);
        } else {
            AddToOpponentsProjectedRoute(pOpponent_spec, section_no, 1);
        }
        TopUpRandomRoute(pOpponent_spec, -1);
        ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
        sprintf(str, "%s: Shit! I'm out of here...", pOpponent_spec->car_spec->driver_name);
        break;

    case ePOC_die:
        break;
    }
}

// IDA: void __usercall ProcessWaitForSomeHaplessSod(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessWaitForSomeHaplessSod(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    switch (pCommand) {
    case ePOC_start:
    case ePOC_run:
        pOpponent_spec->car_spec->brake_force = 15.f * pOpponent_spec->car_spec->M;
        break;
    default:
        break;
    }
}

// IDA: void __usercall ProcessReturnToStart(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessReturnToStart(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    br_vector3 section_v;
    br_vector3 our_pos_xz;
    br_vector3 cop_to_start;
    br_scalar distance;
    int res;
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    switch (pCommand) {
    case ePOC_run:
        if (TeleportCopToStart(pOpponent_spec)) {
            break;
        }
        if (pOpponent_spec->return_to_start_data.waiting_near_start) {
            pOpponent_spec->car_spec->brake_force = pOpponent_spec->car_spec->M * 15.0f;
        } else {
            our_pos_xz = pOpponent_spec->car_spec->car_master_actor->t.t.translate.t;
            our_pos_xz.v[1] = 0.0f;
            BrVector3Sub(&cop_to_start, &pOpponent_spec->start_pos, &our_pos_xz);
            if (BrVector3Length(&cop_to_start) >= 10.0) {
                if (pOpponent_spec->follow_path_data.section_no > 20000) {
                    ShiftOpponentsProjectedRoute(pOpponent_spec, pOpponent_spec->follow_path_data.section_no - 20000);
                    pOpponent_spec->follow_path_data.section_no = 20000;
                }
                if (pOpponent_spec->nnext_sections <= 4) {
                    CalcReturnToStartPointRoute(pOpponent_spec);
                }
                res = ProcessFollowPath(pOpponent_spec, ePOC_run, 0, 0, 0);
                if (res == eFPR_given_up || res == eFPR_end_of_path) {
                    if (res == eFPR_given_up) {
                        dr_dprintf("%s: Restarting return_to_start route because ProcessFollowPath() gave up.", pOpponent_spec->car_spec->driver_name);
                    } else {
                        dr_dprintf("%s: Restarting return_to_start route because ran out of path!", pOpponent_spec->car_spec->driver_name);
                    }
                    ClearOpponentsProjectedRoute(pOpponent_spec);
                    CalcReturnToStartPointRoute(pOpponent_spec);
                    ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
                }
            } else {
                pOpponent_spec->return_to_start_data.waiting_near_start = 1;
                pOpponent_spec->car_spec->brake_force = pOpponent_spec->car_spec->M * 15.0f;
            }
        }
        break;
    case ePOC_start:
        dr_dprintf("%s: ProcessReturnToStart() - new objective started", pOpponent_spec->car_spec->driver_name);
        pOpponent_spec->return_to_start_data.waiting_near_start = 0;
        pOpponent_spec->return_to_start_data.section_no = FindNearestPathSection(&pOpponent_spec->start_pos, &section_v, &pOpponent_spec->return_to_start_data.nearest_path_point, &distance);
        pOpponent_spec->return_to_start_data.nearest_path_point.v[1] = 0.0;
        CalcReturnToStartPointRoute(pOpponent_spec);
        ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
        break;
    default:
        break;
    }
}

// IDA: void __usercall ProcessLevitate(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessLevitate(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    float t;
    float terminal_time;
    float y;
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    if (pCommand == ePOC_start) {
        dr_dprintf("%s: ProcessLevitate() - new objective started", pOpponent_spec->car_spec->driver_name);
        pOpponent_spec->levitate_data.waiting_to_levitate = 1;
        pOpponent_spec->car_spec->brake_force = 15.f * pOpponent_spec->car_spec->M;
        pOpponent_spec->car_spec->acc_force = 0.f;
        pOpponent_spec->levitate_data.time_started = gTime_stamp_for_this_munging;
    } else if (pCommand == ePOC_run) {
        if (pOpponent_spec->levitate_data.waiting_to_levitate) {
            if ((BrVector3Length(&pOpponent_spec->car_spec->v) < .01f && BrVector3Length(&pOpponent_spec->car_spec->omega) < 1.f) || gTime_stamp_for_this_munging - pOpponent_spec->levitate_data.time_started > 4000) {
                pOpponent_spec->levitate_data.waiting_to_levitate = 0;
                pOpponent_spec->levitate_data.time_started = gTime_stamp_for_this_munging;
                pOpponent_spec->levitate_data.initial_y = pOpponent_spec->car_spec->car_master_actor->t.t.translate.t.v[1];
                if (pOpponent_spec->car_spec->has_been_stolen) {
                    NewTextHeadupSlot(eHeadupSlot_misc, 250, 2500, -4, GetMiscString(kMiscString_CarAddedToChangeCarList));
                }
            } else {
                pOpponent_spec->car_spec->brake_force = 15.f * pOpponent_spec->car_spec->M;
                pOpponent_spec->car_spec->acc_force = 0.f;
                BrVector3InvScale(&pOpponent_spec->car_spec->omega, &pOpponent_spec->car_spec->omega,
                    powf(gFrame_period_for_this_munging / 1000.f, 2.f));
            }
        }
        if (!pOpponent_spec->levitate_data.waiting_to_levitate) {
            TurnOpponentPhysicsOff(pOpponent_spec);
            t = (gTime_stamp_for_this_munging - pOpponent_spec->levitate_data.time_started) / 1000.f;
            if (t < 20.f) {
                y = .5f * t * t / 2.f;
            } else {
                y = 10.f * (t - 20.f) + 100.f;
            }
            pOpponent_spec->car_spec->car_master_actor->t.t.translate.t.v[1] = pOpponent_spec->levitate_data.initial_y + y;
            if (y > 200.f) {
                pOpponent_spec->finished_for_this_race = 1;
            }
        }
    }
}

// IDA: void __usercall ProcessGetNearPlayer(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessGetNearPlayer(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    br_vector3* initial_pos;
    br_actor* car_actor;
    int res;
    char str[256];
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    if (pCommand == ePOC_start) {
        dr_dprintf("%s: ProcessGetNearPlayer() - new objective started", pOpponent_spec->car_spec->driver_name);
        ClearOpponentsProjectedRoute(pOpponent_spec);
        CalcGetNearPlayerRoute(pOpponent_spec, &gProgram_state.current_car);
        ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
        return;
    }
    if (pCommand == ePOC_run) {
        if ((pOpponent_spec->car_spec->car_ID & 0xff00) == 768 && pOpponent_spec->distance_from_home > 75.0) {
            dr_dprintf("%s: Completing get_near objective because I'm out of my precinct", pOpponent_spec->car_spec->driver_name);
            NewObjective(pOpponent_spec, eOOT_return_to_start);
            return;
        }
        if (pOpponent_spec->follow_path_data.section_no > 20000) {
            if (pOpponent_spec->player_to_oppo_d < 10.0 || pOpponent_spec->follow_path_data.section_no == pOpponent_spec->players_section_when_last_calced_full_path) {
                dr_dprintf("%s: ProcessGetNearPlayer() - giving up 'cos got to player's section", pOpponent_spec->car_spec->driver_name);
                ObjectiveComplete(pOpponent_spec);
                return;
            }
            ShiftOpponentsProjectedRoute(pOpponent_spec, pOpponent_spec->follow_path_data.section_no - 20000);
            pOpponent_spec->follow_path_data.section_no = 20000;
        }
        if (pOpponent_spec->nnext_sections <= 4) {
            CalcGetNearPlayerRoute(pOpponent_spec, &gProgram_state.current_car);
        }
        res = ProcessFollowPath(pOpponent_spec, ePOC_run, 0, 0, 0);
        sprintf(str, "Get near: %d", GetOpponentsRealSection(pOpponent_spec, pOpponent_spec->follow_path_data.section_no));

        if (res == eFPR_given_up) {
            NewObjective(pOpponent_spec, eOOT_pursue_and_twat, &gProgram_state.current_car);
        } else if (res == eFPR_end_of_path) {
            dr_dprintf("%s: Restarting get_near_player route because ran out of path!", pOpponent_spec->car_spec->driver_name);
            ClearOpponentsProjectedRoute(pOpponent_spec);
            CalcGetNearPlayerRoute(pOpponent_spec, &gProgram_state.current_car);
            ProcessFollowPath(pOpponent_spec, ePOC_start, 0, 0, 0);
        }
    }
}

// IDA: void __usercall ProcessFrozen(tOpponent_spec *pOpponent_spec@<EAX>, tProcess_objective_command pCommand@<EDX>)
void ProcessFrozen(tOpponent_spec* pOpponent_spec, tProcess_objective_command pCommand) {
    LOG_TRACE("(%p, %d)", pOpponent_spec, pCommand);

    switch (pCommand) {
    case ePOC_start:
        dr_dprintf("%d ProcessFrozen() - new task started", pOpponent_spec->index);
        dr_dprintf("%s: Rematerialising from ePOC_start in ProcessFrozen()...", pOpponent_spec->car_spec->driver_name);
        RematerialiseOpponentOnNearestSection(pOpponent_spec, 0.f);
        pOpponent_spec->car_spec->acc_force = 0.f;
        pOpponent_spec->car_spec->brake_force = 15.f * pOpponent_spec->car_spec->M;
        break;
    case ePOC_run:
        pOpponent_spec->car_spec->brake_force = 15.f * pOpponent_spec->car_spec->M;
        break;
    case ePOC_die:
        pOpponent_spec->car_spec->brake_force = 0.f;
        break;
    }
}

// IDA: void __usercall ObjectiveComplete(tOpponent_spec *pOpponent_spec@<EAX>)
void ObjectiveComplete(tOpponent_spec* pOpponent_spec) {
    LOG_TRACE("(%p)", pOpponent_spec);

    dr_dprintf("%s: Objective Completed", pOpponent_spec->car_spec->driver_name);
    pOpponent_spec->new_objective_required = 1;
    switch (pOpponent_spec->current_objective) {
    case eOOT_complete_race:
        gNum_of_opponents_completing_race--;
        break;
    case eOOT_pursue_and_twat:
        gNum_of_opponents_pursuing--;
        break;
    case eOOT_get_near_player:
        gNum_of_opponents_getting_near--;
        break;
    default:
        break;
    }
}

// IDA: void __usercall ChooseNewObjective(tOpponent_spec *pOpponent_spec@<EAX>, int pMust_choose_one@<EDX>)
void ChooseNewObjective(tOpponent_spec* pOpponent_spec, int pMust_choose_one) {
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

// IDA: void __usercall ProcessThisOpponent(tOpponent_spec *pOpponent_spec@<EAX>)
void ProcessThisOpponent(tOpponent_spec* pOpponent_spec) {
    int i;
    LOG_TRACE("(%p)", pOpponent_spec);

    if ((gMap_mode && gShow_opponents) || pOpponent_spec->last_in_view + 3000 >= gTime_stamp_for_this_munging) {
        if (pOpponent_spec->cheating) {
            OiStopCheating(pOpponent_spec);
        }
    } else if (pOpponent_spec->cheating == 0) {
        StartToCheat(pOpponent_spec);
    }
    ChooseNewObjective(pOpponent_spec, pOpponent_spec->new_objective_required);
    pOpponent_spec->new_objective_required = 0;
    if (gCountdown || gRace_finished) {
        pOpponent_spec->car_spec->brake_force = pOpponent_spec->car_spec->M * 10.f;
    }
    if (!pOpponent_spec->finished_for_this_race && !gStop_opponents_moving && !gRace_finished && pOpponent_spec->stun_time_ends < gTime_stamp_for_this_munging) {
        ProcessCurrentObjective(pOpponent_spec, ePOC_run);
    }
    if (pOpponent_spec->cheating) {
        BrVector3Copy(&pOpponent_spec->car_spec->pos, &pOpponent_spec->car_spec->car_master_actor->t.t.translate.t);
    }
}

void IncrementOpponentCheckpoint(tOpponent_spec* ai, int num_checkpoints, int num_laps) {
    if (ai->finished) return;
    ai->checkpoint++;
    LOG_TRACE("AI %p checkpoint incremented to %d (lap %d)", ai, ai->checkpoint, ai->lap);
    printf("AI %p at checkpoint %d (lap %d)\n", ai, ai->checkpoint, ai->lap);
    if (ai->checkpoint >= num_checkpoints) {
        ai->checkpoint = 0;
        ai->lap++;
        LOG_TRACE("AI %p completed a lap! New lap: %d", ai, ai->lap);
        printf("AI %p completed a lap! New lap: %d\n", ai, ai->lap);
        // Optional: Log or trigger lap-complete effects
    }
    if (ai->lap >= num_laps) {
        ai->finished = 1;
        LOG_TRACE("AI %p finished the race! (lap %d)", ai, ai->lap);
        printf("AI %p finished the race! (lap %d)\n", ai, ai->lap);
        // Only end the race if it isn't already finished
        if (!gRace_finished) {
            RaceCompleted(eRace_over_out_of_time);
        }
    }
}

void CheckOpponentCheckpoints(void) {
    int i;
    for (i = 0; i < gProgram_state.AI_vehicles.number_of_opponents; i++) {
        tOpponent_spec *opp = &gProgram_state.AI_vehicles.opponents[i];
        if (opp->finished) continue;

        tCar_spec *car = opp->car_spec;
        br_vector3 pos_now = car->car_master_actor->t.t.translate.t;
        br_vector3 pos_prev;
        BrVector3Copy(&pos_prev, (br_vector3 *)&car->old_frame_mat.m[3]);

        int next_cp = opp->checkpoint + 1;
        if (next_cp > gCurrent_race.check_point_count)
            next_cp = 1;

        tCheckpoint *cp = &gCurrent_race.checkpoints[next_cp - 1];

        int quad_count = cp->quad_count;
        int crossed = 0;
        br_vector3 dir;
        BrVector3Sub(&dir, &pos_now, &pos_prev);
        for (int j = 0; j < quad_count; j++) {
            if (RayHitFace(&cp->vertices[j][0], &cp->vertices[j][1], &cp->vertices[j][2], &cp->normal[j], &pos_prev, &dir)) {
                crossed = 1;
                break;
            }
            if (RayHitFace(&cp->vertices[j][0], &cp->vertices[j][2], &cp->vertices[j][3], &cp->normal[j], &pos_prev, &dir)) {
                crossed = 1;
                break;
            }
        }
        if (crossed) {
            printf("AI %d crossed the checkpoint (player logic)\n", i);
            IncrementOpponentCheckpoint(opp, gCurrent_race.check_point_count, gCurrent_race.total_laps);
        }
    }
}