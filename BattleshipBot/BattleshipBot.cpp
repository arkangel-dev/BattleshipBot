// BattleshipBot.cpp : Defines the entry point for the console application.
//


#define _USE_MATH_DEFINES

#include "stdafx.h"
#include <winsock2.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <tuple>
#include <corecrt_math_defines.h>

#pragma comment(lib, "wsock32.lib")

#define SHIPTYPE_BATTLESHIP	"0"
#define SHIPTYPE_FRIGATE	"1"
#define SHIPTYPE_SUBMARINE	"2"

#define STUDENT_NUMBER		"Titan_[Bertolt]"
#define STUDENT_FIRSTNAME	"Sam"
#define STUDENT_FAMILYNAME	"Ramirez"
#define MY_SHIP	SHIPTYPE_BATTLESHIP

//#define IP_ADDRESS_SERVER	"127.0.0.1"
#define IP_ADDRESS_SERVER "127.0.0.1"

#define PORT_SEND	 1924 // We define a port that we are going to use.
#define PORT_RECEIVE 1925 // We define a port that we are going to use.


#define MAX_BUFFER_SIZE	500
#define MAX_SHIPS		200

#define FIRING_RANGE	100

#define MOVE_LEFT		-1
#define MOVE_RIGHT		 1
#define MOVE_UP			 1
#define MOVE_DOWN		-1
#define MOVE_FAST		 2
#define MOVE_SLOW		 1


SOCKADDR_IN sendto_addr;
SOCKADDR_IN receive_addr;

SOCKET sock_send;  // This is our socket, it is the handle to the IO address to read/write packets
SOCKET sock_recv;  // This is our socket, it is the handle to the IO address to read/write packets

WSADATA ws_data;

using namespace std;
char InputBuffer[MAX_BUFFER_SIZE];



int myX;
int myY;
int myHealth;
int myFlag;
int myType;

int number_of_ships;
int shipX[MAX_SHIPS];
int shipY[MAX_SHIPS];
int shipHealth[MAX_SHIPS];
int shipFlag[MAX_SHIPS];
int shipType[MAX_SHIPS];
bool message = false;
char MsgBuffer[MAX_BUFFER_SIZE];

bool fire = false;
int fireX;
int fireY;
bool moveShip = false;
int moveX;
int moveY;
bool setFlag = true;
int new_flag = 0;

void send_message(char* dest, char* source, char* msg);
void fire_at_ship(int X, int Y);
void move_in_direction(int left_right, int up_down);
void set_new_flag(int newFlag);

// my functions...
void displayCode(int opcode);
void shuffleMovement();
bool isCluster(int index);
int countShipsInRadius(int xIn, int yIn, int Radius);


/*************************************************************/
/********* Your tactics code starts here *********************/
/*************************************************************/

int up_down = MOVE_LEFT * MOVE_SLOW;
int left_right = MOVE_UP * MOVE_FAST;

int shipDistance[MAX_SHIPS];

int number_of_friends;
int friendX[MAX_SHIPS];
int friendY[MAX_SHIPS];
int friendHealth[MAX_SHIPS];
int friendFlag[MAX_SHIPS];
int friendDistance[MAX_SHIPS];
int friendType[MAX_SHIPS];

int number_of_enemies;
int enemyX[MAX_SHIPS];
int enemyY[MAX_SHIPS];
int enemyHealth[MAX_SHIPS];
int enemyFlag[MAX_SHIPS];
int enemyDistance[MAX_SHIPS];
int enemyType[MAX_SHIPS];

// My variables...
int current_patrol_point = 0;
int current_opcode = -1;
int friendlyFlag = 5005;
int commanderFlag = 5006;
int tele_currentship = 0;
bool tele_operating = false;
bool tele_reset_operating = false;
int tele_reset_currentship = 0;
char friendList[][64] = {"Titan_[Bertolt]", "Titan_[Zeke]"};

// telemetry variables
int tele_number_of_ships;
int tele_shipX[MAX_SHIPS];
int tele_shipY[MAX_SHIPS];
int tele_shipHealth[MAX_SHIPS];
int tele_shipFlag[MAX_SHIPS];


// what this function does is check if there is qeue to send
// telemetry data, and if there are, send it one by one... 
// one will be sent per tick...
//
// How the telemetry system will work
// ===================================
//
// The all ships will be synced to a commander ship...
// The commander ship will basically synchronise all the
// ships timers by regularly sending a reset command to all
// ships...
//
// When a ship recieves a reset command it will clear all its
// telemetry data and send out telemetry data to all the other ships
// Since this happens across all the ships... Each shp will recieve a 
// new set of telemetry data for all the ally ships... The reset command
// will be issued every 5 or 10 ticks
//
// The telemetry data will be stored in the following variables : 
//   int tele_number_of_ships;
//   int tele_shipX[MAX_SHIPS];
//   int tele_shipY[MAX_SHIPS];
//   int tele_shipHealth[MAX_SHIPS];
//   int tele_shipFlag[MAX_SHIPS];
//
void broadcastTelemetry_OP() {
	int friendListSize = sizeof(friendList) / sizeof(*friendList);
	if (tele_operating) {
		if (tele_currentship != friendListSize ) {
			char temp_ship[64];
			strcpy_s(temp_ship, friendList[tele_currentship]);
			char telemetryData[100]; 
			sprintf_s(telemetryData, "TELEMETRY %d %d %d %d", myX, myY, myFlag, myHealth);
			send_message(temp_ship, STUDENT_NUMBER, telemetryData);
			tele_currentship++;
		} else {
			tele_operating = false;
			tele_currentship = 0;
		}
	}
}

// what this function does is broadcast the request of reset the
// telemetry data and broadcast a fresh set of data...
void broadcastTelemetryReset_OP() {
	int friendListSize = sizeof(friendList) / sizeof(*friendList);
	if (tele_reset_operating) {
		if (tele_reset_currentship != friendListSize) {
			char temp_ship[64];
			strcpy_s(temp_ship, friendList[tele_reset_currentship]);
			char telemetryData[100];
			send_message(temp_ship, STUDENT_NUMBER, "RESET_TELEMETRY");
			tele_reset_currentship++;
		}
		else {
			tele_reset_operating = false;
			tele_reset_currentship = 0;
		}
	}
}


// what this function does is send the status
// of the ship to all the ships in the friendList array
// object
void broadcastTelemetry() {
	if (!tele_operating) {
		tele_operating = true;
	}
}

// what this function does is... bascially this function is
// a switch... a switch that will turn on the broadcast system
// and once the broadcast is complete turn it self off...
void broadcastTelemetryReset() {
	if (!tele_reset_operating) {
		tele_reset_operating = true;
	}
}

// what this function does is update the
// ship flag to something that all the ally
// ships can sync to such as a return value of a function
// where y = mx + b for and the m and b variables are
// pre-determined constants and the x value can be
// a tick counter from every time a telemetry-reset command
// is issued by the commander ship
// 
void updateFlag() {
	set_new_flag(rand() % 10 + 1);
}

// what this function does is accept an index
// of a ship and determine if the ship is a friendly
bool IsaFriend(int index) {
	bool rc;
	rc = false;
	//if ((shipFlag[index] == friendlyFlag) || (shipFlag[index] == commanderFlag)) {
	//	rc = true;
	//}

	if (index == 0) {
		return true;
	}

	return rc;
}

// What this function does is calculate the distance between our ship and a ship in the visible radius...
// A ship will be defined by its index. The function will then return a euclidian distance between our
// ship and the ship defined in the parameter...
float getDistance(int index) {
	float x_set = myX - shipX[index];
	x_set = x_set * x_set;
	float y_set = myY - shipY[index];
	y_set = y_set * y_set;
	float total_set = x_set + y_set;
	float final_set = sqrt(total_set);
	return final_set;
}

// What this function does is move TOWARDS an enemy ship...
void moveTowards(int enemyx, int enemyy) {
	int xcurr = myX;
	int ycurr = myY;
	int basex = enemyx - xcurr;
	int basey = enemyy - ycurr;
	basex = basex * 1;
	basey = basey * 1;
	move_in_direction(basex, basey);
}

// this function is part of the default code in the provided solution.
// It just randomly moves accross the plane untill it hits an edge and
// starts moving in the opposite direction...
void idleRandom() {
	if (myY > 900) { up_down = MOVE_DOWN * MOVE_SLOW; }
	if (myX < 200) { left_right = MOVE_RIGHT * MOVE_FAST; }
	if (myY < 100) { up_down = MOVE_UP * MOVE_FAST; }
	if (myX > 800) { left_right = MOVE_LEFT * MOVE_SLOW; }
	move_in_direction(left_right, up_down);
}


// what this function does is retrun a boolean value
// based on whether or not the string that are passed as parameters
// match or not
bool matchString(char* stringA, char* stringB) {
	if (strcmp(stringA, stringB) != 0) {
		return false;
	}
	else {
		return true;
	}
}

// TODO : Clean up this function...
void messageReceived(char* msg) {
	int X;
	int Y;
	int flag;
	int health;
	char operation_op[32];

	if (sscanf_s(msg, "%*s %*s %*s %s", &operation_op, _countof(operation_op)) == 1) {
		printf("Incoming Message : TOPIC : %s\n", operation_op);
		if (matchString(operation_op, "TELEMETRY")) {
			if (sscanf_s(msg, "%*s %*s %*s TELEMETRY %d %d %d %d", &X, &Y, &flag, &health) == 4) {
				printf("Incoming Telemetry Data : \n\tX : %d \n\tY : %d \n\tFlag : %d \n\tHealth : %d\n\n", X, Y, flag, health);

				tele_shipX[tele_number_of_ships] = X;
				tele_shipY[tele_number_of_ships] = Y;
				tele_shipFlag[tele_number_of_ships] = flag;
				tele_shipHealth[tele_number_of_ships] = health;
				tele_number_of_ships++; 

			} else {
				printf("Failed to parse telemetry data.\n");
			}
		} else if (matchString(operation_op, "RESET_TELEMETRY")) {
			printf("Resetting telemetry data\n");
			tele_number_of_ships = 0;
			broadcastTelemetry(); // enable broadcaster...
		} else {
			printf("Unknown topic.\n");
		}
	} else {
		printf("Failed to parse message.\n");
	}
}

// what this function does is move towards a co-ordinate...
// and that's it... nothing more...
void goTowards(int x, int y) {
	if (myX > x) {
		left_right = MOVE_LEFT * MOVE_FAST;
	}
	else {
		left_right = MOVE_RIGHT * MOVE_FAST;
	}
	if (myY < y) {
		up_down = MOVE_UP * MOVE_FAST;
	}
	else {
		up_down = MOVE_DOWN * MOVE_FAST;
	}
	move_in_direction(left_right * 2, (up_down) * 2);
}

// what this function is move the ship AWAY from a set of
// co-ordinates...
void moveAway(int x, int y) {
	if (myX > x) {left_right = (MOVE_LEFT * MOVE_FAST) * -1;
	} else {left_right = (MOVE_RIGHT * MOVE_FAST) * -1;}
	if (myY < y) {up_down = (MOVE_UP * MOVE_FAST) * -1;
	} else {up_down = (MOVE_DOWN * MOVE_FAST) * -1;}
	move_in_direction(left_right * 2, (up_down) * 2);
}

// what this function does is accept a set of co-ordinates and
// an offset, and if the ship is within a square box with a length
// of the offset, return true, if not return false
bool inZone(int xloc, int yloc, int offset) {
	int xzone_min = xloc - offset;
	int xzone_max = xloc + offset;
	int yzone_min = yloc - offset;
	int yzone_max = yloc + offset;

	if (((xzone_min <= myX) && (xzone_max >= myX)) && ((yzone_min <= myY) && (yzone_max >= myY))) {
		return true;
	}
	else {
		return false;
	}
}

// what this function does is move the ship along a pre-determined
// path along the edge of the map...
void patrol_path() {
	displayCode(300);
	int pathways[8][2] = {
		{100, 100},
		{450, 400},
		{900, 100},
		{600, 450},
		{900, 900},
		{450, 600},
		{100, 900},
		{300, 450}
	};

	int pathway_size = *(&pathways + 1) - pathways;
	for (int i = 0; i < pathway_size; i++) {
		int currentpath_x = pathways[i][0];
		int currentpath_y = pathways[i][1];
		if (inZone(currentpath_x, currentpath_y, 1)) {
			// rotate back to the head of the array if
			// the incrementer value is greater than the
			// size of the array
			if ((i + 1) == pathway_size) {
				current_patrol_point = 0;
				printf("[000] Patrol : Returning to home point\n");
			} else {
				current_patrol_point = i + 1;
			}
			printf("[000] Patrol : Setting path to point %d (%d, %d)\n", current_patrol_point, pathways[current_patrol_point][0], pathways[current_patrol_point][1]);
		}
	}

	// what this part does is move
	// the ship to the next point based
	// on the current_patrol_point variabl
	moveTowards(pathways[current_patrol_point][0], pathways[current_patrol_point][1]);
}

// what this function does is find the ship that is the weakest of them all...
// and return its index value...
int getWeakestEnemy() {
	int weakest_index = -1;
	int weakest_value = 11;
	for (int i = 1; i < number_of_ships; i++) {
		int index = i;
		if (shipFlag[index] != friendlyFlag) { // check if the ship is not a friendly
			if (shipHealth[index] < weakest_value) {
				weakest_index = index;
				weakest_value = shipHealth[index];
			}
		}
	}
	return weakest_index;
}

// what this function does is check if a point is within a circles boundary
// paramters (pointa_x, pointa_y, pointb_x, pointb_y, radius)
bool inRadius(int pointa_x, int pointa_y, int pointb_x, int pointb_y, int radius) {
	int x_set = (pointa_x - pointb_x);
	x_set = x_set * x_set;
	int y_set = (pointa_y - pointb_y);
	y_set = y_set * y_set;
	int total_set = x_set + y_set;
	int final_distance = sqrt(total_set);
	if (final_distance < radius) {
		return true;
	} else {
		return false;
	}
}




// what this function does is initilise the escape variables
// so that the escape() function will start to "escape"
bool escaping;
int escapex;
int escapey;
int escapeiteration;

// This function is used to trigger the escape sequence...
void escapeFrom(int xin, int yin, int iterations) {
	escaping = true;
	escapex = xin;
	escapey = yin;
	escapeiteration = iterations;
}

// the normal routine will be overwritten by this
// function when the ship encounters a cluster...
void escape_OP() {
	if (escaping) {
		moveAway(escapex, escapey);
		escapeiteration--;
		if (escapeiteration <= 0) {
			escaping = false;
		}
	}
}

// what this function does is accept a opcode and print a code
// once... So... that...
// So what this function ACTUALLY does is check if the requested code is
// not the last code that was printed...
bool matchcode(int opcode, int matchcode) {
	if (current_opcode != opcode) {
		if (opcode == matchcode) {
			current_opcode = opcode;
			return true;
		}
		return false;
	} else {
		return false;
	}
}

// what this function does is print the status code whenever
// its appropriate...
void displayCode(int opcode) {
	if (matchcode(opcode, 100)) {printf("[100] Engaging with ship...\n");}
	else if (matchcode(opcode, 101)) {printf("[101] Closing in on enemy ship...\n");}
	else if (matchcode(opcode, 102)) {printf("[102] Attacking ship...\n");}
	else if (matchcode(opcode, 200)) {printf("[200] Cluster Detected...\n");}
	else if (matchcode(opcode, 201)) {printf("[201] Escaping cluster...\n"); }
	else if (matchcode(opcode, 300)) {printf("[300] Running routine patrol...\n");}
	else if (matchcode(opcode, 500)) {printf("[500] Engine failed...\n");}
	else if (matchcode(opcode, 600)) {printf("[600] Saw a friendly...\n");}
	else if (matchcode(opcode, 700)) {printf("[700] Waiting for master...\n");}
	else if (matchcode(opcode, 800)) {printf("[800] Locking in on master...\n");}
	else if (matchcode(opcode, 900)) {printf("[900] Converging to rendezvous point... \n");}
}

// this function is meant to check if a boolean flag is needed
// to be toggled...
void conditionCheck() {

}

// what this function does is count the number of friendly ships and return
// the amount as an integer...
int countFriendlies() {
	int friendCount = -1;
	for (int i = 0; i < number_of_ships; i++) {
		if (IsaFriend(i)) {
			friendCount++;
		}
	}
	return friendCount;
}

// what this function does return the index of the master bot...
// if the master bot is not available... then return -1
int getMasterIndex() {
	int index = -1;
	for (int i = 0; i < number_of_ships; i++) {
		if (shipFlag[i] == commanderFlag) {
			index = i;
		}

	}
	return index;
}

// what this function does is count the number of enemies are in the
// visible range...
int countEnemies() {
	int enemy_count = 0;
	for (int i = 0; i < number_of_ships; i++) {
		if (!IsaFriend(i)) {
			enemy_count++;
		}
	}
	return enemy_count;
}

// what this function does is return the index of the nearest enemy ship in the 
// visible radius...
int getNearestEnemy() {
	int nearest_index = -1;
	int nearest_value = 99999;
	for (int i = 1; i < number_of_ships; i++) {
		int index = i;
		if (!IsaFriend(i)) {
			if (getDistance(index) < nearest_value) {
				nearest_index = index;
				nearest_value = getDistance(index);
			}
		}
	}
	return nearest_index;
}

// what this function does is fire at enemy ships if there are any nearby...
void attackNearbyEnemies() {
	if (countEnemies() != 0) {
		int tar_index = getNearestEnemy();
		if (getDistance(tar_index) < 100) {
			fire_at_ship(shipX[tar_index], shipY[tar_index]);
		}
	}
}

// what this function does is converge to a certain point on that map until
// either they reach that point wait for the master to arrive or if they see
// a master lock into formation around the master bot...
void convergeToBase() {
	if (getMasterIndex() == -1 && ((myX == 500) && (myY == 500))) {
		displayCode(700);
	}
	else if (!((myX == 500) && (myY == 500))) {
		displayCode(900);
		moveTowards(500, 500);
	}
	attackNearbyEnemies();
}

// in here are the functions that will be have to be executed
// on every tick, to make sure that the code is synchronous
// with the testing server...
void routineFunctions() {
	broadcastTelemetry_OP();
	broadcastTelemetryReset_OP();
	escape_OP();
}


bool evadeClusters = true;
int clusterDefinitionCount = 3;
int clusterDefinitionRadius = 25;

// what this function does is accept an index of a ship, then use the countShipsInRadius()
// function and determine if a ship is a part of a cluster...
bool isCluster(int index) {
	if (countShipsInRadius(shipX[index], shipY[index], clusterDefinitionRadius) >= clusterDefinitionCount) {
		return true;
	}
	else {
		return false;
	}
}


// what this function does is accept an X and Y co-ordinate and
// a radius value and return the number of ships in that range...
int countShipsInRadius(int xIn, int yIn, int Radius) {
	int returnCount = 0;
	for (int i = 1; i < number_of_ships; i++) {
		if (inRadius(xIn, yIn, shipX[i], shipY[i], Radius)) {
			returnCount++;
		}
	}
	return returnCount;
}

// what this function does is move to towards the nearest ship and destroy
// it... Its bascically the old attack tactic put into a function...
void seekAndDestroy() {
	if (countEnemies() != 0) {
		int closestEnemy = getNearestEnemy();
		if (!isCluster(closestEnemy)) {
			if (inRadius(myX, myY, shipX[closestEnemy], shipY[closestEnemy], 100)) {
				displayCode(102);
				fire_at_ship(shipX[closestEnemy], shipY[closestEnemy]);
				if (inRadius(myX, myY, shipX[closestEnemy], shipY[closestEnemy], 90)) {
					shuffleMovement();
				} else {
					moveTowards(shipX[closestEnemy], shipY[closestEnemy]);
				}
			} else {
				displayCode(101);
				moveTowards(shipX[closestEnemy], shipY[closestEnemy]);
			}
		} else {
			displayCode(201);
			escapeFrom(shipX[closestEnemy], shipY[closestEnemy], 10);
		}
	}
}

// what this function does is rotate a point around an origin at a specified degree counter clockwise...
tuple<int, int> rotateAroundOrigin(int originX, int originY, int pointX, int pointY, int angle) {
	double radians = (angle) * (M_PI / 180);
	double rotateX = cos(radians) * (pointX - originX) - sin(radians) * (pointX - originX) + originX;
	double rotateY = sin(radians) * (pointX - originX) + cos(radians) * (pointY - originY) + originY;
	return make_tuple((int)rotateX, (int)rotateY);
}


// what this function does it return a boolean
// valued based on whether or not there are any enemies present...
// NOTE: Uses the flags to determine if the ship is a 
// ally or not...
bool enemyPresent() {
	if (countEnemies() > 0) {
		return true;
	} else {
		return false;
	}
}

// what this function does is move the ship on a 3x3 grid
// so what reduces the changes of an enemy shell hitting our
// ship from absolute certainity to (1/9)...
void shuffleMovement() {
	int movex = (rand() % 3 + 1) - 2;
	int movey = (rand() % 3 + 1) - 2;
	move_in_direction(movex, movey);
}

void tactics() {
	
	if (enemyPresent()) {
		seekAndDestroy();
	}
	else {
		patrol_path();
	}
	routineFunctions();
}

/*************************************************************/
/********* Your tactics code ends here ***********************/
/*************************************************************/

// Print out the contents of the ascii.txt file to the console...
// For aesthetic affects...
void outputHeader() {
	std::ifstream f("ascii.txt");
	if (f.is_open())
		std::cout << f.rdbuf();
	printf("\n\n");
}

void communicate_with_server() {
	char buffer[4096];
	int  len = sizeof(SOCKADDR);
	char chr;
	bool finished;
	int  i;
	int  j;
	int  rc;
	char* p;

	sprintf_s(buffer, "Register  %s,%s,%s,%s", STUDENT_NUMBER, STUDENT_FIRSTNAME, STUDENT_FAMILYNAME, MY_SHIP);
	sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));

	while (true) {
		if (recvfrom(sock_recv, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&receive_addr, &len) != SOCKET_ERROR) {
			p = ::inet_ntoa(receive_addr.sin_addr);

			if ((strcmp(IP_ADDRESS_SERVER, "127.0.0.1") == 0) || (strcmp(IP_ADDRESS_SERVER, p) == 0)) {
				if (buffer[0] == 'M') {
					messageReceived(buffer);

				} else {
					i = 0;
					j = 0;
					finished = false;
					number_of_ships = 0;

					while ((!finished) && (i < 4096)) {
						chr = buffer[i];

						switch (chr) {
						case '|':
							InputBuffer[j] = '\0';
							j = 0;
							sscanf_s(InputBuffer, "%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships], &shipType[number_of_ships]);
							number_of_ships++;
							break;

						case '\0':
							InputBuffer[j] = '\0';
							sscanf_s(InputBuffer, "%d,%d,%d,%d", &shipX[number_of_ships], &shipY[number_of_ships], &shipHealth[number_of_ships], &shipFlag[number_of_ships], &shipType[number_of_ships]);
							number_of_ships++;
							finished = true;
							break;

						default:
							InputBuffer[j] = chr;
							j++;
							break;
						}
						i++;
					}

					myX = shipX[0];
					myY = shipY[0];
					myHealth = shipHealth[0];
					myFlag = shipFlag[0];
					myType = shipType[0];
				}

				tactics();

				if (message) {
					sendto(sock_send, MsgBuffer, strlen(MsgBuffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					message = false;
				}

				if (fire) {
					sprintf_s(buffer, "Fire %s,%d,%d", STUDENT_NUMBER, fireX, fireY);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					fire = false;
				}

				if (moveShip) {
					sprintf_s(buffer, "Move %s,%d,%d", STUDENT_NUMBER, moveX, moveY);
					rc = sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					moveShip = false;
				}

				if (setFlag) {
					sprintf_s(buffer, "Flag %s,%d", STUDENT_NUMBER, new_flag);
					sendto(sock_send, buffer, strlen(buffer), 0, (SOCKADDR *)&sendto_addr, sizeof(SOCKADDR));
					setFlag = false;
				}

			}
		} else {
			printf_s("recvfrom error = %d\n", WSAGetLastError());
		}
	}
	printf_s("Student %s\n", STUDENT_NUMBER);
}



void send_message(char* dest, char* source, char* msg) {
	message = true;
	sprintf_s(MsgBuffer, "Message %s,%s,%s,%s", STUDENT_NUMBER, dest, source, msg);
}



void fire_at_ship(int X, int Y) {
	fire = true;
	fireX = X;
	fireY = Y;
}



void move_in_direction(int X, int Y) {
	if (X < -2) X = -2;
	if (X > 2) X = 2;
	if (Y < -2) Y = -2;
	if (Y > 2) Y = 2;

	moveShip = true;
	moveX = X;
	moveY = Y;
}


void set_new_flag(int newFlag) {
	setFlag = true;
	new_flag = newFlag;
}



int _tmain(int argc, _TCHAR* argv[])
{
	char chr = '\0';

	printf("\n");
	printf("Battleship Bots\n");
	printf("UWE Computer and Network Systems Assignment 2 (2016-17)\n");
	outputHeader();

	if (WSAStartup(MAKEWORD(2, 2), &ws_data) != 0) return(0);

	sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_send) {
		printf("Socket creation failed!\n");
	}

	sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // Here we create our socket, which will be a UDP socket (SOCK_DGRAM).
	if (!sock_recv) {
		printf("Socket creation failed!\n");
	}

	memset(&sendto_addr, 0, sizeof(SOCKADDR_IN));
	sendto_addr.sin_family = AF_INET;
	sendto_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS_SERVER);
	sendto_addr.sin_port = htons(PORT_SEND);

	memset(&receive_addr, 0, sizeof(SOCKADDR_IN));
	receive_addr.sin_family = AF_INET;
	receive_addr.sin_addr.s_addr = INADDR_ANY;
	receive_addr.sin_port = htons(PORT_RECEIVE);

	int ret = bind(sock_recv, (SOCKADDR *)&receive_addr, sizeof(SOCKADDR));
	if (ret) {
		printf("Bind failed! %d\n", WSAGetLastError());
	}

	communicate_with_server();
	closesocket(sock_send);
	closesocket(sock_recv);
	WSACleanup();

	while (chr != '\n') {
		chr = getchar();
	}

	return 0;
}

