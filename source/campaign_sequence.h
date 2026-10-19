#ifndef CAMPAIGN_SEQUENCE_H
#define CAMPAIGN_SEQUENCE_H

/* Original owned LEVEL13 script references, not an invented boss encounter.
 * Main/frame_1 f_Main: WP3=3, WP4=3, WP7/9=3, WP11=2.
 * Ladder movies32/frame55 and33/frame81 each add one more kill_goal.
 * The exit f_LeaveLevel2 completes13 and unlocks19; princesses are abducted,
 * not rescued, in this level. Scene timing/captions below are DS adaptations.
 */
enum {
    KEEP_SOURCE_LEVEL = 13,
    KEEP_NEXT_SOURCE_LEVEL = 19,
    KEEP_ENCOUNTER_COUNT = 4,
    KEEP_BASE_ENEMY_COUNT = 11,
    KEEP_LADDER_ENEMY_COUNT = 2,
    KEEP_TOTAL_ENEMY_COUNT = 13,
    KEEP_LADDER_ONE_HEIGHT = 35,
    KEEP_LADDER_TWO_HEIGHT = 45
};
static const unsigned char keep_encounter_enemies[KEEP_ENCOUNTER_COUNT] = {3,3,3,4};
static const unsigned char keep_encounter_waypoints[KEEP_ENCOUNTER_COUNT] = {3,4,9,11};

/* LEVEL13 movie407 original placements, in Flash twips (20/source pixel).
 * These three living neutral peasants enter at WP2 and flee LEFT. Their
 * frame2 load scripts select f_NPCWalkLeft at speeds6,8,5 source pixels.
 * Other grey-knight neutral scenery (161,169,171) depicts fallen soldiers.
 */
typedef struct { int x_twips,y_twips;unsigned speed; } KeepRunnerSource;
static const KeepRunnerSource keep_runner_sources[3] = {
    {36598,-2690,6},{40298,-2011,8},{38398,-1031,5}
};

/* Condensed narration of the original timelines264 (crystal theft) and360
 * (princess abduction). These are captions, not purported original dialogue. */
#define KEEP_INTRO_TITLE "CASTLE KEEP"
#define KEEP_INTRO_CAPTION "THE WIZARD HAS STOLEN THE CRYSTAL!"
#define KEEP_MID_TITLE "THE PRINCESSES!"
#define KEEP_MID_CAPTION "BARBARIANS ARE TAKING THEM AWAY!"
#define KEEP_EXIT_CAPTION "CLEAR THE GATE AND FOLLOW THEM!"
#define KEEP_COMPLETE_CAPTION "CASTLE KEEP COMPLETE"

/* MAIN f_UpdatePlayerAttributes11374..80, f_Exp102659..90,
 * f_SpawnBarbarian6014 and6049..52, f_GetPickup103300..76.
 * Physical hit damage is modified by the target's resistance separately.
 */
enum {
    KEEP_XP_PER_HIT = 1,
    KEEP_FIRST_XP_THRESHOLD = 190,
    KEEP_XP_STEP_INCREMENT = 20,
    KEEP_MAX_LEVEL = 99,
    KEEP_BARBARIAN_HP_SOLO = 50,
    KEEP_BARBARIAN_HP_COOP = 60,
    KEEP_FOOD_PERCENT = 10,
    KEEP_CHICKEN_PERCENT = 50,
    KEEP_COIN_GOLD = 1,
    KEEP_BAG_GOLD = 5,
    KEEP_PICKUP_RANGE_X = 25,
    KEEP_PICKUP_RANGE_Y = 10,
    KEEP_PICKUP_MAX_Z = 15
};
static const unsigned char keep_combo_unlock_levels[7] = {1,2,4,8,16,32,50};

/* Cumulative threshold for advancing FROM level. Original f_Exp uses >,
 * so a fresh character reaches level2 at191 XP, level3 at401 XP. */
static inline unsigned keep_xp_threshold(unsigned level) {
    return 190u*level + 10u*level*(level-1u);
}

#endif
