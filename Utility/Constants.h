/**
 * Constants.h
 * Holds global constants.
 * @author Ryuho Kudo
 * @date 3-16-11
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define WORLD_SIZE 80.00

#define LEVEL_WARMUP 0
#define LEVEL_PREWARMUP -1
#define MIN_MULTIPLAYER_PLAYERS 2
#define RESPAWN_TIME 8

#define PLAYER_LIVES 2

#define MAX_NAME_LENGTH 16
#define MAX_NAME_LENGTH_FORMAT "%16s"

#define BLASTER_SHOT_VOLUME 0.4f

#define VERT_FOV 45.0
#define LEFT_ALIGN 0
#define CENTERED 1
#define RIGHT_ALIGN 2

// Object Types
#define TYPE_ASTEROID3D 1
#define TYPE_ASTEROIDSHIP 2
#define TYPE_SHARD 3
#define TYPE_BEAMSHOT 4
#define TYPE_BLASTERSHOT 5
#define TYPE_ELECTRICITYSHOT 6
#define TYPE_ENERGYSHOT 7
#define TYPE_HOMINGMISSILESHOT 8
#define TYPE_TIMEDBOMBSHOT 9
#define TYPE_TRACTORBEAMSHOT 10

#define IS_SHOT(t) (t >= TYPE_BEAMSHOT && t <= TYPE_TRACTORBEAMSHOT)


// This should probably be an enum.
#define VIEW_COUNT 4
#define VIEW_FIRSTPERSON_SHIP 0
#define VIEW_FIRSTPERSON_GUN 1
#define VIEW_THIRDPERSON_SHIP 2
#define VIEW_THIRDPERSON_GUN 3

// Fbo color attachments.
#define NUM_BUFFERS 8

#define DEPTH_BUFFER GL_DEPTH_ATTACHMENT_EXT
#define GLOW_BUFFER GL_COLOR_ATTACHMENT0_EXT
#define BLUR_BUFFER GL_COLOR_ATTACHMENT1_EXT
#define BLOOM_BUFFER GL_COLOR_ATTACHMENT2_EXT
#define NORMAL_BUFFER GL_COLOR_ATTACHMENT3_EXT
#define ALBEDO_BUFFER GL_COLOR_ATTACHMENT4_EXT
#define OVERLAY_BUFFER GL_COLOR_ATTACHMENT5_EXT
#define NOLIGHT_BUFFER GL_COLOR_ATTACHMENT6_EXT
#define HUD_BUFFER GL_COLOR_ATTACHMENT7_EXT

// Network stuff
#define PACK_SIZE 1400

//packet id numbers
#define NET_HS_REQ 0
#define NET_HS_RES 1
#define NET_HS_ACK 2
#define NET_HS_FIN 3
#define NET_CLIENTCOMMAND 4
#define NET_KILL 5

#define NET_OBJ_REMOVE 100
#define NET_OBJ_SHARD 101
#define NET_OBJ_ASTEROID 102
#define NET_OBJ_SHIP 103
#define NET_OBJ_BLASTERSHOT 104
#define NET_OBJ_BEAMSHOT 105
#define NET_OBJ_TRACTORBEAMSHOT 106

#define NET_LEVEL_UPDATE 200

#define NET_ACTIVATE_STOREMENU 300

#define NET_SHIPID_REQ 500
#define NET_SHIPID_RES 501

#define NET_ALLOBJ_REQ 502
#define NET_ALLOBJ_FIN 503


#endif

