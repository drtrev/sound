/* sound - 3D sound project
 *
 * Copyright (C) 2011 Trevor Dodds <@gmail.com trev.dodds>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * network.h
 *
 * Defines constants for use with network operations
 */

#define PORT 3490          // the port users will be connecting to
// also uses 3491 and 3492 for UDP
// (3491 for server and 3492 for client so they can run on the same machine
#define BACKLOG 10         // how many pending connections queue will hold
#define MAX_RECV_DATA 5000 // max bytes to receive at once - if this is stored in buffer note BUFFER_SIZE (was 3000 before sound quality doubled)
#define MAX_CLIENTS 15     // max number of clients connected at once. Note when a user re-logs on they connect before
                           // disconnect is detected by the server so needs to be at least one spare client socket
#define ASCII_START_DATA 1 // the ASCII char indicating start of a data unit, for recovering from data loss
#define ASCII_PAD_DATA 2   // the ASCII char for padding out data to fill a data unit

// define data unit flags
#define FLAG_NONE ' '
#define FLAG_AUDIO 'a'
#define FLAG_AUDIO_SIZE 4000 // was 2000
#define FLAG_BULLET 'b'
#define FLAG_BULLET_SIZE 4 // one integer (4 bytes) (bullet type)
#define FLAG_GROUP_NEW 'g'
#define FLAG_GROUP_NEW_SIZE 12 // three integers, groupNum, client1 and client2
#define FLAG_GROUP_LEADER 'l'
#define FLAG_GROUP_LEADER_SIZE 8 // two integers, groupNum and clientId
#define FLAG_GROUP_JOIN 'j'
#define FLAG_GROUP_JOIN_SIZE 8 // two integers, groupNum and clientId
#define FLAG_GROUP_LEAVE 'L'
#define FLAG_GROUP_LEAVE_SIZE 4 // one int, groupNum
#define FLAG_GROUP_PUSH_PARENT 'P'
#define FLAG_GROUP_PUSH_PARENT_SIZE 4 // one int, clientId of user who is pushing
#define FLAG_GROUP_POP_PARENT 'O'
#define FLAG_GROUP_POP_PARENT_SIZE 4 // one int, clientId of user who is popping
#define FLAG_GROUP_ADD_PARENT 'A'
#define FLAG_GROUP_ADD_PARENT_SIZE 8 // clientId, parent, for adding to parent array when a new user logs on
#define FLAG_GROUP_SET 'S'
#define FLAG_GROUP_SET_SIZE 8 // clientId, groupNum
#define FLAG_GROUP_UPDATE 'u'
#define FLAG_GROUP_UPDATE_SIZE 28 // clientId, groupNum, parent1...5
#define FLAG_ID 'i'
#define FLAG_ID_SIZE 1
#define FLAG_LOGOUT 'o'
#define FLAG_LOGOUT_SIZE 4 // clientId
#define FLAG_NAME 'n'
#define FLAG_NAME_SIZE 10
#define FLAG_NAME_REQUEST 'r'
#define FLAG_NAME_REQUEST_SIZE 0
#define FLAG_PLAYBACK_FAST 'f'
#define FLAG_PLAYBACK_FAST_SIZE 0
#define FLAG_PLAYBACK_PAUSE 'w'
#define FLAG_PLAYBACK_PAUSE_SIZE 0
#define FLAG_PLAYBACK_REWIND 'W'
#define FLAG_PLAYBACK_REWIND_SIZE 0
#define FLAG_PLAYBACK_START 'Q'
#define FLAG_PLAYBACK_START_SIZE 0
#define FLAG_PLAYBACK_STOP 'q'
#define FLAG_PLAYBACK_STOP_SIZE 0
#define FLAG_POSITION 'p'
#define FLAG_POSITION_SIZE 20
#define FLAG_RECORD_STOP 'R'
#define FLAG_RECORD_STOP_SIZE 0
#define FLAG_TALK 't'
#define FLAG_TALK_SIZE 40
#define FLAG_TALK_REALTIME 'T'
#define FLAG_TALK_REALTIME_SIZE 1
#define FLAG_TIME_ELAPSED 'c'
#define FLAG_TIME_ELAPSED_SIZE 8
#define FLAG_USER_FOLLOW 'F'
#define FLAG_USER_FOLLOW_SIZE 4 // one int, the clientId of the user who is being followed
#define FLAG_USER_FOLLOW_STOP 'G'
#define FLAG_USER_FOLLOW_STOP_SIZE 0 // user has stopped following people
#define FLAG_USER_MEAN 'm'
#define FLAG_USER_MEAN_SIZE 0 // just need to know they pressed to go to the mean location
#define FLAG_USER_SCREENSHOT 'V'
#define FLAG_USER_SCREENSHOT_SIZE 0 // just need to know they took a screenshot
#define FLAG_USER_SELECT 's'
#define FLAG_USER_SELECT_SIZE 4 // one int, the clientId of the user who has been selected
#define FLAG_USER_VIEW 'v'
#define FLAG_USER_VIEW_SIZE 4 // view number

// zero is null, 1 is ASCII_START_DATA, 2 is server, client id's are > 2
#define SERVER_ID 2

#define PERSPECTIVE_FIRST_PERSON 0
#define PERSPECTIVE_THIRD_PERSON 1
#define PERSPECTIVE_OVERVIEW 2
#define PERSPECTIVE_TRANSITION_TO_FIRST 3
#define PERSPECTIVE_TRANSITION_TO_THIRD 4
