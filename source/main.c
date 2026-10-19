#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "frame_clock.h"
#include "overlay_copy.h"
#include <nds.h>
#include <fat.h>
#include <dswifi9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "arena.h"
#include "effects.h"
#include "stage.h"
#include "render.h"
#include "hud.h"
#include "menu.h"
#include "audio.h"
#include "protocol.h"
#include "knights.h"
#include "bot.h"
#include "campaign.h"
#include "cinematic.h"
#include "campaign_weapons.h"
#include "campaign_save.h"
#include "campaign_link.h"
#include "campaign_menu.h"
#include "campaign_feedback.h"
#include "chapter_assets.h"
#include "bottom_ui.h"
#include "item_selector.h"
#include "ui_skin.h"

#define GAME_ID 0x43415218u
enum { MENU, MULTIPLAYER, CONTROLS, PRACTICE, HOST_SETUP, HOST, SCAN_SETUP, SCAN, CONNECT, CLIENT, ARENA_SELECT, KNIGHT_SELECT, ERROR_SCREEN, CAMPAIGN_MENU };
static volatile int mode=MENU;
static uint32_t frames,deadline,send_sequence,last_state_sequence;
static volatile uint32_t display_refresh;
static uint32_t pending_refresh;
static void display_vblank(void){display_refresh++;}
static int map_selected,map_for_host,uploaded_map=-1,host_ready,opponent_selected=1;
static int front_has_terrain;
static int knight_selected,client_confirmed;
static int practice_difficulty=BOT_MEDIUM;
static int campaign_mode,campaign_selected;
static Campaign campaign;
static CampaignPacket campaign_inbox;
#define CAMPAIGN_GAME_ID 0x43414337u
static const char *knight_name(unsigned id){
    static const char *names[]={"Blue Knight","Orange Knight","Red Knight","Green Knight"};
    return names[id<KNIGHT_COUNT?id:0];
}
static int selected,menu_selected,multi_selected,room_count,wifi_ready,state_seen,paused;
static int pause_selected;
static unsigned item_cursor,item_use_pending;
static Wifi_AccessPoint rooms[8];
static Arena arena;
static const char *error="";
static RemoteInput remote;
static InputPacket local_packet={.magic=PROTOCOL_MAGIC};
static volatile int input_age,state_age,pending;
static StatePacket inbox;
static uint16_t guest_mac[3];
static volatile int guest_known,guest_aid;
static unsigned measured_fps,measured_work_ms,render_count;
static uint16_t pixels[256*192] __attribute__((aligned(32)));
static uint16_t bottom[256*192] __attribute__((aligned(32)));
static int bg,sub,terrain;
static int front_base=16,pending_base=10,top_pending,bottom_pending,pending_terrain;
static ArenaCamera pending_camera;
static uint16_t page_left[2][192],page_right[2][192];
static int local_hero;
static int save_ready,save_status,campaign_panel,panel_selected,request_waiting,guest_profile_ready;
static CampaignFeedback campaign_feedback;
static CampaignRequest request_out,request_in;
static CampaignRequestState request_state;
static volatile int request_pending;
static uint32_t request_sequence;
static void save_init(void){
    if(save_status)return;
    save_status=3;campaign_save_open(NULL);
    int mounted=fatInit(1,true); /* 4 KiB cache; after the visible main menu. */
    audio_prepare_assets();
    if(!mounted)return;
    const char *drive=fatGetDefaultDrive();
    if(strcmp(drive,"sd:/")&&strcmp(drive,"fat:/"))return;
    char prefix[80];snprintf(prefix,sizeof(prefix),"%scastle-crashers-ds",drive);
    campaign_save_open(prefix);save_ready=1;save_status=1;
}
static void save_capture(int flush){
    if(!campaign_mode||!arena.running)return;
    unsigned h=(unsigned)local_hero;
    if(arena.p[h].ready&&arena.p[h].knight_id==(unsigned)knight_selected)campaign_save_capture(&campaign,h);
    if(flush&&save_ready&&campaign_save_dirty())save_status=campaign_save_flush()?2:4;
}
static void load_character(unsigned hero,unsigned knight){
    campaign_character_put(campaign_save_character(knight),&campaign,hero);
}
static int campaign_peer_loading,campaign_peer_stamp=-1,chapter_ready_sent=-1,client_transition_grace,client_transition_stamp=-1;
static int campaign_scene_stamp(void){return ((int)arena.map_id<<8)|(campaign.phase==CAMPAIGN_WORLD_MAP?CAMPAIGN_WORLD_MAP:0);}
static void campaign_sync_transition(void){
    if(mode==HOST&&campaign_mode&&arena.running&&campaign_peer_stamp!=campaign_scene_stamp()){
        campaign_peer_stamp=campaign_scene_stamp();campaign_peer_loading=1;input_age=0;
    }
}
static void request_send(unsigned operation,unsigned slot){
    if(request_waiting)return;
    request_out=(CampaignRequest){.magic=CAMPAIGN_REQUEST_MAGIC,.version=CAMPAIGN_VERSION,.sequence=++request_sequence,.operation=operation,.slot=slot};
    if(operation==CAMPAIGN_REQUEST_PROFILE)request_out.character=*campaign_save_character((unsigned)knight_selected);
    request_waiting=1;
}
static void panel_action(unsigned slot){
    if(mode==CLIENT)request_send(CAMPAIGN_REQUEST_STAT,slot);
    else campaign_spend(&campaign,&arena,0,slot);
}
#ifdef BOOT_DIAGNOSTIC
/* Direct key polling also works if the ARM7 input service has stalled. */
static void boot_continue(void){
    while(!(REG_KEYINPUT&KEY_A)){}
    while(REG_KEYINPUT&KEY_A){}
    while(!(REG_KEYINPUT&KEY_A)){}
}
#endif
/* The unused rows 320..511 of BG3 hold the alternate 256x192 overlay.
   Neither camera nor terrain sampling can read this reserved region. */
_Static_assert(WORLD_HEIGHT*512<=10*16384,"Terrain overlaps back buffer");
_Static_assert(10*16384+256*192*2==16*16384,"Back buffer VRAM layout");
static void present(void){
    int terrain_copy=0;
    if(top_pending&&frame_clock_present_due(display_refresh,pending_refresh)){
        if(pending_terrain)terrain_copy=arena_present_background();
        bgSetMapBase(bg,pending_base);front_base=pending_base;bgShow(bg);
        if(pending_terrain){
            bgShow(terrain);bgSetRotateScale(terrain,0,pending_camera.width,pending_camera.width);
            bgSetScroll(terrain,pending_camera.x,pending_camera.y);
        }else bgHide(terrain);
        bgUpdate();front_has_terrain=pending_terrain;top_pending=0;
    }
    if(bottom_pending&&!terrain_copy){
        DC_FlushRange(bottom,sizeof(bottom));dmaCopy(bottom,bgGetGfxPtr(sub),sizeof(bottom));
        bottom_pending=0;
    }
}

static void from_client(Wifi_MPPacketType type,int aid,int base,int len){
    if(mode!=HOST||!guest_known||aid!=guest_aid||type!=WIFI_MPTYPE_REPLY||len!=sizeof(InputPacket))return;
    InputPacket p;Wifi_RxRawReadPacket(base,sizeof(p),&p);
    if(campaign_mode&&p.magic==CAMPAIGN_REQUEST_MAGIC){CampaignRequest r;memcpy(&r,&p,sizeof(r));if(campaign_request_valid(&r)){request_in=r;request_pending=1;input_age=0;}return;}
    if(protocol_remote_accept(&remote,&p))input_age=0;
}
static void from_host(Wifi_MPPacketType type,int base,int len){
    if((mode!=CLIENT&&mode!=CONNECT)||type!=WIFI_MPTYPE_CMD||len!=sizeof(StatePacket))return;
    if(campaign_mode){
        CampaignPacket p;Wifi_RxRawReadPacket(base,sizeof(p),&p);
        if(!campaign_packet_valid(&p)||(state_seen&&!protocol_sequence_newer(p.sequence,last_state_sequence)))return;
        campaign_inbox=p;pending=1;state_age=0;last_state_sequence=p.sequence;state_seen=1;return;
    }
    StatePacket p;Wifi_RxRawReadPacket(base,sizeof(p),&p);
    if(!protocol_state_valid(&p)||(state_seen&&!protocol_sequence_newer(p.sequence,last_state_sequence)))return;
    inbox=p;pending=1;state_age=0;last_state_sequence=p.sequence;state_seen=1;
}
static void fail(const char *message){save_capture(1);paused=0;campaign_panel=0;mode=ERROR_SCREEN;if(wifi_ready)Wifi_IdleMode();error=message;}
static void leave(void){item_cursor=item_use_pending=0;save_capture(1);campaign_panel=request_waiting=0;mode=MENU;campaign_mode=0;if(wifi_ready){Wifi_DisconnectAP();Wifi_IdleMode();}arena_init(&arena);paused=0;client_confirmed=0;}
static uint16_t controls(int k){
    return ((k&KEY_LEFT)?LEFT:0)|((k&KEY_RIGHT)?RIGHT:0)|((k&KEY_UP)?UP:0)|((k&KEY_DOWN)?DOWN:0)|
        ((k&KEY_A)?ATTACK:0)|((k&KEY_Y)?BOW:0)|((k&KEY_X)?HEAVY:0)|((k&KEY_B)?JUMP:0)|((k&KEY_R)?BLOCK:0)|((k&KEY_L)?MAGIC:0);
}
static void radio_start(int host){
    local_hero=host?0:1;
#ifdef BOOT_FTL_CORE
    /* This diagnostic has no ARM7 radio service. Never send it Wi-Fi commands. */
    error="Wireless off in boot test";mode=ERROR_SCREEN;return;
#endif
#ifdef DEFERRED_RADIO
    if(!wifi_ready){
        fifoSendValue32(FIFO_USER_01,0x43415201u);
        int ready=0;
        for(int wait=0;wait<180&&!ready;wait++){
            if(fifoCheckValue32(FIFO_USER_01))ready=fifoGetValue32(FIFO_USER_01)==0x43415202u;
            if(!ready)swiWaitForVBlank();
        }
        if(!ready){error="Radio service did not start";mode=ERROR_SCREEN;return;}
    }
#endif
    if(!wifi_ready)wifi_ready=Wifi_InitDefault(INIT_ONLY|WIFI_LOCAL_ONLY);
    if(!wifi_ready){error="Radio initialization failed";mode=ERROR_SCREEN;return;}
    arena_lobby_init(&arena);arena.map_id=map_selected;stage_set_map(arena.map_id);paused=0;client_confirmed=0;
    campaign_peer_loading=0;campaign_peer_stamp=chapter_ready_sent=client_transition_stamp=-1;client_transition_grace=0;
    request_pending=request_waiting=guest_profile_ready=0;request_state=(CampaignRequestState){0};request_sequence=0;
    if(campaign_mode){arena.p[0].x=110;arena.p[1].x=145;arena.map_id=CAMPAIGN_MAP;campaign=(Campaign){0};campaign.owned_weapons[0]=campaign.owned_weapons[1]=weapon_set_low(1);campaign.profiles[0]=(CampaignProfile){.level=1,.knight=knight_selected};campaign.profiles[1]=(CampaignProfile){.level=1};}
    if(campaign_mode&&host)load_character(0,(unsigned)knight_selected);
    if(host)arena_select_knight(&arena,0,(unsigned)knight_selected,1);
    int irq=enterCriticalSection();protocol_remote_reset(&remote);pending=0;input_age=state_age=0;
    state_seen=0;last_state_sequence=0;guest_known=0;guest_aid=0;local_packet=(InputPacket){.magic=PROTOCOL_MAGIC};leaveCriticalSection(irq);
    deadline=frames+600;
    Wifi_MultiplayerFromClientSetPacketHandler(from_client);Wifi_MultiplayerFromHostSetPacketHandler(from_host);
    int rc=host?Wifi_MultiplayerHostMode(1,sizeof(StatePacket),sizeof(InputPacket)):Wifi_MultiplayerClientMode(sizeof(InputPacket));
    if(rc<0)fail("Radio mode failed");else mode=host?HOST_SETUP:SCAN_SETUP;
}
/* Bank changes happen while the previous complete image remains visible.
   A whole chapter stays resident: no SD reads during its rooms or waves. */
static void line(int row,const char *format,...);
/* Loading screen on the upper display while a level's art is read from the card; the lower screen keeps its panel. */
static int force_redraw;
static void loading_progress(unsigned done,unsigned total){
    static unsigned shown=1000;
    unsigned percent=total?(unsigned)((uint64_t)done*100/total):100;
    if(done&&done<total&&percent<shown+4&&shown!=1000)return;
    shown=percent;
    menu_render_loading(pixels,percent);
    int base=front_base==16?10:16,page=base==10?0:1;
    uint16_t *destination=(uint16_t *)(0x06000000+base*16384);
    DC_FlushRange(pixels,sizeof(pixels));dmaCopy(pixels,destination,sizeof(pixels));
    for(int y=0;y<192;y++){page_left[page][y]=0;page_right[page][y]=256;}
    pending_base=base;pending_terrain=0;pending_refresh=display_refresh;top_pending=1;force_redraw=1;
    swiWaitForVBlank();present();
    if(done>=total){shown=1000;memset(pixels,0,sizeof(pixels));} /* the overlay starts clear again */
}
static int prepare_chapter(unsigned bank){
    if(chapter_assets_ready(bank)){
        if(mode==CLIENT&&campaign_mode&&arena.running&&!request_waiting&&chapter_ready_sent!=campaign_scene_stamp()){
            request_send(CAMPAIGN_REQUEST_LOADED,bank);request_out.reserved0[0]=arena.map_id;request_out.reserved0[1]=campaign.phase==CAMPAIGN_WORLD_MAP?CAMPAIGN_WORLD_MAP:0;chapter_ready_sent=campaign_scene_stamp();
        }
        return 1;
    }
    if(!audio_assets_available())save_init();
    if(!audio_assets_available()){fail("Game data unavailable");return 0;}
    audio_set_mode(AUDIO_SILENT);
    chapter_assets_set_progress(loading_progress);
    if(!chapter_assets_load(bank)){fail("Could not load game data");return 0;}
    arena_render_invalidate_assets();uploaded_map=-1;
    return prepare_chapter(bank);
}
static int lobby_screen(void){return mode==KNIGHT_SELECT||(mode==CLIENT&&(!client_confirmed||!arena.running))||(mode==HOST&&!arena.running);}
static void draw(uint32_t target_refresh){
    static int last_mode=-1,last_choice=-1,last_movie_frame=-1;
    if(mode==ARENA_SELECT)prepare_chapter(0);
    else if((mode==PRACTICE||mode==HOST||mode==CLIENT)&&!lobby_screen()) /* The lobby needs no chapter bank; loading one would keep silencing the menu music. */
        prepare_chapter(campaign_mode?(campaign.phase==CAMPAIGN_WORLD_MAP?3:arena.map_id==15?8:arena.map_id==14?7:arena.map_id==13?6:arena.map_id==12?5:arena.map_id==11?4:arena.map_id>=8?3:arena.map_id==6||arena.map_id==7?2:1):0);
    int worldmap=campaign_mode&&arena.running&&campaign.phase==CAMPAIGN_WORLD_MAP;
    int movie=(mode==PRACTICE||mode==HOST||mode==CLIENT)&&campaign_mode&&arena.running&&(campaign.phase==KEEP_INTRO||campaign.phase==KEEP_ABDUCTION);
    int picker=lobby_screen();
    int screen_mode=worldmap?2000:movie?1000+campaign.phase:picker?KNIGHT_SELECT:mode;
    int choice=worldmap?campaign.group:picker?knight_selected*1000+opponent_selected*100000+(mode==HOST?host_ready*4096+(guest_known?1:0)+(int)(remote.seen&&!arena.p[1].ready?remote.knight_id:arena.p[1].knight_id)*2+(arena.p[1].ready?16:0)+(int)campaign.profiles[1].level*32:0)+(mode==CLIENT?(arena.countdown&&!arena.running?4096:0)+(arena.p[0].ready?1:0)+(int)arena.p[0].knight_id*2+client_confirmed*16:0):mode==ARENA_SELECT?map_selected:mode==MENU?menu_selected:mode==CAMPAIGN_MENU?campaign_selected:multi_selected;
    int static_screen=worldmap||movie||picker||mode==MENU||mode==CAMPAIGN_MENU||mode==CONTROLS||mode==MULTIPLAYER||mode==ARENA_SELECT||mode==SCAN||mode==CONNECT||mode==HOST_SETUP||mode==SCAN_SETUP||mode==ERROR_SCREEN;
    int movie_frame=movie?(int)cinematic_frame(campaign.phase==KEEP_INTRO?CINEMATIC_CRYSTAL:CINEMATIC_PRINCESSES,campaign.timer):-1;
    if(static_screen&&!worldmap&&!force_redraw&&last_mode==screen_mode&&last_choice==choice&&(!movie||last_movie_frame==movie_frame))return;
    force_redraw=0;
    pending_refresh=target_refresh;
    last_movie_frame=movie_frame;
    int changed_mode=last_mode!=screen_mode;
    last_mode=screen_mode;last_choice=choice;
    pending_terrain=!static_screen;
    if(worldmap)arena_render_world_map(pixels,&campaign,&arena);
    else if(movie){arena_render_movie(pixels,arena.map_id,campaign.phase==KEEP_INTRO?CINEMATIC_CRYSTAL:CINEMATIC_PRINCESSES,campaign.timer);render_count++;}
    else if(picker){
        ArenaLobby lobby={.own=mode==CLIENT?1:0,.room=mode==CLIENT||mode==HOST||map_for_host,.knight={-1,-1},.title=mode==KNIGHT_SELECT?"CHARACTER SELECT":"WAITING ROOM"};
        unsigned mine=(unsigned)knight_selected,own_level=campaign_mode||map_for_host?campaign_save_character(mine)->profile.level:1;
        if(mode==CLIENT){
            lobby.knight[1]=knight_selected;lobby.level[1]=own_level?own_level:1;lobby.ready[1]=(uint8_t)client_confirmed;
            if(arena.p[0].ready){lobby.knight[0]=(int)arena.p[0].knight_id;lobby.level[0]=campaign_mode?campaign.profiles[0].level:0;lobby.ready[0]=arena.countdown!=0&&!arena.running;}
            lobby.hint=client_confirmed?"Ready - waiting for host to start":"Up/Down knight   A ready   B leave";
        }else{
            lobby.knight[0]=knight_selected;lobby.level[0]=own_level?own_level:1;lobby.ready[0]=mode==HOST?(uint8_t)host_ready:0;
            if(mode==HOST&&guest_known){lobby.knight[1]=(int)(remote.seen&&!arena.p[1].ready?remote.knight_id:arena.p[1].knight_id);lobby.level[1]=campaign_mode&&guest_profile_ready?campaign.profiles[1].level:0;lobby.ready[1]=arena.p[1].ready;}
            if(mode==KNIGHT_SELECT&&!map_for_host&&!campaign_mode){lobby.knight[1]=opponent_selected;lobby.level[1]=1;lobby.ready[0]=lobby.ready[1]=1;}
            lobby.hint=mode==HOST?(arena.p[1].ready?"A: ready   START: begin":"Waiting for player 2..."):map_for_host?"A: create room   B: back":(mode==KNIGHT_SELECT&&!map_for_host&&!campaign_mode)?"Up/Down you  Left/Right CPU  A start":"Up/Down knight   A start   B back";
        }
        arena_render_lobby(pixels,&lobby);
    }
    else if(mode==ARENA_SELECT){
        /* The preview samples the arena terrain surface; make sure it is bound and holds the highlighted map. */
        if(uploaded_map!=(int)map_selected){arena_upload_background(bgGetGfxPtr(terrain),BG_PALETTE,map_selected);uploaded_map=(int)map_selected;}
        arena_render_selection(pixels,map_selected);
    }
    else if(mode==CONTROLS)menu_render_controls(pixels);
    else if(mode==CAMPAIGN_MENU)menu_render_campaign(pixels,campaign_selected);
    else if(mode==MENU||mode==ERROR_SCREEN)menu_render(pixels,menu_selected);
    else if(mode==MULTIPLAYER||mode==SCAN||mode==CONNECT||mode==HOST_SETUP||mode==SCAN_SETUP)menu_render_multiplayer(pixels,multi_selected);
    else {
        if(changed_mode)memset(pixels,0,sizeof(pixels));
        if(uploaded_map!=arena.map_id){arena_upload_background(bgGetGfxPtr(terrain),BG_PALETTE,arena.map_id);uploaded_map=arena.map_id;}
        arena_render_campaign(campaign_mode?&campaign:NULL);
        arena_render(&arena,pixels);render_count++;
        pending_terrain=!arena_render_is_opaque();
        pending_camera=*arena_render_background_camera();
        /* The displayed opaque page hides BG3. Prepare its terrain before
           re-enabling it, so the handoff needs only a register/page flip. */
        if(pending_terrain&&!front_has_terrain)arena_present_background();
    }
    pending_base=front_base==16?10:16;
    int page=pending_base==10?0:1;
    uint16_t *destination=(uint16_t *)(0x06000000+pending_base*16384);
    if(static_screen){
        DC_FlushRange(pixels,sizeof(pixels));dmaCopy(pixels,destination,sizeof(pixels));
        for(int y=0;y<192;y++){page_left[page][y]=0;page_right[page][y]=256;}
    }else{
        const uint16_t *left=arena_dirty_left(),*right=arena_dirty_right();
        OverlayCopy run={0,0};
        for(int y=0;y<192;y++){
            int lo=left[y]<page_left[page][y]?left[y]:page_left[page][y];
            int hi=right[y]>page_right[page][y]?right[y]:page_right[page][y];
            lo&=~1;hi=(hi+1)&~1;
            if(hi>lo){
                OverlayCopy done=overlay_copy_add(&run,(unsigned)y*512+lo*2,(unsigned)y*512+hi*2);
                if(done.end){DC_FlushRange((uint8_t *)pixels+done.first,done.end-done.first);dmaCopy((uint8_t *)pixels+done.first,(uint8_t *)destination+done.first,done.end-done.first);}
            }
            page_left[page][y]=left[y];page_right[page][y]=right[y];
        }
        if(run.end){DC_FlushRange((uint8_t *)pixels+run.first,run.end-run.first);dmaCopy((uint8_t *)pixels+run.first,(uint8_t *)destination+run.first,run.end-run.first);}
    }
    top_pending=1;
}
static void line(int row,const char *format,...){
    char text[80];va_list ap;va_start(ap,format);vsnprintf(text,sizeof(text),format,ap);va_end(ap);
    ui_skin_text(bottom,10,row==11?179:8+row*15,236,text,row==0?UI_GOLD:row>=2&&row<=10?UI_INK:UI_LIGHT);
}
static int ui_near_weapon(void){
    if(!campaign_mode)return 0;
    const Fighter *p=&arena.p[mode==CLIENT?1:0];
    for(unsigned i=0;i<CAMPAIGN_DROPS;i++)if(campaign.drops[i].kind==DROP_WEAPON&&abs(p->x-campaign.drops[i].x)<=28&&abs(p->y-campaign.drops[i].y)<=22)return 1+campaign.drops[i].value;
    return 0;
}
static void text_ui(void){
    static int previous[89],seen;
    int picker=lobby_screen();
    int state[89]={mode,paused,arena.p[0].hp,arena.p[1].hp,arena.p[0].mana,arena.p[1].mana,
        arena.wins[0],arena.wins[1],arena.round,arena.running,arena.winner,(arena.countdown+29)/30,
        measured_fps,measured_work_ms,audio_enabled(),room_count,selected,
        mode==HOST?Wifi_MultiplayerGetNumClients():0,menu_selected,multi_selected,
        arena.clash_timer,arena.clash_score[0],arena.clash_score[1],arena.p[0].pose==POSE_ROLL,map_selected,arena.map_id,arena.p[mode==CLIENT?1:0].bow_equipped,
        knight_selected,client_confirmed,arena.p[0].knight_id,arena.p[1].knight_id,arena.p[0].ready,arena.p[1].ready,practice_difficulty,campaign_mode,campaign_selected,(int)campaign.kills,campaign.phase,campaign.group,(int)campaign.profiles[0].xp,(int)campaign.profiles[1].xp,campaign.profiles[0].gold,campaign.profiles[1].gold,campaign.profiles[0].level,campaign.profiles[1].level,campaign.timer/15,campaign.revive[0]+campaign.revive[1],campaign.pickup_timer[0]/3,campaign.pickup_timer[1]/3,campaign.potions[0],campaign.potions[1],campaign.story_flags,campaign.weapons[0],campaign.weapons[1],(int)(campaign.owned_weapons[0].w[0]^campaign.owned_weapons[0].w[1]^campaign.owned_weapons[0].w[2]^campaign.owned_weapons[0].w[3]),(int)(campaign.owned_weapons[1].w[0]^campaign.owned_weapons[1].w[1]^campaign.owned_weapons[1].w[2]^campaign.owned_weapons[1].w[3]),campaign_panel,panel_selected,request_waiting,save_status,campaign.stats[0][0],campaign.stats[0][1],campaign.stats[0][2],campaign.stats[0][3],campaign.stats[1][0],campaign.stats[1][1],campaign.stats[1][2],campaign.stats[1][3],guest_profile_ready,audio_assets_available(),campaign.tools[0],campaign.tools[1],campaign.shovel_equipped[0],campaign.shovel_equipped[1],campaign_peer_loading,pause_selected,campaign.paused,paused,campaign.war.machine.hp,campaign_feedback.notice[0]/6,campaign_feedback.notice[1]/6,campaign_feedback.combo[0],campaign_feedback.combo[1],campaign_save_dirty(),ui_near_weapon(),(int)item_cursor,campaign.war.gate_hp,(int)campaign_boss_buried_frame(&campaign,0),(int)campaign_boss_buried_frame(&campaign,1)};
    if(seen&&mode!=SCAN&&!memcmp(previous,state,sizeof(state)))return;
    memcpy(previous,state,sizeof(state));seen=1;
    if(!picker&&(mode==PRACTICE||mode==HOST||mode==CLIENT)){
        bottom_ui_base(bottom,&arena,campaign_mode?&campaign:NULL);
    }
    else {ui_skin_background(bottom);ui_skin_panel(bottom,4,33,248,140);line(0,mode==CONTROLS?"COMBAT CONTROLS":"CASTLE CRASHERS");}
    if(campaign_panel&&campaign_mode&&arena.running){campaign_menu_draw(bottom,&campaign,mode==CLIENT?1:0,campaign_panel,panel_selected,request_waiting,mode!=PRACTICE);bottom_pending=1;return;}
    if(paused){
        ui_skin_background(bottom);ui_skin_panel(bottom,4,33,248,140);
        line(0,mode==CLIENT?"PLAYER MENU":"PAUSE MENU");
        line(2,mode==CLIENT?"Host controls the pause":mode==HOST&&!campaign_mode?"Arena room keeps playing":"Game paused");
        const char *choices[]={"Resume","World map","Sound","Save and quit"};
        for(int i=0;i<4;i++){
            const char *label=choices[i];
            if(i==1&&(!campaign_mode||mode==CLIENT))label="Return to game";
            if(i==3&&!campaign_mode)label="Quit to menu";
            if(i==2)line(4+i,"%c Sound: %s",pause_selected==i?'>':' ',audio_enabled()?"ON":"OFF");
            else line(4+i,"%c %s",pause_selected==i?'>':' ',label);
        }
        line(9,"A: select  B / START: resume");
        if(mode==HOST)line(10,"Quit closes this room");
        else if(mode==CLIENT)line(10,"Quit leaves this room");
        bottom_pending=1;return;
    }
    if(picker&&mode==HOST){line(2,"WAITING ROOM");line(4,"Players: %d / 2",1+(arena.p[1].ready?1:0));line(6,arena.p[1].ready?"Player 2 is ready":guest_known?"Player 2 is choosing...":"Waiting for player 2...");line(7,"A: %s",host_ready?"Ready (press to undo)":"Ready up");line(8,host_ready&&arena.p[1].ready?"START: begin the game":"START needs both ready");line(10,"SELECT: close room");}
    else if(picker&&mode==CLIENT&&client_confirmed){line(2,"WAITING ROOM");line(4,"You are ready");line(6,"The host starts the game");line(10,"SELECT: leave room");}
    else if(picker){line(2,"CHOOSE YOUR KNIGHT");line(4,"%s",knight_name((unsigned)knight_selected));line(6,(mode==KNIGHT_SELECT&&!map_for_host&&!campaign_mode)?"Up/Down you, Left/Right CPU":"Up / Down: Swap knight");if(mode==KNIGHT_SELECT&&!map_for_host&&!campaign_mode){line(3,"Difficulty: %s",bot_difficulty_name(practice_difficulty));line(7,"L / R: Easy / Medium / Hard");}line(8,mode==CLIENT?"A: Ready in this room":map_for_host?"A: Create room":campaign_mode?"A: Start campaign":"A: Start practice");line(10,mode==CLIENT?"B: Leave room":"B: Back");if(campaign_mode){const CampaignCharacter *p=campaign_save_character((unsigned)knight_selected);line(9,"Saved LV%u / Gold%u",p->profile.level,p->profile.gold);if(request_waiting)line(8,"Sending character to host...");}}
    else if(mode==CAMPAIGN_MENU){line(2,"CAMPAIGN");line(4,"Solo or two DS players");line(6,"Up/Down: Choose   A: Select");line(8,"Local wireless, no internet");line(9,save_status==3?"SD save unavailable":save_status==4?"Save failed - retry on exit":save_status?"Progress saves to SD":"Choose a mode to load saves");line(10,"B: Back");}
    else if(mode==MENU){line(2,"Up/Down chooses. A confirms.");line(4,"Campaign, practice or arena");line(5,"Choose a mode to begin.");line(7,"START: sound %s",audio_enabled()?"ON":"OFF");line(10,"v0.40 Boss movement");}
    else if(mode==ARENA_SELECT){line(2,"CHOOSE ARENA");line(4,"%s",stage_name(map_selected));line(6,"Left / Right: Change arena");line(8,"A: Choose knight");line(10,"B: Back");}
    else if(mode==MULTIPLAYER){line(2,"MULTIPLAYER");line(4,"Host Room on one DS.");line(5,"Join Room on the other DS.");line(7,"Both run this same ROM.");line(8,"No router or internet needed.");line(11,"A select   B back");}
    else if(mode==CONTROLS){line(1,"B: Jump  B,B: Spin (Lv2)");line(2,"Hold D-pad: run  A:Light X:Heavy");line(3,"R + D-pad: Shield walk");line(4,"L+A: Bolt (air: downward)");line(5,"L + X: Magic splash");line(6,"Y: Use item / No item");line(7,"A A A A / X X X: Combos");line(8,"A then X: Uppercut (Lv8)");line(9,"Run+A/X: Dash attacks (Lv4)");line(10,"Near downed + X: Stomp");line(11,"A / B: Back");}
    else if(mode==SCAN){
        line(1,"Nearby rooms: %d",room_count);
        for(int i=0;i<room_count;i++)line(2+i,"%c %02X%02X  %.17s",i==selected?'>':' ',rooms[i].bssid[4],rooms[i].bssid[5],rooms[i].ssid);
        line(11,"Up/Down   A join   B back");
    }else if(mode==ERROR_SCREEN){line(3,"%s",error);line(10,"SELECT returns to menu");}
    else if(mode==HOST_SETUP||mode==SCAN_SETUP||mode==CONNECT){line(3,"Preparing local wireless...");line(10,"SELECT cancels");}
    else {
        BottomUi ui={.item_cursor=item_cursor,.arena=&arena,.campaign=campaign_mode?&campaign:NULL,.feedback=&campaign_feedback,
            .hero=mode==CLIENT?1:0,.fps=measured_fps,.work_ms=measured_work_ms,.save_status=save_status,
            .host=mode==HOST,.client=mode==CLIENT,.practice=mode==PRACTICE,.difficulty=practice_difficulty,
            .save_dirty=campaign_save_dirty(),.audio_ok=audio_assets_available(),.peer_loading=campaign_peer_loading,.guest_known=guest_known};
        bottom_ui_game(bottom,&ui);
    }
    if(campaign_mode&&arena.running&&!picker&&!paused&&(mode==HOST||mode==CLIENT||mode==PRACTICE)){
        unsigned hero=mode==CLIENT?1:0;
        if(campaign_feedback.notice[hero])hud_render_level_notice(bottom,campaign.profiles[hero].level,campaign_feedback_combo_controls(campaign_feedback.combo[hero]),campaign_feedback.notice[hero]);
    }
    bottom_pending=1;
}
static void boot_stage(const char *stage){
    draw(display_refresh);
    ui_skin_background(bottom);ui_skin_panel(bottom,4,33,248,140);
    line(0,"CASTLE CRASHERS");line(3,"%s",stage);
    line(5,"Mode: %s",isDSiMode()?"DSi":"DS");
    line(9,"Startup check build");
#ifdef BOOT_DIAGNOSTIC
    line(10,"Press A to continue");
#endif
    bottom_pending=1;swiWaitForVBlank();present();
#ifdef BOOT_DIAGNOSTIC
    boot_continue();
#endif
}
int main(void){
#ifdef BOOT_DIAGNOSTIC
    powerOn(POWER_ALL_2D);setBrightness(3,0);
    consoleDemoInit();
    printf("Castle Arena boot check\n\n1: Game entry reached\nMode: %s\n\nPress A to test VBlank\n",isDSiMode()?"DSi":"DS");
#ifdef BOOT_FTL_CORE
    printf("\nFTL core / wireless disabled\n");
#endif
    boot_continue();
    swiWaitForVBlank();
    printf("\n2: VBlank working\nPress A to load graphics\n");
    boot_continue();
#endif
    defaultExceptionHandler();
    powerOn(POWER_ALL_2D);setBrightness(3,0);
    videoSetMode(MODE_5_2D);videoSetModeSub(MODE_5_2D);vramSetBankA(VRAM_A_MAIN_BG);vramSetBankB(VRAM_B_MAIN_BG);vramSetBankD(VRAM_D_MAIN_BG_0x06040000);vramSetBankC(VRAM_C_SUB_BG);
    bg=bgInitHidden(2,BgType_Bmp16,BgSize_B16_256x256,16,0);
    terrain=bgInitHidden(3,BgType_Bmp8,BgSize_B8_512x512,0,0);bgSetPriority(terrain,3);bgSetPriority(bg,2);
    /* Continuous terrain uses a 512-pixel ring: affine wrapping is required. */
    bgWrapOn(terrain);
    dmaFillHalfWords(0,(void *)0x06028000,sizeof(pixels));
    dmaFillHalfWords(0,(void *)0x06040000,sizeof(pixels));
    sub=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
    lcdMainOnTop();arena_init(&arena);
    boot_stage("Starting audio...");audio_init();
    boot_stage("Starting music...");audio_set_mode(AUDIO_MENU);
    boot_stage("Waiting for display...");
    timerStart(0,ClockDivider_1024,0,NULL);
    uint16_t perf_previous=timerTick(0),latched=0;int last_ui_mode=-1;
    unsigned perf_elapsed=0,work_peak=0;
    irqSet(IRQ_VBLANK,display_vblank);irqEnable(IRQ_VBLANK);
    uint32_t last_game_refresh=display_refresh;
    while(1){
        swiWaitForVBlank();uint16_t work_begin=timerTick(0);present();scanKeys();frames++;
        if(frames==30&&!audio_assets_available())audio_prepare_assets(); /* The main menu is on screen: mount NitroFS and start its music. */
        int game_due=frame_clock_due(display_refresh,&last_game_refresh);int down=keysDown(),held=keysHeld();latched|=controls(down);
        uint16_t touch_command=0;
        if((down&KEY_TOUCH)&&(mode==PRACTICE||mode==HOST||mode==CLIENT)&&arena.running&&(!arena.winner||campaign_panel)&&!paused){
            touchPosition touch;touchRead(&touch);
            if(campaign_mode&&campaign_panel){int hit=campaign_menu_hit(touch.px,touch.py);if(hit==6){campaign_panel=0;save_capture(mode==PRACTICE);}else if(hit>=0){panel_selected=hit;panel_action((unsigned)hit);} }
            else if(campaign_mode&&campaign.phase!=KEEP_BARRACKS_INTRO&&campaign.phase!=KEEP_INTRO&&campaign.phase!=KEEP_ABDUCTION&&touch.py>=118&&touch.py<170&&(touch.px>=170&&touch.px<222)){campaign_panel=2;panel_selected=0;latched=0;}
            else if(campaign_mode&&item_selector_hit(touch.px,touch.py)>=0){
                unsigned h=mode==CLIENT?1:0,mask=campaign_item_mask(&campaign,h);
                int hit=item_selector_hit(touch.px,touch.py);item_cursor=item_selector_current(mask,item_cursor);
                if(hit){item_cursor=item_selector_move(mask,item_cursor,hit==1?-1:1);item_use_pending=0;}
                else if(item_cursor)item_use_pending=item_cursor;
            }
            else if(campaign_mode&&campaign.phase==CAMPAIGN_WORLD_MAP&&mode!=CLIENT&&!campaign.paused){
                int hit=bottom_ui_map_hit(touch.px,touch.py);
                if(hit>=0)down|=hit==0?KEY_LEFT:hit==2?KEY_RIGHT:KEY_A;
            }
            else if(!campaign_mode&&hud_item_hit(touch.px,touch.py))touch_command=EQUIP;
        }
        /* Equip the visible item before dispatching a tap-use. Guests wait for
           the authoritative snapshot/ack, so a delayed switch cannot use the old item. */
        if(campaign_mode&&arena.running&&!arena.winner&&(mode==PRACTICE||mode==HOST||mode==CLIENT)&&!paused&&!campaign.paused&&!campaign_panel){
            unsigned h=mode==CLIENT?1:0,mask=campaign_item_mask(&campaign,h);
            item_cursor=item_selector_current(mask,item_cursor);
            unsigned equipped=campaign_equipped_item(&campaign,h);
            if(!arena.p[h].hp||!item_cursor||(item_use_pending&&!(mask&(1u<<item_use_pending))))item_use_pending=0;
            if(item_cursor&&arena.p[h].hp&&equipped!=item_cursor){
                if(mode==CLIENT){if(!request_waiting)request_send(CAMPAIGN_REQUEST_ITEM,item_cursor);}
                else if(campaign_select_item(&campaign,h,item_cursor)){arena.p[h].equip_timer=30;equipped=item_cursor;}
            }
            if(item_use_pending&&item_use_pending==item_cursor&&equipped==item_cursor&&(mode!=CLIENT||!request_waiting)){touch_command|=BOW|BOW_EDGE;item_use_pending=0;}
        }else item_use_pending=0;
        latched|=touch_command;
        int panel_block=campaign_panel;
        if(campaign_panel){
            if(down&KEY_UP)panel_selected=(panel_selected+3)%4;
            if(down&KEY_DOWN)panel_selected=(panel_selected+1)%4;
            if(down&KEY_A)panel_action((unsigned)panel_selected);
            if(down&(KEY_B|KEY_START)){campaign_panel=0;save_capture(mode==PRACTICE);}
            down&=KEY_SELECT;latched=0;held=0;touch_command=0;
        }
        /* Menus never suspend radio servicing. Only the host pauses shared
           campaign simulation; guests can always open their local menu. */
        int playing=(mode==PRACTICE||mode==HOST||mode==CLIENT)&&arena.running&&!arena.winner;
        if(playing&&!panel_block&&(down&KEY_START)){
            paused=!paused;pause_selected=0;
            if(campaign_mode&&mode!=CLIENT)campaign.paused=paused;
            latched=0;down=0;held=0;
        }
        if(paused&&playing){
            if(down&KEY_UP)pause_selected=(pause_selected+3)%4;
            if(down&KEY_DOWN)pause_selected=(pause_selected+1)%4;
            if(down&KEY_B){paused=0;if(campaign_mode&&mode!=CLIENT)campaign.paused=0;}
            if(down&KEY_A){
                if(pause_selected==2)audio_toggle();
                else if(pause_selected==3){leave();}
                else{
                    if(pause_selected==1&&campaign_mode&&mode!=CLIENT){save_capture(1);campaign_world_map(&campaign,&arena);campaign_sync_transition();}
                    paused=0;if(campaign_mode&&mode!=CLIENT)campaign.paused=0;
                }
            }
            latched=0;down=0;held=0;touch_command=0;
        }
        {   /* Original menu cues: toggle for moving, select for confirming. */
            int menu_screen=mode==MENU||mode==CAMPAIGN_MENU||mode==MULTIPLAYER||mode==ARENA_SELECT||mode==KNIGHT_SELECT||mode==CONTROLS||mode==SCAN||lobby_screen();
            if(menu_screen&&!campaign_panel){
                if(down&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT))audio_menu_sound(0);
                else if(down&KEY_A)audio_menu_sound(1);
                else if(down&KEY_B)audio_menu_sound(2);
            }
        }
        if(down&KEY_SELECT){leave();latched=0;down=0;}
        if(mode==MENU||mode==MULTIPLAYER||mode==CAMPAIGN_MENU){
            int *choice=mode==MENU?&menu_selected:mode==CAMPAIGN_MENU?&campaign_selected:&multi_selected;
            int choices=mode==MENU?4:3;
            if(down&KEY_DOWN)*choice=(*choice+1)%choices;
            if(down&KEY_UP)*choice=(*choice+choices-1)%choices;
            if(mode==MENU&&(down&KEY_START))audio_toggle();
            if(down&KEY_B)mode=MENU;
            if(down&KEY_A){
                if(mode==MENU){campaign_mode=0;if(map_selected>=STAGE_ARENA_COUNT)map_selected=0;if(menu_selected==0)mode=CAMPAIGN_MENU;else if(menu_selected==1){map_for_host=0;mode=ARENA_SELECT;}else mode=menu_selected==2?MULTIPLAYER:CONTROLS;}
                else if(mode==CAMPAIGN_MENU){save_init();campaign_mode=1;map_selected=CAMPAIGN_MAP;map_for_host=campaign_selected==1;if(campaign_selected==2)radio_start(0);else mode=KNIGHT_SELECT;}
                else if(multi_selected==2)mode=MENU;else if(multi_selected==0){map_for_host=1;mode=ARENA_SELECT;}else radio_start(0);
                latched=0;
            }
        }else if(mode==ARENA_SELECT){
            if(down&(KEY_RIGHT|KEY_DOWN))map_selected=(map_selected+1)%STAGE_ARENA_COUNT;
            if(down&(KEY_LEFT|KEY_UP))map_selected=(map_selected+STAGE_ARENA_COUNT-1)%STAGE_ARENA_COUNT;
            if(down&KEY_B)mode=map_for_host?MULTIPLAYER:MENU;
            else if(down&KEY_A){
                mode=KNIGHT_SELECT;
                if(!map_for_host&&!campaign_mode)opponent_selected=(int)((frames*2654435761u>>13)%KNIGHT_COUNT); /* Practice: a different computer knight every time */
                latched=0;
            }
        }else if(mode==KNIGHT_SELECT){
            if(!map_for_host&&!campaign_mode){
                if(down&KEY_R)practice_difficulty=(practice_difficulty+1)%BOT_COUNT;
                if(down&KEY_L)practice_difficulty=(practice_difficulty+BOT_COUNT-1)%BOT_COUNT;
            }
            if(!map_for_host&&!campaign_mode){ /* Practice: up/down picks your knight, left/right the computer's. */
                if(down&KEY_DOWN)knight_selected=(knight_selected+1)%KNIGHT_COUNT;
                if(down&KEY_UP)knight_selected=(knight_selected+KNIGHT_COUNT-1)%KNIGHT_COUNT;
                if(down&KEY_RIGHT)opponent_selected=(opponent_selected+1)%KNIGHT_COUNT;
                if(down&KEY_LEFT)opponent_selected=(opponent_selected+KNIGHT_COUNT-1)%KNIGHT_COUNT;
            }else{
                if(down&(KEY_RIGHT|KEY_DOWN))knight_selected=(knight_selected+1)%KNIGHT_COUNT;
                if(down&(KEY_LEFT|KEY_UP))knight_selected=(knight_selected+KNIGHT_COUNT-1)%KNIGHT_COUNT;
            }
            if(down&KEY_B)mode=campaign_mode?CAMPAIGN_MENU:ARENA_SELECT;
            else if(down&KEY_A){
                if(map_for_host)radio_start(1);
                else{
                    arena_init(&arena);arena.map_id=map_selected;stage_set_map(arena.map_id);
                    arena_select_knight(&arena,0,(unsigned)knight_selected,1);
                    arena_select_knight(&arena,1,(unsigned)(campaign_mode?(knight_selected+1)%KNIGHT_COUNT:opponent_selected),1);
                    if(campaign_mode){local_hero=0;load_character(0,(unsigned)knight_selected);campaign_start(&campaign,&arena,0);campaign_world_map(&campaign,&arena);}else arena_start_round(&arena);paused=0;campaign_panel=0;mode=PRACTICE;
                }
                latched=0;
            }
        }else if(mode==CONTROLS){if(down&(KEY_A|KEY_B))mode=MENU;
        }else if(mode==HOST_SETUP||mode==SCAN_SETUP){
            if(Wifi_LibraryModeReady()){
                if(mode==HOST_SETUP){Wifi_SetChannel(6);Wifi_MultiplayerAllowNewClients(true);if(Wifi_BeaconStart(campaign_mode?"Castle Campaign v40":"Castle Arena v40",campaign_mode?CAMPAIGN_GAME_ID:GAME_ID)<0)fail("Room creation failed");else{mode=HOST;host_ready=0;arena.countdown=0;}}
                else{Wifi_ScanMode();room_count=selected=0;mode=SCAN;}
            }else if((int32_t)(frames-deadline)>0)fail("Radio setup timed out");
        }else if(mode==SCAN){
            if(down&KEY_B){Wifi_IdleMode();mode=campaign_mode?CAMPAIGN_MENU:MULTIPLAYER;}
            if(frames%30==0){room_count=0;for(int i=0;i<Wifi_GetNumAP()&&room_count<8;i++){
                Wifi_AccessPoint ap;if(Wifi_GetAPData(i,&ap)==WIFI_RETURN_OK&&ap.nintendo.game_id==(campaign_mode?CAMPAIGN_GAME_ID:GAME_ID)&&ap.nintendo.allows_connections&&ap.rssi>0)rooms[room_count++]=ap;
            }if(selected>=room_count)selected=0;}
            if(room_count){if(down&KEY_DOWN)selected=(selected+1)%room_count;if(down&KEY_UP)selected=(selected+room_count-1)%room_count;
                if(down&KEY_A){if(Wifi_ConnectOpenAP(&rooms[selected])<0)fail("Could not join room");else{mode=CONNECT;deadline=frames+600;}}}
        }else if(mode==CONNECT){
            int status=Wifi_AssocStatus();if(status==ASSOCSTATUS_ASSOCIATED){mode=CLIENT;state_age=0;client_confirmed=0;latched=0;}
            else if(status==ASSOCSTATUS_CANNOTCONNECT||(int32_t)(frames-deadline)>0)fail("Connection failed / timeout");
        }else if(mode==CLIENT){
            if(client_transition_grace)client_transition_grace--;
            if(Wifi_AssocStatus()!=ASSOCSTATUS_ASSOCIATED||++state_age>(client_transition_grace?1800:180))fail("Host connection lost");
            else{
                int irq=enterCriticalSection();if(pending){if(campaign_mode)campaign_unpack(&campaign_inbox,&campaign,&arena);else arena=inbox.arena;pending=0;}leaveCriticalSection(irq);
                if(campaign_mode&&arena.running&&client_transition_stamp!=campaign_scene_stamp()){client_transition_stamp=campaign_scene_stamp();client_transition_grace=1800;}
                if(request_waiting&&campaign.menu_ack[1]==(request_out.sequence&15))request_waiting=0;
                if(!client_confirmed){
                    if(down&(KEY_RIGHT|KEY_DOWN))knight_selected=(knight_selected+1)%KNIGHT_COUNT;
                    if(down&(KEY_LEFT|KEY_UP))knight_selected=(knight_selected+KNIGHT_COUNT-1)%KNIGHT_COUNT;
                    if(down&KEY_B){int was_campaign=campaign_mode;leave();mode=was_campaign?CAMPAIGN_MENU:MULTIPLAYER;latched=0;}
                    else if(down&KEY_A){client_confirmed=1;if(campaign_mode)request_send(CAMPAIGN_REQUEST_PROFILE,0);latched=0;down=0;touch_command=0;}
                }
                /* Stay in CLIENT throughout selection so host packets and replies continue.
                   No menu confirmation press can become a buffered combat action. */
                int can_play=mode==CLIENT&&client_confirmed&&arena.running&&!panel_block&&!paused&&(!campaign_mode||!campaign.paused);
                uint16_t pressed=can_play?(controls(down)|touch_command):0;
                local_packet.keys=can_play?controls(held):0;local_packet.sequence++;
                local_packet.knight_id=(uint16_t)knight_selected;
                local_packet.confirmed=(uint16_t)(mode==CLIENT&&client_confirmed&&(!campaign_mode||!request_waiting||request_out.operation!=CAMPAIGN_REQUEST_PROFILE));
                if(pressed&ATTACK)local_packet.attack_count++;
                if(pressed&JUMP)local_packet.jump_count++;
                if(pressed&HEAVY)local_packet.heavy_count++;
                if(pressed&MAGIC)local_packet.magic_count++;
                if(pressed&BOW)local_packet.bow_count++;
                if(pressed&EQUIP)local_packet.equip_count++;
                if(mode==CLIENT){if(request_waiting&&(frames&1))Wifi_MultiplayerClientReplyTxFrame(&request_out,sizeof(request_out));else Wifi_MultiplayerClientReplyTxFrame(&local_packet,sizeof(local_packet));}
            }
        }else if(mode==HOST||mode==PRACTICE){
            if(mode==HOST){
                campaign_sync_transition();
                CampaignRequest command;int has_request;int command_irq=enterCriticalSection();has_request=request_pending;command=request_in;request_pending=0;leaveCriticalSection(command_irq);
                if(has_request&&campaign_request_apply(&request_state,&command,&campaign,&arena)){
                    if(command.operation==CAMPAIGN_REQUEST_PROFILE)guest_profile_ready=1;
                    if(command.operation==CAMPAIGN_REQUEST_LOADED&&(((int)command.reserved0[0]<<8)|command.reserved0[1])==campaign_peer_stamp)campaign_peer_loading=0;
                }
                Wifi_ConnectedClient c;int guests=Wifi_MultiplayerGetClients(1,&c);
                if(guests==1&&c.state!=WIFI_CLIENT_ASSOCIATED)guests=0;
                input_age++;
                if(guests==1&&(!guest_known||guest_aid!=c.association_id||memcmp(guest_mac,c.macaddr,sizeof(guest_mac)))){
                    int irq=enterCriticalSection();protocol_remote_reset(&remote);input_age=0;request_state=(CampaignRequestState){0};guest_profile_ready=0;campaign.menu_ack[1]=0;
                    memcpy(guest_mac,c.macaddr,sizeof(guest_mac));guest_aid=c.association_id;guest_known=1;leaveCriticalSection(irq);
                }
                if(guests==0){int irq=enterCriticalSection();protocol_remote_reset(&remote);leaveCriticalSection(irq);guest_known=0;guest_aid=0;arena.p[1].ready=0;}
                int start_match=0;
                int irq=enterCriticalSection();
                if(!arena.running){
                    unsigned guest_knight=remote.knight_id;
                    int ready=guests==1&&remote.seen&&remote.confirmed&&input_age<=12&&(!campaign_mode||(guest_profile_ready&&campaign.profiles[1].knight==guest_knight));
                    arena_select_knight(&arena,1,guest_knight,ready);
                }
                if(!arena.running&&(down&KEY_A)){host_ready=!host_ready;arena.countdown=(uint16_t)host_ready;}
                if((down&KEY_START)&&(!arena.running||arena.winner)&&guests==1&&remote.seen&&remote.confirmed&&input_age<=12&&arena_players_ready(&arena)&&(arena.running||host_ready)){
                    if(campaign_mode){if(!arena.running)campaign_start(&campaign,&arena,1);campaign_world_map(&campaign,&arena);}else arena_start_round(&arena);start_match=1;latched=0;
                }
                leaveCriticalSection(irq);
                if(arena.running&&(guests!=1||input_age>(campaign_peer_loading?1800:180)))fail("Guest connection lost");
                else if(start_match)Wifi_MultiplayerAllowNewClients(false);
            }else if(down&KEY_START){if(arena.winner){if(campaign_mode)campaign_world_map(&campaign,&arena);else{if(mode==PRACTICE)arena.p[1].knight_id=(uint8_t)((frames*2654435761u>>13)%KNIGHT_COUNT);arena_start_round(&arena);}}}
            campaign_sync_transition();
            if(campaign_mode&&campaign.phase==CAMPAIGN_WORLD_MAP&&(mode==HOST||mode==PRACTICE)){
                /* The route runs left to right and climbs to Thieves' Forest: up/right go on, down/left go back, with no wrap-around. */
                if((down&(KEY_UP|KEY_RIGHT))&&campaign.group+1<CAMPAIGN_WORLD_NODES)campaign_world_choose(&campaign,campaign.group+1);
                if((down&(KEY_DOWN|KEY_LEFT))&&campaign.group>0)campaign_world_choose(&campaign,campaign.group-1);
                if((down&KEY_A)&&!paused&&!campaign_world_travelling(&campaign)&&!(mode==HOST&&campaign_peer_loading)){campaign_world_enter(&campaign,&arena,campaign.group);latched=0;campaign_sync_transition();}
            }
            if((mode==HOST||mode==PRACTICE)&&game_due){
                uint16_t k[2]={controls(held)|latched,0};
                if(latched&ATTACK)k[0]|=ATTACK_EDGE;
                if(latched&HEAVY)k[0]|=HEAVY_EDGE;
                if(latched&JUMP)k[0]|=JUMP_EDGE;
                if(latched&BOW)k[0]|=BOW_EDGE;
                if(latched&EQUIP)k[0]|=EQUIP_EDGE;
                if(mode==HOST){int irq=enterCriticalSection();k[1]=protocol_remote_take(&remote);if(input_age>12)k[1]=0;leaveCriticalSection(irq);}else k[1]=campaign_mode?0:bot_input(&arena,practice_difficulty);
                if((!paused||(mode==HOST&&!campaign_mode))&&!(mode==HOST&&campaign_peer_loading)&&!(mode==PRACTICE&&panel_block)){if(campaign_mode){if(!campaign.paused){if(campaign.phase==CAMPAIGN_WORLD_MAP)campaign_world_tick(&campaign,&arena);else campaign_step(&campaign,&arena,k);}}else arena_step(&arena,k);}
                campaign_sync_transition();
                if(mode==HOST&&campaign_mode){CampaignPacket p;campaign_pack(&p,&campaign,&arena,++send_sequence);Wifi_MultiplayerHostCmdTxFrame(&p,sizeof(p));}
                else if(mode==HOST){StatePacket p={.magic=PROTOCOL_MAGIC,.version=PROTOCOL_VERSION,.sequence=++send_sequence,.arena=arena};Wifi_MultiplayerHostCmdTxFrame(&p,sizeof(p));}
            }
        }
        if(campaign_mode&&arena.running&&(mode==HOST||mode==CLIENT||mode==PRACTICE)){
            unsigned local=mode==CLIENT?1:0;
            if(campaign_feedback_update(&campaign_feedback,&campaign,&arena,local,game_due&&!paused&&!campaign.paused)&&!campaign_panel){campaign_panel=2;panel_selected=0;paused=0;latched=0;}
        }else campaign_feedback_reset(&campaign_feedback);
        effects_levelup[0]=campaign_feedback.up[0];effects_levelup[1]=campaign_feedback.up[1];
        {unsigned hero=mode==CLIENT?1:0;static char banner[32];const char *keys=0;const char *title=0;
         if(campaign_mode&&arena.running&&campaign_feedback.notice[hero]){
             static const struct{uint8_t level;const char *name;}combos[]={{2,"Spin attack"},{4,"Dash attacks"},{8,"Uppercut"},{16,"Long combo"},{32,"Heavy combo"},{50,"Master combo"}};
             title=campaign_feedback.combo[hero]?"NEW COMBO!":"LEVEL UP!";
             if(campaign_feedback.combo[hero]){
                 for(unsigned i=0;i<6;i++)if(combos[i].level==campaign_feedback.combo[hero])snprintf(banner,sizeof banner,"%s: %s",combos[i].name,campaign_feedback_combo_controls(campaign_feedback.combo[hero]));
                 keys=banner;
             }
         }
         arena_render_banner(title,keys);}
        if(campaign_mode&&arena.running&&(mode==HOST||mode==CLIENT||mode==PRACTICE)){
            save_capture(0);
            static int saved_map=-1,previous_phase=-1;
            int checkpoint=saved_map!=(int)arena.map_id||(previous_phase==KEEP_FIGHT&&campaign.phase==KEEP_TRAVEL)||(previous_phase==KEEP_STORE_FIGHT&&campaign.phase==KEEP_STORE);
            if(save_ready&&campaign_save_dirty()&&(save_status!=4||checkpoint)&&(checkpoint||arena.winner||(mode==PRACTICE&&(paused||campaign_panel))))save_status=campaign_save_flush()?2:4;
            saved_map=arena.map_id;previous_phase=campaign.phase;
        }
        enum AudioMode music=AUDIO_MENU;
        if((mode==HOST||mode==CLIENT||mode==PRACTICE)&&arena.running){
            music=campaign_mode?AUDIO_CAMPAIGN:AUDIO_ARENA;
            if(campaign_mode&&campaign.phase==CAMPAIGN_WORLD_MAP)music=AUDIO_WORLDMAP;
            else if(campaign_mode&&arena.map_id==BOSS_MAP)music=AUDIO_BARBARIAN_BOSS;
            else if(campaign_mode&&arena.map_id==FLOOD_MAP)music=AUDIO_FLOOD;
            else if(campaign_mode&&arena.map_id==DEER_MAP)music=AUDIO_DEER;
            else if(campaign_mode&&arena.map_id==THIEVES_MAP)music=AUDIO_THIEVES;
            else if(campaign_mode&&arena.map_id==FOREST_MAP)music=AUDIO_FOREST;
            else if(campaign_mode&&(arena.map_id==6||arena.map_id==7))music=AUDIO_WAR;
            else if(campaign_mode&&arena.map_id==8)music=AUDIO_BLACKSMITH;
            else if(campaign_mode&&arena.map_id==9)music=AUDIO_ARK;
            else if(campaign_mode&&arena.map_id==10)music=AUDIO_WEAPONS;
            else if(campaign_mode&&arena.map_id==5)music=campaign.phase==KEEP_BARRACKS_INTRO&&campaign.timer<161?AUDIO_BARRACKS:AUDIO_BARRACKS_QUIET;
        }
        audio_set_mode(music);audio_update(&arena);audio_update_enemy(campaign_mode?&campaign.enemies[0]:NULL);audio_update_campaign(campaign_mode?&campaign:NULL,&arena);
        if(game_due){draw(last_game_refresh+2);latched=0;}
        int static_ui=mode==CAMPAIGN_MENU||mode==MENU||mode==CONTROLS||mode==MULTIPLAYER||mode==ARENA_SELECT||mode==KNIGHT_SELECT||mode==ERROR_SCREEN;
        if(down||mode!=last_ui_mode||(!static_ui&&frames%6==0)){text_ui();last_ui_mode=mode;}
        uint16_t now=timerTick(0);unsigned work=(uint16_t)(now-work_begin);
        if(work>work_peak)work_peak=work;
        perf_elapsed+=(uint16_t)(now-perf_previous);perf_previous=now;
        if(perf_elapsed>=32728){
            measured_fps=render_count*32728/perf_elapsed;measured_work_ms=work_peak*1000/32728;
            render_count=0;perf_elapsed=0;work_peak=0;
        }
    }
}

