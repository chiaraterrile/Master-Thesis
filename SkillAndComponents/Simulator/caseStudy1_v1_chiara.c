#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////
// Simulation support
///////////////////////////////////////////////////////////////////////////

// Additional libraries
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

// Utility functions to generate random numbers
long int randomInteger(int low, int high)
{
	return random() % (high - low + 1) + low;
}

long int randomIntegerWithExclusion(int low, int high, int excludeNum)
{
	long int retNum = random() % (high - low + 1) + low;
	if (retNum == excludeNum)
	{
		randomIntegerWithExclusion(low, high, excludeNum);
	}
	else
	{
		return retNum;
	}
	return 0;
}

// Utility to shuffle an array of integer
void shuffle(int *array, int n)
{
	if (n > 1)
	{
		int i;
		for (i = 0; i < n - 1; i++)
		{
			int j = i + random() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

// Variable used as global clock
static int countLoop = 1;

// Variable used to manage wallclock time
struct timeval start, end;
pthread_mutex_t mutexTime = PTHREAD_MUTEX_INITIALIZER;

// Variable used to print simulation data to file
FILE *out_file;

//Variable used to manage timer
#ifdef MONITOR2
static bool monitor2_timerIsArmed = false;
#endif

#ifdef MONITOR3
static bool monitor3_timerIsArmed = false;
#endif

// Utility structure to hold the state of the abstract simulation
typedef struct
{
	int atChargingStation;
	int atDestination;
	int cyclesToDestination;
	int cyclesToDestinationPlanned;
	int cyclesToCharging;
	int cyclesToChargingPlanned;
	int batteryLevel;
	int currentDestination;
	int storedDestination;
	int batteryDrop;
	int batteryRise;
	int chargingStationSign;
	bool isArrived;
	bool isStarted;
} SimulationInfo;

SimulationInfo simInfo;

// typedef or pthread function
typedef void *(*pthreadFun)(void *);

///////////////////////////////////////////////////////////////////////////
// Thread ids
//
// Each thread has a unique id that serves the purpose of identifying
// the thread during communications among threads
//
///////////////////////////////////////////////////////////////////////////

typedef enum
{
	absentID = 0,
	bt_id = 1,
	skill1_id = 2,
	skill2_id = 3,
	skill3_id = 4,
	skill4_id = 5,
	scmCondition2_id = 6,
	scmCondition3_id = 7,
	scmAction1_id = 8,
	scmAction4_id = 9,
	ccmBr_id = 10,
	ccmGoTo_id = 11,
	component_id = 12,
	visit_id = 13,
	skill5_id = 14,
	skill6_id = 15,
	scmCondition5_id = 16,
	scmCondition6_id = 17,
	refresh_id = 18
} thread_id;

///////////////////////////////////////////////////////////////////////////
// Declarations for channels
//
// Channels with capacity 1 follow the pattern:
// - an array to store the messages
// - a counter that holds the number of messages (at most 1)
// - a condition to signal when the channel is full
// - a condition to signal when the channel is not empty
// - a mutex to synchronize access to the channel
//
// Channels with capacity greater than 1 follow the pattern:
// - an array to store the messages
// - indexes to the (circular) queue front and rear
// - a counter that holds the number of messages
// - a condition to signal when the channel is not empty
// - a mutex to synchronize access to the channel
// - a different "full" condition for each client accessing the channel
//
// SCM stands for Skill Communication Manager
// CCM stands for Component Communication Manager
///////////////////////////////////////////////////////////////////////////

/////////////////////////
// Maximum size of channel buffers from skills to components:
// - ROWS how many messages can be queued at most (it corresponds
//   to the number of skill requesting services from the component)
// - COLS how many data fields in a single message at most
/////////////////////////

int MAX_BUFF_BR_ROWS = 2; // Battery Reader component
int MAX_BUFF_BR_COLS = 2;
int MAX_BUFF_GOTO_ROWS = 4; // Navigation component
int MAX_BUFF_GOTO_COLS = 3;

/////////////////////////
// Channel to and from the BT
////////////////////////

//from visit to  BT root node
int bufferFromVisitToBt[1] = {0};
int countFromVisitToBt = 0;
pthread_cond_t condFullFromVisitToBt = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromVisitToBt = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexVisitToBt = PTHREAD_MUTEX_INITIALIZER;

//from BT root node to visit
int bufferFromBtToVisit[1] = {0};
int countFromBtToVisit = 0;
pthread_cond_t condfullFromBtToVisit = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromBtToVisit = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexBtToVisit = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Action1 Skill1 GoToDestination
/////////////////////////

// from Action1 to SCM1
int bufferAction1ToSCM1[2] = {0, 0};
int countAction1ToSCM1 = 0;
pthread_cond_t condFullFromAction1ToSCM1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromAction1ToSCM1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexAction1ToSCM1 = PTHREAD_MUTEX_INITIALIZER;

//from SCM1 to Action1
int bufferSCM1ToAction1[2] = {0, 0};
int countSCM1ToAction1 = 0;
pthread_cond_t condFullFromSCM1ToAction1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM1ToAction1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM1ToAction1 = PTHREAD_MUTEX_INITIALIZER;

//from SCM1 to Skill1
int bufferSCM1ToSkill1[2] = {0, 0};
int countSCM1ToSkill1 = 0;
pthread_cond_t condFullFromSCM1ToSkill1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM1ToSkill1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM1ToSkill1 = PTHREAD_MUTEX_INITIALIZER;

//from Skill1 to SCM1
int bufferSkill1ToSCM1[2] = {0, 0};
int countSkill1ToSCM1 = 0;
pthread_cond_t condFullFromSkill1ToSCM1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill1ToSCM1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill1ToSCM1 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Condition2 and Skill2 BatteryLevel
/////////////////////////

// from Condition2 to SCM2
int bufferCondition2ToSCM2[2] = {0, 0};
int countCondition2ToSCM2 = 0;
pthread_cond_t condFullFromCondition2ToSCM2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCondition2ToSCM2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCondition2ToSCM2 = PTHREAD_MUTEX_INITIALIZER;

// from SCM2 to Condition2
int bufferSCM2ToCondition2[2] = {0, 0};
int countSCM2ToCondition2 = 0;
pthread_cond_t condFullFromSCM2ToCondition2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM2ToCondition2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM2ToCondition2 = PTHREAD_MUTEX_INITIALIZER;

// from SCM2 to Skill2
int bufferSCM2ToSkill2[2] = {0, 0};
int countSCM2ToSkill2 = 0;
pthread_cond_t condFullFromSCM2ToSkill2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM2ToSkill2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM2ToSkill2 = PTHREAD_MUTEX_INITIALIZER;

// from Skill2 to SCM2
int bufferSkill2ToSCM2[2] = {0, 0};
int countSkill2ToSCM2 = 0;
pthread_cond_t condFullFromSkill2ToSCM2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill2ToSCM2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill2ToSCM2 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Condition3 and Skill3 BatteyNotCharging
/////////////////////////

// from Condition3 to SCM3
int bufferCondition3ToSCM3[2] = {0, 0};
int countCondition3ToSCM3 = 0;
pthread_cond_t condFullFromCondition3ToSCM3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCondition3ToSCM3 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCondition3ToSCM3 = PTHREAD_MUTEX_INITIALIZER;

// from SCM3 to Condition3
int bufferSCM3ToCondition3[2] = {0, 0};
int countSCM3ToCondition3 = 0;
pthread_cond_t condFullFromSCM3ToCondition3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM3ToCondition3 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM3ToCondition3 = PTHREAD_MUTEX_INITIALIZER;

// from scm3 to Skill3
int bufferSCM3ToSkill3[2] = {0, 0};
int countSCM3ToSkill3 = 0;
pthread_cond_t condFullFromSCM3ToSkill3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM3ToSkill3 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM3ToSkill3 = PTHREAD_MUTEX_INITIALIZER;

// from Skill3 to SCM3
int bufferSkill3ToSCM3[2] = {0, 0};
int countSkill3ToSCM3 = 0;
pthread_cond_t condFullFromSkill3ToSCM3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill3ToSCM3 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill3ToSCM3 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Action4 and Skill4 GoToChargingStation
/////////////////////////

// from Action4   to SCM4
int bufferAction4ToSCM4[2] = {0, 0};
int countAction4ToSCM4 = 0;
pthread_cond_t condFullFromAction4ToSCM4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromAction4ToSCM4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexAction4ToSCM4 = PTHREAD_MUTEX_INITIALIZER;

// from SCM4 to Action4
int bufferSCM4ToAction4[2] = {0, 0};
int countSCM4ToAction4 = 0;
pthread_cond_t condFullFromSCM4ToAction4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM4ToAction4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM4ToAction4 = PTHREAD_MUTEX_INITIALIZER;

// from SCM4 to Skill4
int bufferSCM4ToSkill4[2] = {0, 0};
int countSCM4ToSkill4 = 0;
pthread_cond_t condFullFromSCM4ToSkill4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM4ToSkill4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM4ToSkill4 = PTHREAD_MUTEX_INITIALIZER;

// from Skill4 to SCM4
int bufferSkill4ToSCM4[2] = {0, 0};
int countSkill4ToSCM4 = 0;
pthread_cond_t condFullFromSkill4ToSCM4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill4ToSCM4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill4ToSCM4 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Condition5 and Skill5 AtDestination (using GoToDestination thrift)
/////////////////////////

// from Condition5 to SCM5
int bufferCondition5ToSCM5[2] = {0, 0};
int countCondition5ToSCM5 = 0;
pthread_cond_t condFullFromCondition5ToSCM5 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCondition5ToSCM5 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCondition5ToSCM5 = PTHREAD_MUTEX_INITIALIZER;

// from SCM5 to Condition5
int bufferSCM5ToCondition5[2] = {0, 0};
int countSCM5ToCondition5 = 0;
pthread_cond_t condFullFromSCM5ToCondition5 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM5ToCondition5 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM5ToCondition5 = PTHREAD_MUTEX_INITIALIZER;

// from scm5 to Skill5
int bufferSCM5ToSkill5[2] = {0, 0};
int countSCM5ToSkill5 = 0;
pthread_cond_t condFullFromSCM5ToSkill5 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM5ToSkill5 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM5ToSkill5 = PTHREAD_MUTEX_INITIALIZER;

// from Skill5 to SCM5
int bufferSkill5ToSCM5[2] = {0, 0};
int countSkill5ToSCM5 = 0;
pthread_cond_t condFullFromSkill5ToSCM5 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill5ToSCM5 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill5ToSCM5 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between Condition6 and Skill6 AtChargingStation (using GoToDestination thrift)
/////////////////////////

// from Condition6 to SCM6
int bufferCondition6ToSCM6[2] = {0, 0};
int countCondition6ToSCM6 = 0;
pthread_cond_t condFullFromCondition6ToSCM6 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCondition6ToSCM6 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCondition6ToSCM6 = PTHREAD_MUTEX_INITIALIZER;

// from SCM6 to Condition6
int bufferSCM6ToCondition6[2] = {0, 0};
int countSCM6ToCondition6 = 0;
pthread_cond_t condFullFromSCM6ToCondition6 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM6ToCondition6 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM6ToCondition6 = PTHREAD_MUTEX_INITIALIZER;

// from SCM6 to Skill6
int bufferSCM6ToSkill6[2] = {0, 0};
int countSCM6ToSkill6 = 0;
pthread_cond_t condFullFromSCM6ToSkill6 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSCM6ToSkill6 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSCM6ToSkill6 = PTHREAD_MUTEX_INITIALIZER;

// from Skill6 to SCM6
int bufferSkill6ToSCM6[2] = {0, 0};
int countSkill6ToSCM6 = 0;
pthread_cond_t condFullFromSkill6ToSCM6 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromSkill6ToSCM6 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkill6ToSCM6 = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between skills and the GoTo Component (Navigation)
/////////////////////////

// From Skills to CCM GoTo
int bufferSkillsToCCM_GoTo[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
static int frontIndex_CCM_GoTo = -1;
static int rearIndex_CCM_GoTo = -1;
int countSkillsToCCM_GoTo = 0;
pthread_cond_t condNotEmptyFromSkillsToCCM_GoTo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkillsToCCM_GoTo = PTHREAD_MUTEX_INITIALIZER;

bool hasBeenReadSkill1 = false;
pthread_cond_t condFullFromSkill1ToCCM_GoTo = PTHREAD_COND_INITIALIZER;

bool hasBeenReadSkill4 = false;
pthread_cond_t condFullFromSkill4ToCCM_GoTo = PTHREAD_COND_INITIALIZER;

bool hasBeenReadSkill5 = false;
pthread_cond_t condFullFromSkill5ToCCM_GoTo = PTHREAD_COND_INITIALIZER;

bool hasBeenReadSkill6 = false;
pthread_cond_t condFullFromSkill6ToCCM_GoTo = PTHREAD_COND_INITIALIZER;

// from CCM_GoTo to skill1
int bufferCCM_GoToToSkill1[2] = {0, 0};
int countCCM_GoToToSkill1 = 0;
pthread_cond_t condFullFromCCM_GoToToSkill1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_GoToToSkill1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_GoToToSkill1 = PTHREAD_MUTEX_INITIALIZER;

// from CCM_GoTo to skill4
int bufferCCM_GoToToSkill4[2] = {0, 0};
int countCCM_GoToToSkill4 = 0;
pthread_cond_t condFullFromCCM_GoToToSkill4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_GoToToSkill4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_GoToToSkill4 = PTHREAD_MUTEX_INITIALIZER;

// from CCM_GoTo to skill5
int bufferCCM_GoToToSkill5[2] = {0, 0};
int countCCM_GoToToSkill5 = 0;
pthread_cond_t condFullFromCCM_GoToToSkill5 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_GoToToSkill5 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_GoToToSkill5 = PTHREAD_MUTEX_INITIALIZER;

// from CCM_GoTo to skill6
int bufferCCM_GoToToSkill6[2] = {0, 0};
int countCCM_GoToToSkill6 = 0;
pthread_cond_t condFullFromCCM_GoToToSkill6 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_GoToToSkill6 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_GoToToSkill6 = PTHREAD_MUTEX_INITIALIZER;

// from CCM_GoTo to Navigation component
int bufferCCM_GoToToNavigation[2] = {0, 0};
int countCCM_GoToToNavigation = 0;
pthread_cond_t condFullFromCCM_GoToToNavigation = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_GoToToNavigation = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_GoToToNavigation = PTHREAD_MUTEX_INITIALIZER;

// from Navigation component to CCM_GoTo
int bufferNavigationToCCM_GoTo[2] = {0, 0};
int countNavigationToCCM_GoTo = 0;
pthread_cond_t condFullFromNavigationToCCM_GoTo = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromNavigationToCCM_GoTo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexNavigationToCCM_GoTo = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////
// Channels between skills and the BatteryReader Component
/////////////////////////

// from Skills to CCM_BR
int bufferSkillsToCCM_BR[2][2] = {{0, 0}, {0, 0}};
static int frontIndex_CCM_BR = -1;
static int rearIndex_CCM_BR = -1;
int countSkillsToCCM_BR = 0;
pthread_cond_t condNotEmptyFromSkillsToCCM_BR = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexSkillsToCCM_BR = PTHREAD_MUTEX_INITIALIZER;

bool hasBeenReadSkill2 = false;
pthread_cond_t condFullFromSkill2ToCCM_BR = PTHREAD_COND_INITIALIZER;

bool hasBeenReadSkill3 = false;
pthread_cond_t condFullFromSkill3ToCCM_BR = PTHREAD_COND_INITIALIZER;

// from CCM to skill2
int bufferCCM_BRToSkill2[2] = {0, 0};
int countCCM_BRToSkill2 = 0;
pthread_cond_t condFullFromCCM_BRToSkill2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_BRToSkill2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_BRToSkill2 = PTHREAD_MUTEX_INITIALIZER;

// from  CCM_BR to skill3
int bufferCCM_BRToSkill3[2] = {0, 0};
int countCCM_BRToSkill3 = 0;
pthread_cond_t condFullFromCCM_BRToSkill3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_BRToSkill3 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_BRToSkill3 = PTHREAD_MUTEX_INITIALIZER;

// from CCM to battery reader
int bufferCCM_BRToBatteryReader[2] = {0, 0};
int countCCM_BRToBatteryReader = 0;
pthread_cond_t condFullFromCCM_BRToBatteryReader = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyFromCCM_BRToBatteryReader = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexCCM_BRToBatteryReader = PTHREAD_MUTEX_INITIALIZER;

// from  battery reader to CCM
int bufferBatteryReaderToCCM_BR[2] = {0, 0};
int countBatteryReaderToCCM_BR = 0;
pthread_cond_t condFullFromBatteryReaderToCCM_BR = PTHREAD_COND_INITIALIZER;
pthread_cond_t condNotEmptyBatteryReaderToCCM_BR = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexBatteryReaderToCCM_BR = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////
// Mutexes to syncrhonize shared variables in skills and components
///////////////////////////////////////////////////////////////////////////

// skill internal mutex
pthread_mutex_t mutexSkill1ReadWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSkill2ReadWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSkill3ReadWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSkill4ReadWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSkill5ReadWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSkill6ReadWrite = PTHREAD_MUTEX_INITIALIZER;

//component mutex
pthread_mutex_t mutexComponentReadWrite = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////
// Mutexes to synchronize the monitor
///////////////////////////////////////////////////////////////////////////

pthread_mutex_t mutexBtToAction1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBtToCondition2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBtToCondition3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBtToAction4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBtToCondition5 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBtToCondition6 = PTHREAD_MUTEX_INITIALIZER;

// monitor mutex
pthread_mutex_t mutexMonitor = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////
// Enumerations to define channel domains and declarations of
// status and monitoring variables
///////////////////////////////////////////////////////////////////////////

// Declaration of a void value to simulate void returns
typedef enum
{
	voidVal = 0
} voidRetValue;

// Bt status and Signal types
typedef enum
{
	STATUS_ABSENT = 0,
	STATUS_RUNNING = 1,
	STATUS_SUCCESS = 2,
	STATUS_FAILURE = 3,
	STATUS_IDLE = 4
} status;
static status monitorVisitStatus = STATUS_ABSENT;
// static status monitorAction1Status = STATUS_ABSENT;
// static status monitorCondition2Status = STATUS_ABSENT;
// static status monitorCondition3Status = STATUS_ABSENT;
// static status monitorAction4Status = STATUS_ABSENT;
// static status monitorCondition5Status = STATUS_ABSENT;
// static status monitorCondition6Status = STATUS_ABSENT;

// BT visit
typedef enum
{
	ABSENT = 0,
	TICK = 1,
	HALT = 2
} btSignal;
static btSignal monitorBtSignal = ABSENT;
// static btSignal monitorAction1Visit = ABSENT;
// static btSignal monitorCondition2Visit = ABSENT;
// static btSignal monitorCondition3Visit = ABSENT;
// static btSignal monitorAction4Visit = ABSENT;
// static btSignal monitorCondition5Visit = ABSENT;
// static btSignal monitorCondition6Visit = ABSENT;

// Standard interface between leaf nodes and skills
typedef enum
{
	absent_command = 0,
	send_start = 1,
	send_stop = 2,
	send_ok = 3,
	request_ack = 4
} skill_request;
static skill_request monitorAction1Cmd = absent_command;
static skill_request monitorAction4Cmd = absent_command;
static skill_request monitorCondition2Cmd = absent_command;
static skill_request monitorCondition3Cmd = absent_command;
static skill_request monitorCondition5Cmd = absent_command;
static skill_request monitorCondition6Cmd = absent_command;

// Battery ChargingStatus enum
typedef enum
{
	BATTERY_NOT_CHARGING = 0,
	BATTERY_CHARGING = 1
} ChargingStatus;

// GoTo Skill GoToStatus enum
typedef enum
{
	NOT_STARTED = 0,
	RUNNING = 1,
	SUCCESS = 2,
	ABORT = 3
} GoToStatus;

// Component BatteryReader functions name
typedef enum
{
	batteryReader_absentFunction = 0,
	level = 1,
	charging_status = 2
} BatteryReaderComponentFunction;
//static BatteryReaderComponentFunction monitorSkill2Comp2Cmd = batteryReader_absentFunction;
//static BatteryReaderComponentFunction monitorSkill3Comp3Cmd = batteryReader_absentFunction;

// Location variable destination
typedef enum
{
	ABSENT_LOCATION = 0,
	KITCHEN = 1,
	CHARGING_STATION = 2
} destination;
static destination currentSimLocation = ABSENT_LOCATION;

// Component GoTo functions name
typedef enum
{
	goTo_absentFunction = 0,
	goTo = 1,
	getStatus = 2,
	halt = 3,
	isAtLocation = 4
} GoToComponentFunction;

// Navigation command
typedef enum
{
	navigation_absentCmd = 0,
	start_navigation = 1,
	getStatus_navigation = 2,
	stop_navigation = 3
} NavigationCommand;
static NavigationCommand monitorCCM_GoToToNavigationCmd = navigation_absentCmd;

// Condition nodes
typedef enum
{
	CONDITION_IDLE = 0,
	CONDITION_START_SKILL = 1,
	CONDITION_WAIT_ANSWER = 2,
	CONDITION_RETURN_SUCCESS = 3,
	CONDITION_RETURN_FAILURE = 4,
	CONDITION_RETURN_IDLE = 5
} conditionState;
static conditionState currentCondition2State = CONDITION_IDLE;
static conditionState currentCondition3State = CONDITION_IDLE;
static conditionState currentCondition5State = CONDITION_IDLE;
static conditionState currentCondition6State = CONDITION_IDLE;

// Action node
typedef enum
{
	ACTION_IDLE = 0,
	ACTION_START_SKILL = 1,
	ACTION_SKILL_STARTED = 2,
	ACTION_SKILL_STOPPED = 3,
	ACTION_WAIT_ANSWER = 4,
	ACTION_WAIT_RUNNING = 5,
	ACTION_RETURN_RUNNING = 6,
	ACTION_RETURN_SUCCESS = 7,
	ACTION_RETURN_FAILURE = 8,
	ACTION_STOP_SKILL = 9,
	ACTION_RETURN_IDLE = 10
} actionState;
static actionState currentAction1State = ACTION_IDLE;
static actionState currentAction4State = ACTION_IDLE;

// Command signals
typedef enum
{
	CMD_ABSENT = 0,
	CMD_START = 1,
	CMD_STOP = 2,
	CMD_OK = 3,
	CMD_GET = 4
} commandSignal;
static commandSignal monitorSkill1InternalCmd = CMD_ABSENT;
static commandSignal monitorSkill2InternalCmd = CMD_ABSENT;
static commandSignal monitorSkill3InternalCmd = CMD_ABSENT;
static commandSignal monitorSkill4InternalCmd = CMD_ABSENT;
static commandSignal monitorSkill5InternalCmd = CMD_ABSENT;
static commandSignal monitorSkill6InternalCmd = CMD_ABSENT;

// Skill condition state
typedef enum
{
	SKILL_COND_IDLE = 0,
	SKILL_COND_STARTING = 1,
	SKILL_COND_GET = 2,
	SKILL_COND_SUCCESS = 3,
	SKILL_COND_FAILURE = 4,
	SKILL_COND_RECEIVING_SUCCESS = 5,
	SKILL_COND_RECEIVING_FAILURE = 6
} skillConditionState;
static skillConditionState currentSkill2State = SKILL_COND_IDLE;
static skillConditionState currentSkill3State = SKILL_COND_IDLE;
static skillConditionState currentSkill5State = SKILL_COND_IDLE;
static skillConditionState currentSkill6State = SKILL_COND_IDLE;

// Skill action states
typedef enum
{
	SKILL_ACT_IDLE = 0,
	SKILL_ACT_WAIT_COMMAND = 1,
	SKILL_ACT_GET = 2,
	SKILL_ACT_SUCCESS = 3,
	SKILL_ACT_FAILURE = 4,
	SKILL_ACT_SEND_REQUEST = 5,
	SKILL_ACT_HALT = 6,
	SKILL_ACT_RECEIVING_SUCCESS = 7,
	SKILL_ACT_RECEIVING_FAILURE = 8
} skillActionState;
static skillActionState currentSkill1State = SKILL_ACT_IDLE;
static skillActionState currentSkill4State = SKILL_ACT_IDLE;

// Skill response signals
typedef enum
{
	SIG_ABSENT = 0,
	SIG_SUCCESS = 1,
	SIG_FAILED = 2,
	SIG_RUNNING = 3,
	SIG_STARTED = 4,
	SIG_STOPPED = 5
} skillSignal;
static skillSignal sharedSkill1Status = SIG_ABSENT;
static skillSignal sharedSkill2Status = SIG_ABSENT;
static skillSignal sharedSkill3Status = SIG_ABSENT;
static skillSignal sharedSkill4Status = SIG_ABSENT;
static skillSignal sharedSkill5Status = SIG_ABSENT;
static skillSignal sharedSkill6Status = SIG_ABSENT;

// Component acknowledge signals
typedef enum
{
	COMP_ABSENT = 0,
	COMP_OK = 1,
	COMP_FAIL = 2
} componentSignal;
static componentSignal monitorComp1Ack = COMP_ABSENT; //GoToDestination
static componentSignal monitorComp2Ack = COMP_ABSENT; //BatteryLevel
static componentSignal monitorComp3Ack = COMP_ABSENT; //BatteryNotCharging
static componentSignal monitorComp4Ack = COMP_ABSENT; //GoToChargingStation
static componentSignal monitorComp5Ack = COMP_ABSENT; //AtDestination
static componentSignal monitorComp6Ack = COMP_ABSENT; //AtChargingStation

// Communication Manager for skills
typedef enum
{
	SCM_INIT = 0,
	SCM_WAIT_COMMAND = 1,
	SCM_RETURN_DATA = 2,
	SCM_FWD_START = 3,
	SCM_FWD_STOP = 4,
	SCM_FWD_GET = 5,
	SCM_FWD_OK = 6
} scmState;
static scmState currentSCM1Skill1State = SCM_INIT;
static scmState currentSCM4Skill4State = SCM_INIT;
static scmState currentSCM2Skill2State = SCM_INIT;
static scmState currentSCM3Skill3State = SCM_INIT;
static scmState currentSCM5Skill5State = SCM_INIT;
static scmState currentSCM6Skill6State = SCM_INIT;

// Communication Manager for condition nodes
typedef enum
{
	CCM_COND_IDLE = 0,
	CCM_COND_WAIT_REQUEST = 1,
	CCM_COND_RETURN_DATA = 2,
	CCM_COND_RETURN_STATUS = 3,
	CCM_COND_RETURN_FAIL = 4
} ccmConditionState;
static ccmConditionState currentCCM_BrComponentState = CCM_COND_IDLE;

// Communication Manager for action nodes
typedef enum
{
	CCM_ACT_IDLE = 0,
	CCM_ACT_STARTING = 1,
	CCM_ACT_STOPPING = 2,
	CCM_ACT_RETURN_DATA = 3,
	CCM_ACT_RETURN_FAIL = 4,
	CCM_ACT_RETURN_IS_AT_LOCATION = 5
} ccmActionState;
static ccmActionState currentCCM_GoToComponentState = CCM_ACT_IDLE;

// Component states TODO: this is the navigator
typedef enum
{
	INIT_SIMULATOR = 0,
	WAIT_GO_TO_COMMAND = 1,
	STARTING_GO_TO_DESTINATION = 2,
	GO_TO_DESTINATION = 3,
	DESTINATION_REACHED = 4,
	STOPPING = 5
} componentStatus;
// static componentStatus currentCompState = INIT_SIMULATOR;

//shared variables between CCM and simulator
//buff[0] represent the component ack (componentSignal)
//buff[1] represent the data value shared from component
//and skill (i.e. battery level data, goToStatus value, etc.)
int navigationCompSharedBuffer[3] = {0, 0, 0}; //goTo component
int batteryCompSharedBufferLevel[2] = {0, 0};  //batteryLevel
int batteryCompSharedBufferStatus[2] = {0, 0}; //batteryNotCharging

///////////////////////////////////////////////////////////////////////////
// Utility functions for simulation
///////////////////////////////////////////////////////////////////////////

#if defined(SIMULATE) || defined(SIMULATE_CHIARA) || defined(MONITOR) || defined(MONITOR2) || defined(MONITOR3)

const char *getThreadIdName(thread_id tmp)
{
	switch (tmp)
	{
	case visit_id:
		return "a";
		break;
	case bt_id:
		return "b";
		break;
	case skill1_id:
		return "c";
		break;
	case skill2_id:
		return "d";
		break;
	case skill3_id:
		return "e";
		break;
	case skill4_id:
		return "f";
		break;
	case skill5_id:
		return "g";
		break;
	case skill6_id:
		return "h";
		break;
	case scmCondition2_id:
		return "i";
		break;
	case scmCondition3_id:
		return "l";
		break;
	case scmCondition5_id:
		return "m";
		break;
	case scmCondition6_id:
		return "n";
		break;
	case scmAction1_id:
		return "o";
		break;
	case scmAction4_id:
		return "p";
		break;
	case ccmBr_id:
		return "q";
		break;
	case ccmGoTo_id:
		return "r";
		break;
	case component_id:
		return "u";
		break;
	case absentID:
		return "t";
		break;
	case refresh_id:
		return "s";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getBtSignalName(btSignal tmp)
{
	switch (tmp)
	{
	case TICK:
		return "TICK";
		break;
	case HALT:
		return "HALT";
		break;
	case ABSENT:
		return "ABSENT";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getStatusName(status tmp)
{
	switch (tmp)
	{
	case STATUS_ABSENT:
		return "STATUS_ABSENT";
		break;
	case STATUS_RUNNING:
		return "STATUS_RUNNING";
		break;
	case STATUS_SUCCESS:
		return "STATUS_SUCCESS";
		break;
	case STATUS_FAILURE:
		return "STATUS_FAILURE";
		break;
	case STATUS_IDLE:
		return "STATUS_IDLE";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getActionNodeStateName(actionState tmp)
{
	switch (tmp)
	{
	case ACTION_IDLE:
		return "ACTION_IDLE";
		break;
	case ACTION_START_SKILL:
		return "ACTION_START_SKILL";
		break;
	case ACTION_SKILL_STARTED:
		return "ACTION_SKILL_STARTED";
		break;
	case ACTION_WAIT_ANSWER:
		return "ACTION_WAIT_ANSWER";
		break;
	case ACTION_WAIT_RUNNING:
		return "ACTION_WAIT_RUNNING";
		break;
	case ACTION_RETURN_RUNNING:
		return "ACTION_RETURN_RUNNING";
		break;
	case ACTION_RETURN_SUCCESS:
		return "ACTION_RETURN_SUCCESS";
		break;
	case ACTION_RETURN_FAILURE:
		return "ACTION_RETURN_FAILURE";
		break;
	case ACTION_STOP_SKILL:
		return "ACTION_STOP_SKILL";
		break;
	case ACTION_SKILL_STOPPED:
		return "ACTION_SKILL_STOPPED";
		break;
	case ACTION_RETURN_IDLE:
		return "ACTION_RETURN_IDLE";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getccmConditionStateName(ccmConditionState tmp)
{
	switch (tmp)
	{
	case CCM_COND_IDLE:
		return "CCM_COND_IDLE";
		break;
	case CCM_COND_WAIT_REQUEST:
		return "CCM_COND_WAIT_REQUEST";
		break;
	case CCM_COND_RETURN_DATA:
		return "CCM_COND_RETURN_DATA";
		break;
	case CCM_COND_RETURN_FAIL:
		return "CCM_COND_RETURN_FAIL";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getccmActionStateName(ccmActionState tmp)
{
	switch (tmp)
	{
	case CCM_ACT_IDLE:
		return "CCM_ACT_IDLE";
		break;
	case CCM_ACT_STARTING:
		return "CCM_ACT_STARTING";
		break;
	case CCM_ACT_STOPPING:
		return "CCM_ACT_STOPPING";
		break;
	case CCM_ACT_RETURN_DATA:
		return "CCM_ACT_RETURN_DATA";
		break;
	case CCM_ACT_RETURN_FAIL:
		return "CCM_ACT_RETURN_FAIL";
		break;
	case CCM_ACT_RETURN_IS_AT_LOCATION:
		return "CCM_ACT_RETURN_IS_AT_LOCATION";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getConditionNodeStateName(conditionState tmp)
{
	switch (tmp)
	{
	case CONDITION_IDLE:
		return "CONDITION_IDLE";
		break;
	case CONDITION_START_SKILL:
		return "CONDITION_START_SKILL";
		break;
	case CONDITION_WAIT_ANSWER:
		return "CONDITION_WAIT_ANSWER";
		break;
	case CONDITION_RETURN_SUCCESS:
		return "CONDITION_RETURN_SUCCESS";
		break;
	case CONDITION_RETURN_FAILURE:
		return "CONDITION_RETURN_FAILURE";
		break;
	case CONDITION_RETURN_IDLE:
		return "CONDITION_RETURN_IDLE";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getSkillRequestName(skill_request tmp)
{
	switch (tmp)
	{
	case absent_command:
		return "absent_command";
		break;
	case send_start:
		return "send_start";
		break;
	case send_stop:
		return "send_stop";
		break;
	case send_ok:
		return "send_ok";
		break;
	case request_ack:
		return "request_ack";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getCommandSignalName(commandSignal tmp)
{
	switch (tmp)
	{
	case CMD_START:
		return "CMD_START";
		break;
	case CMD_STOP:
		return "CMD_STOP";
		break;
	case CMD_ABSENT:
		return "CMD_ABSENT";
		break;
	case CMD_OK:
		return "CMD_OK";
		break;
	case CMD_GET:
		return "CMD_GET";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getSkillSignalName(skillSignal tmp)
{
	switch (tmp)
	{
	case SIG_SUCCESS:
		return "SIG_SUCCESS";
		break;
	case SIG_FAILED:
		return "SIG_FAILED";
		break;
	case SIG_ABSENT:
		return "SIG_ABSENT";
		break;
	case SIG_RUNNING:
		return "SIG_RUNNING";
		break;
	case SIG_STARTED:
		return "SIG_STARTED";
		break;
	case SIG_STOPPED:
		return "SIG_STOPPED";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getSkillCondStateName(skillConditionState tmp)
{
	switch (tmp)
	{
	case SKILL_COND_IDLE:
		return "SKILL_COND_IDLE";
		break;
	case SKILL_COND_STARTING:
		return "SKILL_COND_STARTING";
		break;
	case SKILL_COND_GET:
		return "SKILL_COND_GET";
		break;
	case SKILL_COND_SUCCESS:
		return "SKILL_COND_SUCCESS";
		break;
	case SKILL_COND_FAILURE:
		return "SKILL_COND_FAILURE";
		break;
	case SKILL_COND_RECEIVING_SUCCESS:
		return "SKILL_COND_RECEIVING_SUCCESS";
		break;
	case SKILL_COND_RECEIVING_FAILURE:
		return "SKILL_COND_RECEIVING_FAILURE";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getSkillActStateName(skillActionState tmp)
{
	switch (tmp)
	{
	case SKILL_ACT_IDLE:
		return "SKILL_ACT_IDLE";
		break;
	case SKILL_ACT_WAIT_COMMAND:
		return "SKILL_ACT_WAIT_COMMAND";
		break;
	case SKILL_ACT_GET:
		return "SKILL_ACT_GET";
		break;
	case SKILL_ACT_SUCCESS:
		return "SKILL_ACT_SUCCESS";
		break;
	case SKILL_ACT_FAILURE:
		return "SKILL_ACT_FAILURE";
		break;
	case SKILL_ACT_SEND_REQUEST:
		return "SKILL_ACT_SEND_REQUEST";
		break;
	case SKILL_ACT_HALT:
		return "SKILL_ACT_HALT";
		break;
	case SKILL_ACT_RECEIVING_SUCCESS:
		return "SKILL_ACT_RECEIVING_SUCCESS";
		break;
	case SKILL_ACT_RECEIVING_FAILURE:
		return "SKILL_ACT_RECEIVING_FAILURE";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getComponentSignalName(componentSignal tmp)
{
	switch (tmp)
	{
	case COMP_OK:
		return "COMP_OK";
		break;
	case COMP_FAIL:
		return "COMP_FAIL";
		break;
	case COMP_ABSENT:
		return "COMP_ABSENT";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getComponentStatelName(componentStatus tmp)
{
	switch (tmp)
	{
	case INIT_SIMULATOR:
		return "INIT_SIMULATOR";
		break;
	case WAIT_GO_TO_COMMAND:
		return "WAIT_GO_TO_COMMAND";
		break;
	case STARTING_GO_TO_DESTINATION:
		return "STARTING_GO_TO_DESTINATION";
		break;
	case GO_TO_DESTINATION:
		return "GO_TO_DESTINATION";
		break;
	case DESTINATION_REACHED:
		return "DESTINATION_REACHED";
		break;
	case STOPPING:
		return "STOPPING";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getBatteryNotChargingName(ChargingStatus tmp)
{
	switch (tmp)
	{
	case BATTERY_NOT_CHARGING:
		return "BATTERY_NOT_CHARGING";
		break;
	case BATTERY_CHARGING:
		return "BATTERY_CHARGING";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getGoToStatusName(GoToStatus tmp)
{
	switch (tmp)
	{
	case NOT_STARTED:
		return "NOT_STARTED";
		break;
	case RUNNING:
		return "RUNNING";
		break;
	case SUCCESS:
		return "SUCCESS";
		break;
	case ABORT:
		return "ABORT";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getBatteryReaderFuncName(BatteryReaderComponentFunction tmp)
{
	switch (tmp)
	{
	case batteryReader_absentFunction:
		return "batteryReader_absentFunction";
		break;
	case level:
		return "level";
		break;
	case charging_status:
		return "charging_status";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getGoToFuncName(GoToComponentFunction tmp)
{
	switch (tmp)
	{
	case goTo_absentFunction:
		return "goTo_absentFunction";
		break;
	case goTo:
		return "goTo";
		break;
	case getStatus:
		return "getStatus";
		break;
	case halt:
		return "halt";
		break;
	case isAtLocation:
		return "isAtLocation";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getNavigationCommandName(NavigationCommand tmp)
{
	switch (tmp)
	{
	case navigation_absentCmd:
		return "navigation_absentCmd";
		break;
	case start_navigation:
		return "start_navigation";
		break;
	case getStatus_navigation:
		return "getStatus_navigation";
		break;
	case stop_navigation:
		return "stop_navigation";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

const char *getDestinationName(destination tmp)
{
	switch (tmp)
	{
	case ABSENT_LOCATION:
		return "ABSENT_LOCATION";
		break;
	case KITCHEN:
		return "KITCHEN";
		break;
	case CHARGING_STATION:
		return "CHARGING_STATION";
		break;
	default:
		return "SOMETHING_WRONG";
		break;
	}
}

char *toLower(const char *str, size_t len)
{
    char *str_l = calloc(len+1, sizeof(char));

    for (size_t i = 0; i < len; ++i) {
        str_l[i] = tolower((unsigned char)str[i]);
    }
    return str_l;
}

#endif

///////////////////////////////////////////////////////////////////////////
// Utility functions to handle circular buffers
///////////////////////////////////////////////////////////////////////////

void insertToTail(int rows, int cols, int aBuffer[rows][cols], int *front, int *rear, int *item, int *counter)
{
	if ((*front == 0 && *rear == rows - 1) || (*front == *rear + 1))
	{
		return;
	}
	if (*front == -1)
	{
		*front = 0;
		*rear = 0;
	}
	else
	{
		if (*rear == rows - 1)
			*rear = 0;
		else
			*rear = *rear + 1;
	}

	for (int j = 0; j < cols; j++)
	{
		aBuffer[*rear][j] = item[j];
	}

	*counter += 1;
}

void deleteFromFront(int rows, int cols, int aBuffer[rows][cols], int *front, int *rear, int *retBuff, int *counter)
{
	if (*front == -1)
	{
		return;
	}
	for (int j = 0; j < cols; j++)
	{
		retBuff[j] = aBuffer[*front][j];
	}

	*counter -= 1;

	if (*front == *rear)
	{
		*front = -1;
		*rear = -1;
	}
	else
	{
		if (*front == rows - 1)
			*front = 0;
		else
			*front = *front + 1;
	}
}

void readFromFront(int rows, int cols, int aBuffer[rows][cols], int *front, int *rear, int *retBuff)
{
	for (int j = 0; j < cols; j++)
	{
		retBuff[j] = aBuffer[*front][j];
	}
}

///////////////////////////////////////////////////////////////////////////
// Monitor functions
///////////////////////////////////////////////////////////////////////////

#if defined(MONITOR)
typedef enum
{
	MONITOR1_CHECK_BATTERY = 1,
	MONITOR1_ERROR = 6
} monitor1Status;
static monitor1Status currentMonitor1Status = MONITOR1_CHECK_BATTERY;
static componentSignal _monitor1BatteryCompAck = COMP_ABSENT;
static int _monitor1BatteryLevel = 1000;
void propertyOne()
{
	switch (currentMonitor1Status)
	{
	case MONITOR1_CHECK_BATTERY:
		_monitor1BatteryCompAck = bufferCCM_BRToSkill2[0];
		_monitor1BatteryLevel = bufferCCM_BRToSkill2[1];
		//const char* _batteryCompAckName = getComponentSignalName(_monitor1BatteryCompAck);
		if (_monitor1BatteryCompAck == COMP_OK)
		{
			//printf("			Monitor1 status: MONITOR1_CHECK_BATTERY  \n" );
			//printf("			CompAck = %s -- BatteryLevel : %d\n",_batteryCompAckName,_monitor1BatteryLevel);
			if (_monitor1BatteryLevel >= 200)
			{
				currentMonitor1Status = MONITOR1_CHECK_BATTERY;
			}
			else
			{
				currentMonitor1Status = MONITOR1_ERROR;
			}
		}
		break;

	case MONITOR1_ERROR:
		printf("			Monitor1 status: MONITOR1_ERROR  \n");
		printf("			BatteryLevel under 20 percent \n ");
		assert(false);
		break;

	default:
		break;
	}

	// printf("BatteryLevel : %d \n ",bufferCCM_BRToSkill2[1]);
	// if(bufferCCM_BRToSkill2[1]<20){
	// 	printf("BatteryLevel under 20 percent \n ");
	// 	assert(false);
	// }
}

#endif

#if defined(MONITOR2)
typedef enum
{
	MONITOR2_IDLE = 0,
	MONITOR2_CHECK_BATTERY = 1,
	MONITOR2_START_TIMER = 2,
	MONITOR2_DECREASE_TIMER = 3,
	MONITOR2_CHECK_COMMAND = 4,
	MONITOR2_CHECK_TIMER = 5,
	MONITOR2_ERROR = 6
} monitor2Status;
static monitor2Status currentMonitor2Status = MONITOR2_IDLE;
static int monitor2_max_timer = 0;
static int monitor2_tickCounter = 0;
static bool monitor2_timerStatus = false;
static int _monitor2BatteryLevel = 1000;
static componentSignal _monitor2BatteryCompAck = COMP_ABSENT;
void propertyTwo()
{

	switch (currentMonitor2Status)
	{
	case MONITOR2_IDLE:
		printf("			Monitor2 status:  MONITOR2_IDLE \n");
		const char *NavigationResponse = getGoToStatusName(bufferCCM_GoToToSkill1[1]);
		printf("			Response from navigation : %s \n", NavigationResponse);
		if (bufferCCM_GoToToSkill1[1] == RUNNING)
		{
			currentMonitor2Status = MONITOR2_CHECK_BATTERY;
			pthread_mutex_lock(&mutexMonitor);
			monitor2_timerIsArmed = false;
			monitor2_timerStatus = monitor2_timerIsArmed;
			monitor2_tickCounter = countLoop;
			monitor2_max_timer = 0;
			pthread_mutex_unlock(&mutexMonitor);
		}
		break;

	case MONITOR2_CHECK_BATTERY:
		_monitor2BatteryCompAck = bufferCCM_BRToSkill2[0];
		_monitor2BatteryLevel = bufferCCM_BRToSkill2[1];
		const char *_monitor2BatteryCompAckName = getComponentSignalName(_monitor2BatteryCompAck);
		if (_monitor2BatteryCompAck == COMP_OK)
		{
			printf("			Monitor2 status: MONITOR2_CHECK_BATTERY  \n");
			printf("			CompAck = %s -- BatteryLevel : %d\n", _monitor2BatteryCompAckName, _monitor2BatteryLevel);
			if (_monitor2BatteryLevel <= 300)
			{
				currentMonitor2Status = MONITOR2_START_TIMER;
			}
		}

		break;

	case MONITOR2_START_TIMER:
		printf("			Monitor2 status: MONITOR2_START_TIMER  \n");
		pthread_mutex_lock(&mutexMonitor);
		monitor2_timerIsArmed = true;
		monitor2_timerStatus = monitor2_timerIsArmed;
		monitor2_tickCounter = countLoop;
		//with 2 tick counter goes to timeout if battery reader skill condition is >20
		//Changing the value of the condition from 30 to 20, the skill takes 5
		//ticks to go from 30% to 20% (-2 at each tick)
		monitor2_max_timer = monitor2_tickCounter + 3;
		pthread_mutex_unlock(&mutexMonitor);
		printf("			Tick counter = %d -- maxTimer = %d -- timerIsArmed = %s \n", monitor2_tickCounter, monitor2_max_timer, monitor2_timerStatus == true ? "TRUE" : "FALSE");
		currentMonitor2Status = MONITOR2_CHECK_COMMAND;
		break;

	case MONITOR2_CHECK_COMMAND:
		printf("			Monitor2 status: MONITOR2_CHECK_COMMAND  \n");
		//variable used to store skill message locally
		//_monitor2_messageFromSkill[0] = GoToComponentFunction
		//_monitor2_messageFromSkill[1] = thread_id
		//_monitor2_messageFromSkill[2] = destination
		static int _monitor2_messageFromSkill[3];
		readFromFront(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _monitor2_messageFromSkill);
		if (_monitor2_messageFromSkill[0] == start_navigation && _monitor2_messageFromSkill[2] == CHARGING_STATION)
		{
			currentMonitor2Status = MONITOR2_IDLE;
			pthread_mutex_lock(&mutexMonitor);
			monitor2_timerIsArmed = false;
			monitor2_timerStatus = monitor2_timerIsArmed;
			monitor2_tickCounter = countLoop;
			monitor2_max_timer = 0;
			pthread_mutex_unlock(&mutexMonitor);
			printf("			Checkcommand:  start_navigation to  CHARGING_STATION has been received \n");
		}
		else
		{
			currentMonitor2Status = MONITOR2_CHECK_TIMER;
		}
		break;

	case MONITOR2_CHECK_TIMER:
		printf("			Monitor2 status: MONITOR2_CHECK_TIMER  \n");
		pthread_mutex_lock(&mutexMonitor);
		monitor2_tickCounter = countLoop;
		pthread_mutex_unlock(&mutexMonitor);
		printf("			Tick Value = %d \n", monitor2_tickCounter);
		if (monitor2_tickCounter == monitor2_max_timer)
		{
			currentMonitor2Status = MONITOR2_ERROR;
		}
		else
		{
			currentMonitor2Status = MONITOR2_CHECK_COMMAND;
		}
		break;

	case MONITOR2_DECREASE_TIMER:
		printf("			Monitor2 status: MONITOR2_INCREASE_TIMER  \n");

		break;

	case MONITOR2_ERROR:
		printf("			Monitor2 status: MONITOR2_ERROR  \n");
		printf("			Timer is over!!   \n ");
		assert(false);
		break;

	default:
		break;
	}
}
#endif

#if defined(MONITOR3)
typedef enum
{
	MONITOR3_IDLE = 0,
	MONITOR3_CHECK_BATTERY = 1,
	MONITOR3_START_TIMER = 2,
	MONITOR3_DECREASE_TIMER = 3,
	MONITOR3_CHECK_COMMAND = 4,
	MONITOR3_CHECK_TIMER = 5,
	MONITOR3_ERROR = 6
} monitor3Status;
static monitor3Status currentMonitor3Status = MONITOR3_IDLE;
static int monitor3_max_timer = 0;
static int monitor3_tickCounter = 0;
static bool monitor3_timerStatus = false;
static int _monitor3BatteryLevel = 100;
static componentSignal _monitor3BatteryCompAck = COMP_ABSENT;
static int _monitor3AtDestinationValue = 0;
static componentSignal _monitor3AtDestinationCompAck = COMP_ABSENT;
void propertyThree()
{
	switch (currentMonitor3Status)
	{
	case MONITOR3_IDLE:
		printf("			Monitor3 status:  MONITOR3_IDLE \n");
		const char *NavigationResponse = getGoToStatusName(bufferCCM_GoToToSkill1[1]);
		printf("			Response from navigation : %s \n", NavigationResponse);
		if (bufferCCM_GoToToSkill1[1] == RUNNING)
		{
			currentMonitor3Status = MONITOR3_CHECK_BATTERY;
			pthread_mutex_lock(&mutexMonitor);
			monitor3_timerIsArmed = false;
			monitor3_timerStatus = monitor3_timerIsArmed;
			monitor3_tickCounter = countLoop;
			monitor3_max_timer = 0;
			pthread_mutex_unlock(&mutexMonitor);
		}
		break;

	case MONITOR3_CHECK_BATTERY:
		_monitor3BatteryCompAck = bufferCCM_BRToSkill2[0];
		_monitor3BatteryLevel = bufferCCM_BRToSkill2[1];
		const char *_monitor3BatteryCompAckName = getComponentSignalName(_monitor3BatteryCompAck);
		if (_monitor3BatteryCompAck == COMP_OK)
		{
			printf("			Monitor3 status: MONITOR3_CHECK_BATTERY  \n");
			printf("			CompAck = %s -- BatteryLevel : %d\n", _monitor3BatteryCompAckName, _monitor3BatteryLevel);
			if (_monitor3BatteryLevel <= 1050 && _monitor3BatteryLevel >= 950)
			{
				currentMonitor3Status = MONITOR3_START_TIMER;
			}
			else
			{
				currentMonitor3Status = MONITOR3_IDLE;
			}
		}
		break;

	case MONITOR3_START_TIMER:
		printf("			Monitor3 status: MONITOR3_START_TIMER  \n");
		pthread_mutex_lock(&mutexMonitor);
		monitor3_timerIsArmed = true;
		monitor3_timerStatus = monitor3_timerIsArmed;
		monitor3_tickCounter = countLoop;
		//with 2 tick counter goes to timeout if battery reader skill condition is >20
		//Changing the value of the condition from 30 to 20, the skill takes 5
		//ticks to go from 30% to 20% (-2 at each tick)
		monitor3_max_timer = monitor3_tickCounter + 30;
		pthread_mutex_unlock(&mutexMonitor);
		printf("			Tick counter = %d -- maxTimer = %d -- timerIsArmed = %s \n", monitor3_tickCounter, monitor3_max_timer, monitor3_timerStatus == true ? "TRUE" : "FALSE");
		currentMonitor3Status = MONITOR3_CHECK_COMMAND;
		break;

	case MONITOR3_CHECK_COMMAND:
		printf("			Monitor3 status: MONITOR3_CHECK_COMMAND  \n");
		_monitor3AtDestinationCompAck = bufferCCM_GoToToSkill5[0];
		_monitor3AtDestinationValue = bufferCCM_GoToToSkill5[1];
		if (_monitor3AtDestinationCompAck == COMP_OK && _monitor3AtDestinationValue == 1)
		{
			currentMonitor3Status = MONITOR3_IDLE;
			pthread_mutex_lock(&mutexMonitor);
			monitor3_timerIsArmed = false;
			monitor3_timerStatus = monitor3_timerIsArmed;
			monitor3_tickCounter = countLoop;
			monitor3_max_timer = 0;
			pthread_mutex_unlock(&mutexMonitor);
			printf("			Checkcommand:  Robot is at Destination \n");
		}
		else
		{
			currentMonitor3Status = MONITOR3_CHECK_TIMER;
		}
		break;

	case MONITOR3_CHECK_TIMER:
		printf("			Monitor3 status: MONITOR3_CHECK_TIMER  \n");
		pthread_mutex_lock(&mutexMonitor);
		monitor3_tickCounter = countLoop;
		pthread_mutex_unlock(&mutexMonitor);
		printf("			Tick Value = %d \n", monitor3_tickCounter);
		if (monitor3_tickCounter == monitor3_max_timer)
		{
			currentMonitor3Status = MONITOR3_ERROR;
		}
		else
		{
			currentMonitor3Status = MONITOR3_CHECK_COMMAND;
		}
		break;

	case MONITOR3_ERROR:
		printf("			Monitor3 status: MONITOR3_ERROR  \n");
		printf("			Timer is over!!   \n ");
		assert(false);
		break;

	default:
		break;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////
// Utility functions to handle channels with different data types
///////////////////////////////////////////////////////////////////////////

//functions
void setValueToBtFromVisit(btSignal value, int *bufferFromUp, int *counter)
{
	bufferFromUp[0] = value;
	*counter += 1;
	;
}

status getResultFromBt(int *bufferToUp, int *counter)
{
	status tmp = bufferToUp[0];
	*counter -= 1;
	return tmp;
}

btSignal getValueFromVisit(int *bufferFromUp, int *counter)
{
	btSignal tmp = bufferFromUp[0];
	*counter -= 1;
	return tmp;
}

void writeResultToVisit(status res, int *bufferToUp, int *counter)
{
	bufferToUp[0] = res;
	*counter += 1;
}

void setSkillRequestAndCounter(skill_request value, int *bufferFromUp, skill_request *currCondSignal, int *counter)
{
	bufferFromUp[0] = value;
	*currCondSignal = value;
	*counter += 1;
}

void setCommandValueAndCounter(commandSignal value, int *bufferFromUp, commandSignal *currCondSignal, int *counter)
{
	bufferFromUp[0] = value;
	*currCondSignal = value;
	*counter += 1;
}

void setNavigationCmdAndCounter(NavigationCommand value, destination dest, int *bufferFromUp, NavigationCommand *currCondSignal, int *counter)
{
	bufferFromUp[0] = value;
	bufferFromUp[1] = dest;
	*currCondSignal = value;
	*counter += 1;
}

void setNavigationCmd(NavigationCommand value, destination dest, int *bufferFromUp, NavigationCommand *currCondSignal)
{
	bufferFromUp[0] = value;
	bufferFromUp[1] = dest;
	*currCondSignal = value;
}

void setComponentAckDataIntAndCounter(componentSignal value, int aData, int *bufferFromUp, componentSignal *currCompSignal, int *counter)
{
	bufferFromUp[0] = value;
	bufferFromUp[1] = aData;
	*currCompSignal = value;
	*counter += 1;
}

void setComponentAckGoToStatusAndCounter(componentSignal value, GoToStatus aData, int *bufferFromUp, componentSignal *currCompSignal, int *counter)
{
	bufferFromUp[0] = value;
	bufferFromUp[1] = aData;
	*currCompSignal = value;
	*counter += 1;
}

void setComponentAckIsAtLocationAndCounter(componentSignal value, int aData, int *bufferFromUp, componentSignal *currCompSignal, int *counter)
{
	bufferFromUp[0] = value;
	bufferFromUp[1] = aData;
	*currCompSignal = value;
	*counter += 1;
}

void setSkillAckAndCounter(skillSignal *currSkillSignal, int *bufferFromUp, int *counter)
{
	bufferFromUp[0] = *currSkillSignal;
	*counter += 1;
}

commandSignal watchCommandValue(int *bufferFromUp, pthread_mutex_t *mutexVar)
{
	pthread_mutex_lock(mutexVar);
	commandSignal tmp = bufferFromUp[0];
	pthread_mutex_unlock(mutexVar);
	return tmp;
}

componentSignal getCompAck(int *bufferToUp)
{
	componentSignal tmp = bufferToUp[0];
	bufferToUp[0] = COMP_ABSENT;
	return tmp;
}

GoToStatus getGoToStatus(int *channelBuffer)
{
	GoToStatus tmp = channelBuffer[1];
	channelBuffer[1] = NOT_STARTED;
	return tmp;
}

NavigationCommand getNavigationCommand(int *channelBuffer)
{
	NavigationCommand tmp = channelBuffer[0];
	channelBuffer[0] = navigation_absentCmd;
	return tmp;
}

NavigationCommand watchNavigationCommandValue(int *bufferFromUp, pthread_mutex_t *mutexVar)
{
	pthread_mutex_lock(mutexVar);
	NavigationCommand tmp = bufferFromUp[0];
	pthread_mutex_unlock(mutexVar);
	return tmp;
}

destination watchDestinationValue(int *bufferFromUp, pthread_mutex_t *mutexVar)
{
	pthread_mutex_lock(mutexVar);
	destination tmp = bufferFromUp[1];
	pthread_mutex_unlock(mutexVar);
	return tmp;
}

destination getDestination(int *channelBuffer)
{
	destination tmp = channelBuffer[1];
	channelBuffer[1] = ABSENT_LOCATION;
	return tmp;
}

ChargingStatus getChargingStatus(int *channelBuffer)
{
	ChargingStatus tmp = channelBuffer[1];
	channelBuffer[1] = 0;
	return tmp;
}

int getLevel(int *channelBuffer)
{
	int tmp = channelBuffer[1];
	channelBuffer[1] = 0;
	return tmp;
}

int getIsAtLocation(int *channelBuffer)
{
	int tmp = channelBuffer[1];
	//channelBuffer[1] = 0;
	return tmp;
}

skillSignal getValueFromSkillAndWakeupNoMutex(int *bufferToUp, int *counter, pthread_cond_t *condVar)
{
	skillSignal tmp = bufferToUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferToUp[0] = SIG_ABSENT;
	return tmp;
}

commandSignal getCommandValueAndWakeUp(int *bufferFromUp, int *counter, pthread_mutex_t *mutexVar, pthread_cond_t *condVar)
{
	pthread_mutex_lock(mutexVar);
	commandSignal tmp = bufferFromUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferFromUp[0] = CMD_ABSENT;
	pthread_mutex_unlock(mutexVar);
	return tmp;
}

commandSignal getCommandValueAndWakeUpNoMutex(int *bufferFromUp, int *counter, pthread_cond_t *condVar)
{
	commandSignal tmp = bufferFromUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferFromUp[0] = CMD_ABSENT;
	return tmp;
}

skill_request getSkillRequestAndWakeUpNoMutex(int *bufferFromUp, int *counter, pthread_cond_t *condVar)
{
	skill_request tmp = bufferFromUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferFromUp[0] = absent_command;
	return tmp;
}

NavigationCommand getNavigationCmdValueAndWakeUp(int *bufferFromUp, int *counter, pthread_mutex_t *mutexVar, pthread_cond_t *condVar)
{
	pthread_mutex_lock(mutexVar);
	NavigationCommand tmp = bufferFromUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferFromUp[0] = navigation_absentCmd;
	bufferFromUp[1] = ABSENT_LOCATION;
	pthread_mutex_unlock(mutexVar);
	return tmp;
}

NavigationCommand getNavigationCmdValueAndWakeUpNoMutex(int *bufferFromUp, int *counter, pthread_cond_t *condVar)
{
	NavigationCommand tmp = bufferFromUp[0];
	*counter -= 1;
	pthread_cond_signal(condVar);
	bufferFromUp[0] = navigation_absentCmd;
	bufferFromUp[1] = ABSENT_LOCATION;
	return tmp;
}

void resetNavigationCmdValue(int *bufferFromUp)
{
	bufferFromUp[0] = navigation_absentCmd;
	bufferFromUp[1] = ABSENT_LOCATION;
}

void sendSignalAndDecreaseCounter(pthread_cond_t *condVar, int *counter)
{
	*counter -= 1;
	pthread_cond_signal(condVar);
}

///////////////////////////////////////////////////////////////////////////
// BT Function prototypes
///////////////////////////////////////////////////////////////////////////

// Inner nodes
status sequence1(btSignal visit);
status fallback1(btSignal visit);
status fallback2(btSignal visit);
status sequence2(btSignal visit);
status sequence3(btSignal visit);
status fallback3(btSignal visit);

// Leaves
status AtDestination(btSignal visit);
status GoToDestination(btSignal visit);
status BatteryLevel(btSignal visit);
status BatteryNotCharging(btSignal visit);
status AtChargingStation(btSignal visit);
status GoToChargingStation(btSignal visit);
status AlwaysRunning(btSignal visit);

///////////////////////////////////////////////////////////////////////////
// BT control nodes
///////////////////////////////////////////////////////////////////////////

status sequence1(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("SEQUENCE1: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = fallback1(TICK);
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("SEQUENCE1: FROM LEFT FAILURE/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = fallback2(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("SEQUENCE1: FROM LEFT FAILURE/RUNNING and FROM RIGHT != RUNNING -->> RETURN LEFT\n");
			fflush(stdout);
#endif
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_SUCCESS)
		{
#ifdef SIMULATE
			printf("SEQUENCE1: FROM LEFT SUCCESS -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = fallback2(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE1: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = fallback1(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE1: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = fallback2(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

status fallback1(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("FALLBACK1: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = sequence2(TICK);
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK1: FROM LEFT SUCCESS/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = sequence3(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK1: FROM LEFT SUCCESS/RUNNING -->> RETURN LEFT\n");
			fflush(stdout);
#endif
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_FAILURE)
		{
#ifdef SIMULATE
			printf("FALLBACK1: FROM LEFT FAILURE -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = sequence3(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK1: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = sequence2(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK1: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = sequence3(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

status fallback2(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("FALLBACK2: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = AtDestination(TICK);
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK2: FROM LEFT SUCCESS/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = GoToDestination(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK2: FROM LEFT SUCCESS/RUNNING -->> RETURN LEFT\n");
			fflush(stdout);
#endif
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_FAILURE)
		{
#ifdef SIMULATE
			printf("FALLBACK2: FROM LEFT FAILURE -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = GoToDestination(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK2: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = AtDestination(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK2: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = GoToDestination(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

status sequence2(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("SEQUENCE2: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = BatteryLevel(TICK);
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("SEQUENCE2: FROM LEFT FAILURE/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = BatteryNotCharging(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_SUCCESS)
		{
#ifdef SIMULATE
			printf("SEQUENCE2: FROM LEFT SUCCESS -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = BatteryNotCharging(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE2: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = BatteryLevel(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE2: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = BatteryNotCharging(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

status sequence3(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("SEQUENCE3: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = fallback3(TICK);
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("SEQUENCE3: FROM LEFT FAILURE/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = AlwaysRunning(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_FAILURE || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_SUCCESS)
		{
#ifdef SIMULATE
			printf("SEQUENCE3: FROM LEFT SUCCESS -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = AlwaysRunning(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE3: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = fallback3(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("SEQUENCE3: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = AlwaysRunning(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

status fallback3(btSignal visit)
{
	static status _from_left = STATUS_ABSENT;
	static status _from_right = STATUS_ABSENT;
	// Wait
	if (visit == TICK)
	{
#ifdef SIMULATE
		printf("FALLBACK3: TICK LEFT\n");
		fflush(stdout);
#endif
		// Tick left
		_from_left = AtChargingStation(TICK);
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right == STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK3: FROM LEFT SUCCESS/RUNNING -->> HALT RIGHT\n");
			fflush(stdout);
#endif
			// Left HALT right
			do
			{
				_from_right = GoToChargingStation(HALT);
			} while (_from_right != STATUS_IDLE);
			// Return Left
			return _from_left;
		}
		if ((_from_left == STATUS_SUCCESS || _from_left == STATUS_RUNNING) &&
			_from_right != STATUS_RUNNING)
		{
#ifdef SIMULATE
			printf("FALLBACK3: FROM LEFT SUCCESS/RUNNING -->> RETURN LEFT\n");
			fflush(stdout);
#endif
			// Return Left
			return _from_left;
		}
		if (_from_left == STATUS_FAILURE)
		{
#ifdef SIMULATE
			printf("FALLBACK3: FROM LEFT FAILURE -->> TICK RIGHT\n");
			fflush(stdout);
#endif
			// Tick right
			_from_right = GoToChargingStation(TICK);
			// Return right
			return _from_right;
		}
	}
	if (visit == HALT && _from_left == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK3: HALT and FROM LEFT RUNNING -->> HALT LEFT\n");
		fflush(stdout);
#endif
		// Halt left
		do
		{
			_from_left = AtChargingStation(HALT);
		} while (_from_left != STATUS_IDLE);
		// Return halted left
		return STATUS_IDLE;
	}
	if (visit == HALT && _from_right == STATUS_RUNNING)
	{
#ifdef SIMULATE
		printf("FALLBACK3: HALT and FROM RIGHT RUNNING -->> HALT RIGHT\n");
		fflush(stdout);
#endif
		// Halt right
		do
		{
			_from_right = GoToChargingStation(HALT);
		} while (_from_right != STATUS_IDLE);
		// Return halted right
		return STATUS_IDLE;
	}
	return STATUS_ABSENT;
}

///////////////////////////////////////////////////////////////////////////
// BT leaf nodes
///////////////////////////////////////////////////////////////////////////

// Connection between BT and condition/action skills FSM
status BatteryLevel(btSignal visit)
{
	static skill_request _conditionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;

#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
		switch (currentCondition2State)
		{
		case CONDITION_IDLE:
			//wait for visit
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				//start the skill
				currentCondition2State = CONDITION_START_SKILL;
			}
			else if (visit == HALT)
			{
				//the halt command is managed directly from BatteryLevel node
				//stop command will be not forwarded to the skill
				currentCondition2State = CONDITION_RETURN_IDLE;
			}

			break;

		case CONDITION_START_SKILL:
			//send start to skill and wait for started ack
			//The started ack is encoded by pthread signal
#ifdef SIMULATE
			printf("	%s Condition state = START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCondition2ToSCM2);
			// send  send_start command to skill protocol
			_conditionCmd = send_start;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition2ToSCM2, &monitorCondition2Cmd, &countCondition2ToSCM2);
#ifdef SIMULATE
			const char *sigToSkillS = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillS);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition2ToSCM2);
			// wait until the command has been read
			while (countCondition2ToSCM2 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition2ToSCM2, &mutexCondition2ToSCM2);
			}
			//here the FSM knows that the start command has been read
			pthread_mutex_unlock(&mutexCondition2ToSCM2);
			//leaf node is sure only that the skill has read the signal
			//The leaf node checks the skill status in the CONDITION_WAIT_ANSWER state
			currentCondition2State = CONDITION_WAIT_ANSWER;
			break;

		case CONDITION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The condition skill status can only be SUCCESS or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexCondition2ToSCM2);
			// send request_ack command to SCM
			_conditionCmd = request_ack;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition2ToSCM2, &monitorCondition2Cmd, &countCondition2ToSCM2);
#ifdef SIMULATE
			const char *sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition2ToSCM2);
			// wait until the command has been read
			while (countCondition2ToSCM2 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition2ToSCM2, &mutexCondition2ToSCM2);
			}
			//here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexCondition2ToSCM2);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM2ToCondition2);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM2ToCondition2 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM2ToCondition2, &mutexSCM2ToCondition2);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM2ToCondition2, &countSCM2ToCondition2, &condFullFromSCM2ToCondition2);
			pthread_mutex_unlock(&mutexSCM2ToCondition2);

#ifdef SIMULATE
			//const char* watchedValFromSkill = getSkillSignalName(_skillStatus);
			//printf("    %s Signal status from Skill is  :%s:  \n",funcName,  watchedValFromSkill );
			//fflush(stdout);
#endif
			//based on signal value, go to RETURN_SUCCESS or RETURN_FAILURE
			//RETURN_SUCCESS and RETURN_FAILURE need to wakeup the skill
			//by sending pthread signal
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentCondition2State = CONDITION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentCondition2State = CONDITION_RETURN_FAILURE;
				break;

			default:
				break;
			}
			break;

		case CONDITION_RETURN_SUCCESS:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition2ToSCM2);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition2ToSCM2, &monitorCondition2Cmd, &countCondition2ToSCM2);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition2ToSCM2);
			// wait until the command has been read
			while (countCondition2ToSCM2 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition2ToSCM2, &mutexCondition2ToSCM2);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition2ToSCM2);
			//set the next state (go to IDLE)
			currentCondition2State = CONDITION_IDLE;
			return STATUS_SUCCESS;
			//break;

		case CONDITION_RETURN_FAILURE:
			//signals success to BT and wakeup skill
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition2ToSCM2);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition2ToSCM2, &monitorCondition2Cmd, &countCondition2ToSCM2);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition2ToSCM2);
			// wait until the command has been read
			while (countCondition2ToSCM2 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition2ToSCM2, &mutexCondition2ToSCM2);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition2ToSCM2);
			//set the next state (go to IDLE)
			currentCondition2State = CONDITION_IDLE;
			return STATUS_FAILURE;
			//break;

		case CONDITION_RETURN_IDLE:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentCondition2State = CONDITION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status BatteryNotCharging(btSignal visit)
{
	static skill_request _conditionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
		switch (currentCondition3State)
		{
		case CONDITION_IDLE:
			//wait for tick
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				currentCondition3State = CONDITION_START_SKILL;
			}
			else if (visit == HALT)
			{
				currentCondition3State = CONDITION_RETURN_IDLE;
			}
			break;

		case CONDITION_START_SKILL:
			//send start to skill and wait for started signal
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCondition3ToSCM3);
			// send  send_start command to skill protocol
			_conditionCmd = send_start;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition3ToSCM3, &monitorCondition3Cmd, &countCondition3ToSCM3);
#ifdef SIMULATE
			const char *sigToSkill = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition3ToSCM3);
			// wait until the command has been read
			while (countCondition3ToSCM3 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition3ToSCM3, &mutexCondition3ToSCM3);
			}
			//here the FSM knows that the start command has been read
			pthread_mutex_unlock(&mutexCondition3ToSCM3);
			//leaf node is sure only that the skill has read the signal
			//The leaf node checks the skill status in the CONDITION_WAIT_ANSWER state
			currentCondition3State = CONDITION_WAIT_ANSWER;
			break;

		case CONDITION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The condition skill status can only be SUCCESS or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexCondition3ToSCM3);
			// send request_ack command to SCM
			_conditionCmd = request_ack;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition3ToSCM3, &monitorCondition3Cmd, &countCondition3ToSCM3);
#ifdef SIMULATE
			const char *sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition3ToSCM3);
			while (countCondition3ToSCM3 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition3ToSCM3, &mutexCondition3ToSCM3);
			}
			//here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexCondition3ToSCM3);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM3ToCondition3);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM3ToCondition3 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM3ToCondition3, &mutexSCM3ToCondition3);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM3ToCondition3, &countSCM3ToCondition3, &condFullFromSCM3ToCondition3);
			pthread_mutex_unlock(&mutexSCM3ToCondition3);

#ifdef SIMULATE
			//const char* watchedValFromSkill = getSkillSignalName(_skillStatus);
			//printf("    %s Signal status from Skill is  :%s:  \n",funcName,  watchedValFromSkill );
			//fflush(stdout);
#endif
			//based on signal value, go to  CONDITION_RETURN_SUCCESS or CONDITION_RETURN_FAILURE
			//CONDITION_RETURN_SUCCESS and CONDITION_RETURN_FAILURE need to wakeup the skill
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentCondition3State = CONDITION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentCondition3State = CONDITION_RETURN_FAILURE;
				break;

			default:
				break;
			}
			break;

		case CONDITION_RETURN_SUCCESS:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition3ToSCM3);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition3ToSCM3, &monitorCondition3Cmd, &countCondition3ToSCM3);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition3ToSCM3);
			// wait until the command has been read
			while (countCondition3ToSCM3 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition3ToSCM3, &mutexCondition3ToSCM3);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition3ToSCM3);
			//set the next state (go to IDLE)
			currentCondition3State = CONDITION_IDLE;
			return STATUS_SUCCESS;
			//break;

		case CONDITION_RETURN_FAILURE:
			//signals success to BT and wakeup skill
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition3ToSCM3);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition3ToSCM3, &monitorCondition3Cmd, &countCondition3ToSCM3);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition3ToSCM3);
			// wait until the command has been read
			while (countCondition3ToSCM3 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition3ToSCM3, &mutexCondition3ToSCM3);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition3ToSCM3);
			//set the next state (go to IDLE)
			currentCondition3State = CONDITION_IDLE;
			return STATUS_FAILURE;
			//break;

		case CONDITION_RETURN_IDLE:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentCondition3State = CONDITION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status AtDestination(btSignal visit)
{
	static skill_request _conditionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
		switch (currentCondition5State)
		{
		case CONDITION_IDLE:
			//wait for tick
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				currentCondition5State = CONDITION_START_SKILL;
			}
			else if (visit == HALT)
			{
				currentCondition5State = CONDITION_RETURN_IDLE;
			}
			break;

		case CONDITION_START_SKILL:
			//send start to skill and wait for started signal
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCondition5ToSCM5);
			// send  send_start command to skill protocol
			_conditionCmd = send_start;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition5ToSCM5, &monitorCondition5Cmd, &countCondition5ToSCM5);
#ifdef SIMULATE
			const char *sigToSkill = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition5ToSCM5);
			// wait until the command has been read
			while (countCondition5ToSCM5 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition5ToSCM5, &mutexCondition5ToSCM5);
			}
			//here the FSM knows that the start command has been read
			pthread_mutex_unlock(&mutexCondition5ToSCM5);
			//leaf node is sure only that the skill has read the signal
			//The leaf node checks the skill status in the CONDITION_WAIT_ANSWER state
			currentCondition5State = CONDITION_WAIT_ANSWER;
			break;

		case CONDITION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The condition skill status can only be SUCCESS or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexCondition5ToSCM5);
			// send request_ack command to SCM
			_conditionCmd = request_ack;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition5ToSCM5, &monitorCondition5Cmd, &countCondition5ToSCM5);
#ifdef SIMULATE
			const char *sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition5ToSCM5);
			while (countCondition5ToSCM5 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition5ToSCM5, &mutexCondition5ToSCM5);
			}
			//here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexCondition5ToSCM5);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM5ToCondition5);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM5ToCondition5 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM5ToCondition5, &mutexSCM5ToCondition5);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM5ToCondition5, &countSCM5ToCondition5, &condFullFromSCM5ToCondition5);
			pthread_mutex_unlock(&mutexSCM5ToCondition5);

#ifdef SIMULATE
			//const char* watchedValFromSkill = getSkillSignalName(_skillStatus);
			//printf("    %s Signal status from Skill is  :%s:  \n",funcName,  watchedValFromSkill );
			//fflush(stdout);
#endif
			//based on signal value, go to  CONDITION_RETURN_SUCCESS or CONDITION_RETURN_FAILURE
			//CONDITION_RETURN_SUCCESS and CONDITION_RETURN_FAILURE need to wakeup the skill
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentCondition5State = CONDITION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentCondition5State = CONDITION_RETURN_FAILURE;
				break;

			default:
				break;
			}
			break;

		case CONDITION_RETURN_SUCCESS:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition5ToSCM5);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition5ToSCM5, &monitorCondition5Cmd, &countCondition5ToSCM5);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition5ToSCM5);
			// wait until the command has been read
			while (countCondition5ToSCM5 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition5ToSCM5, &mutexCondition5ToSCM5);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition5ToSCM5);
			//set the next state (go to IDLE)
			currentCondition5State = CONDITION_IDLE;
			return STATUS_SUCCESS;
			//break;

		case CONDITION_RETURN_FAILURE:
			//signals success to BT and wakeup skill
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition5ToSCM5);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition5ToSCM5, &monitorCondition5Cmd, &countCondition5ToSCM5);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition5ToSCM5);
			// wait until the command has been read
			while (countCondition5ToSCM5 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition5ToSCM5, &mutexCondition5ToSCM5);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition5ToSCM5);
			//set the next state (go to IDLE)
			currentCondition5State = CONDITION_IDLE;
			return STATUS_FAILURE;
			//break;

		case CONDITION_RETURN_IDLE:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentCondition5State = CONDITION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status AtChargingStation(btSignal visit)
{
	static skill_request _conditionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
		switch (currentCondition6State)
		{
		case CONDITION_IDLE:
			//wait for tick
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				currentCondition6State = CONDITION_START_SKILL;
			}
			else if (visit == HALT)
			{
				currentCondition6State = CONDITION_RETURN_IDLE;
			}
			break;

		case CONDITION_START_SKILL:
			//send start to skill and wait for started signal
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCondition6ToSCM6);
			// send  send_start command to skill protocol
			_conditionCmd = send_start;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition6ToSCM6, &monitorCondition6Cmd, &countCondition6ToSCM6);
#ifdef SIMULATE
			const char *sigToSkill = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition6ToSCM6);
			// wait until the command has been read
			while (countCondition6ToSCM6 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition6ToSCM6, &mutexCondition6ToSCM6);
			}
			//here the FSM knows that the start command has been read
			pthread_mutex_unlock(&mutexCondition6ToSCM6);
			//leaf node is sure only that the skill has read the signal
			//The leaf node checks the skill status in the CONDITION_WAIT_ANSWER state
			currentCondition6State = CONDITION_WAIT_ANSWER;
			break;

		case CONDITION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The condition skill status can only be SUCCESS or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexCondition6ToSCM6);
			// send request_ack command to SCM
			_conditionCmd = request_ack;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition6ToSCM6, &monitorCondition6Cmd, &countCondition6ToSCM6);
#ifdef SIMULATE
			const char *sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition6ToSCM6);
			while (countCondition6ToSCM6 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition6ToSCM6, &mutexCondition6ToSCM6);
			}
			//here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexCondition6ToSCM6);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM6ToCondition6);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM6ToCondition6 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM6ToCondition6, &mutexSCM6ToCondition6);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM6ToCondition6, &countSCM6ToCondition6, &condFullFromSCM6ToCondition6);
			pthread_mutex_unlock(&mutexSCM6ToCondition6);
#ifdef SIMULATE
			//const char* watchedValFromSkill = getSkillSignalName(_skillStatus);
			//printf("    %s Signal status from Skill is  :%s:  \n",funcName,  watchedValFromSkill );
			//fflush(stdout);
#endif
			//based on signal value, go to  CONDITION_RETURN_SUCCESS or CONDITION_RETURN_FAILURE
			//CONDITION_RETURN_SUCCESS and CONDITION_RETURN_FAILURE need to wakeup the skill
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentCondition6State = CONDITION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentCondition6State = CONDITION_RETURN_FAILURE;
				break;

			default:
				break;
			}
			break;

		case CONDITION_RETURN_SUCCESS:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition6ToSCM6);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition6ToSCM6, &monitorCondition6Cmd, &countCondition6ToSCM6);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition6ToSCM6);
			// wait until the command has been read
			while (countCondition6ToSCM6 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition6ToSCM6, &mutexCondition6ToSCM6);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition6ToSCM6);
			//set the next state (go to IDLE)
			currentCondition6State = CONDITION_IDLE;
			return STATUS_SUCCESS;
			//break;

		case CONDITION_RETURN_FAILURE:
			//signals success to BT and wakeup skill
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to protocol skill in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexCondition6ToSCM6);
			// send send_ok command to skill protocol
			_conditionCmd = send_ok;
			setSkillRequestAndCounter(_conditionCmd, bufferCondition6ToSCM6, &monitorCondition6Cmd, &countCondition6ToSCM6);
#ifdef SIMULATE
			sigToSkillW = getSkillRequestName(_conditionCmd);
			printf("	%s Condition signal to skill is : %s\n", funcName, sigToSkillW);
			fflush(stdout);
#endif
			// signal the skill protocol in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromCondition6ToSCM6);
			// wait until the command has been read
			while (countCondition6ToSCM6 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromCondition6ToSCM6, &mutexCondition6ToSCM6);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexCondition6ToSCM6);
			//set the next state (go to IDLE)
			currentCondition6State = CONDITION_IDLE;
			return STATUS_FAILURE;
			//break;

		case CONDITION_RETURN_IDLE:
#ifdef SIMULATE
			printf("	%s Condition state = CONDITION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentCondition6State = CONDITION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status GoToChargingStation(btSignal visit)
{
	static skill_request _actionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
		switch (currentAction4State)
		{
		case ACTION_IDLE:
			//wait for visit
#ifdef SIMULATE
			printf("	%s Action state = ACTION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				//start the skill
				currentAction4State = ACTION_START_SKILL;
			}
			else if (visit == HALT)
			{
				//the halt command is managed directly from action node
				//stop command will be not forwarded to the skill
				currentAction4State = ACTION_RETURN_IDLE;
			}
			break;

		case ACTION_START_SKILL:
			//send start to skill and wait for started ack
			//The started ack is encoded by pthread signal
#ifdef SIMULATE
			printf("	%s Action state = ACTION_START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// send send_start command to SCM1
			_actionCmd = send_start;
			setSkillRequestAndCounter(_actionCmd, bufferAction4ToSCM4, &monitorAction4Cmd, &countAction4ToSCM4);
#ifdef SIMULATE
			const char *sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action cmd to SCM4 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM4 in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction4ToSCM4);
			// wait until the scm reads the command and sends start to the skill
			while (countAction4ToSCM4 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
			// release the lock because the command has been read from SCM
			pthread_mutex_unlock(&mutexAction4ToSCM4);
			//leaf node is sure only that the scm has read the signal
			//node checks the skill status in the WAIT_ANSWER state
			currentAction4State = ACTION_WAIT_ANSWER;
			break;

		case ACTION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Action state = ACTION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The action skill status can  be SUCCESS, RUNNING or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// send request_ack command to skill
			_actionCmd = request_ack;
			setSkillRequestAndCounter(_actionCmd, bufferAction4ToSCM4, &monitorAction4Cmd, &countAction4ToSCM4);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction4ToSCM4);
			//wait until the command has been read
			while (countAction4ToSCM4 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
			// here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexAction4ToSCM4);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM4ToAction4);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM4ToAction4 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM4ToAction4, &mutexSCM4ToAction4);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM4ToAction4, &countSCM4ToAction4, &condFullFromSCM4ToAction4);
			pthread_mutex_unlock(&mutexSCM4ToAction4);

#ifdef SIMULATE
			const char *sigSkill = getSkillSignalName(_skillStatus);
			printf("	%s Answer from SCM is : %s\n", funcName, sigSkill);
			fflush(stdout);
#endif
			//based on signal value, go to ACTION_RETURN_RUNNING, ACTION_RETURN_SUCCESS or ACTION_RETURN_FAILURE
			//ACTION_RETURN_SUCCESS and ACTION_RETURN_FAILURE need to wakeup the skill
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentAction4State = ACTION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentAction4State = ACTION_RETURN_FAILURE;
				break;

			case SIG_RUNNING:
				currentAction4State = ACTION_RETURN_RUNNING;
				break;

			default:
				break;
			}
			break;

		case ACTION_RETURN_RUNNING:
			//result is RUNNING
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_RUNNING\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to ACTION_WAIT_RUNNING)
			currentAction4State = ACTION_WAIT_RUNNING;
			//signals running to BT
			return STATUS_RUNNING;
			//break;

		case ACTION_WAIT_RUNNING:
			//wait for tick or stop signal
#ifdef SIMULATE
			printf("	%s Action state = ACTION_WAIT_RUNNING\n", funcName);
			fflush(stdout);
#endif
			//based on signal value, go to ACTION_WAIT_ANSWER or ACTION_STOP_SKILL
			if (visit == TICK)
			{
				currentAction4State = ACTION_WAIT_ANSWER;
			}
			else if (visit == HALT)
			{
				currentAction4State = ACTION_STOP_SKILL;
			}

			break;

		case ACTION_RETURN_SUCCESS:
			//result is SUCCESS
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to SCM4 in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// send send_ok command to SCM4
			_actionCmd = send_ok;
			setSkillRequestAndCounter(_actionCmd, bufferAction4ToSCM4, &monitorAction4Cmd, &countAction4ToSCM4);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM4 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM4 in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction4ToSCM4);
			// wait until the command has been read
			while (countAction4ToSCM4 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexAction4ToSCM4);
			//set the next state (go to IDLE)
			currentAction4State = ACTION_IDLE;
			//signals success to BT
			return STATUS_SUCCESS;
			//break;

		case ACTION_RETURN_FAILURE:
			//signals failure to BT and wakeup skill
			//result is FAILURE
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to SCM4 in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// send send_ok command to SCM4
			_actionCmd = send_ok;
			setSkillRequestAndCounter(_actionCmd, bufferAction4ToSCM4, &monitorAction4Cmd, &countAction4ToSCM4);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM4 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM4 in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction4ToSCM4);
			// wait until the command has been read
			while (countAction4ToSCM4 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexAction4ToSCM4);
			//set the next state (go to IDLE)
			currentAction4State = ACTION_IDLE;
			//signals failure to BT
			return STATUS_FAILURE;
			//break;

		case ACTION_STOP_SKILL:
#ifdef SIMULATE
			printf("	%s Action state = ACTION_STOP_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// send stop signal to skill
			_actionCmd = send_stop;
			setSkillRequestAndCounter(_actionCmd, bufferAction4ToSCM4, &monitorAction4Cmd, &countAction4ToSCM4);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM4 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the scm that in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction4ToSCM4);
			// wait until the scm reads the command and sends stop to the skill
			while (countAction4ToSCM4 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
			// release the lock because the command has been read from SCM
			pthread_mutex_unlock(&mutexAction4ToSCM4);
			//go to ACTION_RETURN_IDLE and return idle to BT
			currentAction4State = ACTION_RETURN_IDLE;

			break;

		case ACTION_RETURN_IDLE:
//result is IDLE
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentAction4State = ACTION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status GoToDestination(btSignal visit)
{
	static skill_request _actionCmd = absent_command;
	static skillSignal _skillStatus = SIG_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif

	while (1)
	{
		switch (currentAction1State)
		{
		case ACTION_IDLE:
//wait for visit
#ifdef SIMULATE
			printf("	%s Action state = ACTION_IDLE\n", funcName);
			fflush(stdout);
#endif

			if (visit == TICK)
			{
				//start the skill
				currentAction1State = ACTION_START_SKILL;
			}
			else if (visit == HALT)
			{
				//the halt command is managed directly from action node
				//stop command will be not forwarded to the skill
				currentAction1State = ACTION_RETURN_IDLE;
			}
			break;

		case ACTION_START_SKILL:
//send start to skill and wait for started ack
//The started ack is encoded by pthread signal
#ifdef SIMULATE
			printf("	%s Action state = ACTION_START_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);
			// send send_start command to SCM1
			_actionCmd = send_start;
			setSkillRequestAndCounter(_actionCmd, bufferAction1ToSCM1, &monitorAction1Cmd, &countAction1ToSCM1);
#ifdef SIMULATE
			const char *sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM1 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the scm that in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction1ToSCM1);
			// wait until the scm reads the command and sends start to the skill
			while (countAction1ToSCM1 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
			// release the lock because the command has been read from SCM
			pthread_mutex_unlock(&mutexAction1ToSCM1);
			//leaf node is sure only that the scm has read the signal
			//node checks the skill status in the WAIT_ANSWER state
			currentAction1State = ACTION_WAIT_ANSWER;
			break;

		case ACTION_WAIT_ANSWER:
#ifdef SIMULATE
			printf("	%s Action state = ACTION_WAIT_ANSWER\n", funcName);
			fflush(stdout);
#endif
			; //empty statement just for compiler
			//send request_ack command to SCM in order to know the skill status
			//The action skill status can  be SUCCESS, RUNNING or FAILURE
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);
			// send request_ack command to skill
			_actionCmd = request_ack;
			setSkillRequestAndCounter(_actionCmd, bufferAction1ToSCM1, &monitorAction1Cmd, &countAction1ToSCM1);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM1 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction1ToSCM1);
			// wait until the command has been read
			while (countAction1ToSCM1 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
			//here the FSM knows that the request_ack command has been read by SCM
			pthread_mutex_unlock(&mutexAction1ToSCM1);
			//now waiting for an answer from SCM
			pthread_mutex_lock(&mutexSCM1ToAction1);
			// wait until the answer is in the channel (SCM,LF)
			while (countSCM1ToAction1 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSCM1ToAction1, &mutexSCM1ToAction1);
			}
			//here the FSM knows that the answer is in the buffer
			//the SCM has sent the result of the request_ack command
			//read the skill status
			_skillStatus = getValueFromSkillAndWakeupNoMutex(bufferSCM1ToAction1, &countSCM1ToAction1, &condFullFromSCM1ToAction1);
			pthread_mutex_unlock(&mutexSCM1ToAction1);

#ifdef SIMULATE
			const char *sigSkill = getSkillSignalName(_skillStatus);
			printf("	%s Answer from SCM1 is : %s\n", funcName, sigSkill);
			fflush(stdout);
#endif
			//based on signal value, go to ACTION_RETURN_RUNNING, ACTION_RETURN_SUCCESS or ACTION_RETURN_FAILURE
			//ACTION_RETURN_SUCCESS and ACTION_RETURN_FAILURE need to wakeup the skill
			switch (_skillStatus)
			{
			case SIG_SUCCESS:
				currentAction1State = ACTION_RETURN_SUCCESS;
				break;

			case SIG_FAILED:
				currentAction1State = ACTION_RETURN_FAILURE;
				break;

			case SIG_RUNNING:
				currentAction1State = ACTION_RETURN_RUNNING;
				break;

			default:
				break;
			}
			break;

		case ACTION_RETURN_RUNNING:
//result is RUNNING
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_RUNNING\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to ACTION_WAIT_RUNNING)
			currentAction1State = ACTION_WAIT_RUNNING;
			//signals running to BT
			return STATUS_RUNNING;
			//break;

		case ACTION_WAIT_RUNNING:
//wait for tick or stop signal
#ifdef SIMULATE
			printf("	%s Action state = ACTION_WAIT_RUNNING\n", funcName);
			fflush(stdout);
#endif
			//based on signal value, go to ACTION_WAIT_ANSWER or ACTION_STOP_SKILL
			if (visit == TICK)
			{
				currentAction1State = ACTION_WAIT_ANSWER;
			}
			else if (visit == HALT)
			{
				currentAction1State = ACTION_STOP_SKILL;
			}

			break;

		case ACTION_RETURN_SUCCESS:
//result is SUCCESS
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to SCM1 in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);
			// send send_ok command to SCM1
			_actionCmd = send_ok;
			setSkillRequestAndCounter(_actionCmd, bufferAction1ToSCM1, &monitorAction1Cmd, &countAction1ToSCM1);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM1 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM1 in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction1ToSCM1);
			// wait until the command has been read
			while (countAction1ToSCM1 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexAction1ToSCM1);
			//set the next state (go to IDLE)
			currentAction1State = ACTION_IDLE;
			//signals success to BT
			return STATUS_SUCCESS;
			//break;

		case ACTION_RETURN_FAILURE:
//result is FAILURE
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//send send_ok command to SCM1 in order to reset the skill to idle state
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);
			// send send_ok command to SCM1
			_actionCmd = send_ok;
			setSkillRequestAndCounter(_actionCmd, bufferAction1ToSCM1, &monitorAction1Cmd, &countAction1ToSCM1);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM1 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the SCM1 in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction1ToSCM1);
			// wait until the command has been read
			while (countAction1ToSCM1 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
			//here the FSM knows that the request_ack command has been read
			pthread_mutex_unlock(&mutexAction1ToSCM1);
			//set the next state (go to IDLE)
			currentAction1State = ACTION_IDLE;
			//signals failure to BT
			return STATUS_FAILURE;
			//break;

		case ACTION_STOP_SKILL:
#ifdef SIMULATE
			printf("	%s Action state = ACTION_STOP_SKILL\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);
			// send stop signal to skill
			_actionCmd = send_stop;
			setSkillRequestAndCounter(_actionCmd, bufferAction1ToSCM1, &monitorAction1Cmd, &countAction1ToSCM1);
#ifdef SIMULATE
			sigToSkill = getSkillRequestName(_actionCmd);
			printf("	%s Action command to SCM1 is : %s\n", funcName, sigToSkill);
			fflush(stdout);
#endif
			// signal the scm that in order to wake up and read the command
			pthread_cond_signal(&condNotEmptyFromAction1ToSCM1);
			// wait until the scm reads the command and sends stop to the skill
			while (countAction1ToSCM1 == 1)
			{
				// wait on the CV
				pthread_cond_wait(&condFullFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
			// release the lock because the command has been read from SCM
			pthread_mutex_unlock(&mutexAction1ToSCM1);
			//go to ACTION_RETURN_IDLE and return idle to BT
			currentAction1State = ACTION_RETURN_IDLE;
			break;

		case ACTION_RETURN_IDLE:
//result is IDLE
#ifdef SIMULATE
			printf("	%s Action state = ACTION_RETURN_IDLE\n", funcName);
			fflush(stdout);
#endif
			//set the next state (go to IDLE)
			currentAction1State = ACTION_IDLE;
			//signals idle to BT
			return STATUS_IDLE;
			//break;

		default:
			return STATUS_ABSENT;
			//break;
		}
	}
}

status AlwaysRunning(btSignal visit)
{
	if (visit == TICK)
	{
		return STATUS_RUNNING;
	}
	else
	{
		return STATUS_IDLE;
	}
}

//visit function thread implementation (tick generator)
void *visitExecution(void *threadid)
{
	static btSignal _btVal = TICK;
	static status _btRes = STATUS_ABSENT;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif

	while (1)
	{

		// acquire the lock
		pthread_mutex_lock(&mutexVisitToBt);
		//we suppose that action node is fastest that tick generation
		// write TICK signal  to buffer
		_btVal = TICK;
		// if(countLoop%10 == 0){
		// 	// write HALT signal  to buffer
		// 	_btVal = HALT;
		// }
		monitorBtSignal = _btVal;
		setValueToBtFromVisit(_btVal, bufferFromVisitToBt, &countFromVisitToBt);
#if defined(MONITORTHREAD)
		fprintf(out_file, "%s", _tName);
		fflush(out_file);
#endif
#ifdef SIMULATE
		const char *btSig = getBtSignalName(_btVal);
		time_t my_time = time(NULL);
		printf("%s VISIT SIGNAL is : %s -- time = %ld\n", funcName, btSig, my_time);
		fflush(stdout);
#endif
		// signal the CV
		pthread_cond_signal(&condNotEmptyFromVisitToBt);
		while (countFromVisitToBt == 1)
		{
			pthread_cond_wait(&condFullFromVisitToBt, &mutexVisitToBt);
		}
		//increase the tick counter
		pthread_mutex_lock(&mutexMonitor);
		countLoop++;
		pthread_mutex_unlock(&mutexMonitor);
		//visit has been sent
		// release the lock
		pthread_mutex_unlock(&mutexVisitToBt);

		//wait for a BT response
		// acquire the lock
		pthread_mutex_lock(&mutexBtToVisit);

		while (countFromBtToVisit == 0)
		{
			pthread_cond_wait(&condNotEmptyFromBtToVisit, &mutexBtToVisit);
		}
		_btRes = getResultFromBt(bufferFromBtToVisit, &countFromBtToVisit);

		monitorVisitStatus = _btRes;
#if defined(MONITORTHREAD)
		fprintf(out_file, "%s", _tName);
		fflush(out_file);
#endif
#if defined(SIMULATE) || defined(SIMULATE_CHIARA) || defined(MONITOR) || defined(MONITOR2) || defined(MONITOR3)
		//const char* btResult = getStatusName(_btRes);
		//printf("RESULT OF THE VISIT IS  :%s:  \n",  btResult );
		//fflush(stdout);
		assert(!(_btRes == STATUS_SUCCESS));
#endif
		pthread_cond_signal(&condfullFromBtToVisit);
		// release the lock
		pthread_mutex_unlock(&mutexBtToVisit);
		pthread_yield();
		pthread_yield();
		//usleep(1);
		//sleep(1);
	}
}

//BT function thread implementation
//each node is a function and we visit the root node
void *btExecution(void *threadid)
{
	static btSignal _visitVal = ABSENT;
	static status _returnStatus = STATUS_ABSENT;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		// acquire the lock
		pthread_mutex_lock(&mutexVisitToBt);
		// check if the buffer visit is empty
		while (countFromVisitToBt == 0)
		{
			// wait on the CV
			pthread_cond_wait(&condNotEmptyFromVisitToBt, &mutexVisitToBt);
		}
#if defined(MONITORTHREAD)
		fprintf(out_file, "%s", _tName);
		fflush(out_file);
#endif
#if defined(MONITOR2)
		printf("btExecution\n");
		propertyTwo();
#endif
#if defined(MONITOR3)
		printf("btExecution\n");
		propertyThree();
#endif
		//read the value from Visit
		_visitVal = getValueFromVisit(bufferFromVisitToBt, &countFromVisitToBt);
#ifdef SIMULATE
		const char *sigFromBt = getBtSignalName(_visitVal);
		printf("  Signal status from Visit is  :%s:  \n", sigFromBt);
		fflush(stdout);
#endif
		pthread_cond_signal(&condFullFromVisitToBt);
		// release the lock
		pthread_mutex_unlock(&mutexVisitToBt);
		//send visit signal to BT//////
		_returnStatus = sequence1(_visitVal);
//////////////////////////////
#if defined(SIMULATE)
		printf("  %s BT Returns status %s\n", funcName, getStatusName(_returnStatus));
#endif
		//return status to visit
		// acquire the lock
		pthread_mutex_lock(&mutexBtToVisit);
		writeResultToVisit(_returnStatus, bufferFromBtToVisit, &countFromBtToVisit);
		// signal the CV
		pthread_cond_signal(&condNotEmptyFromBtToVisit);
#if defined(MONITORTHREAD)
		fprintf(out_file, "%s", _tName);
		fflush(out_file);
#endif
		while (countFromBtToVisit == 1)
		{
			pthread_cond_wait(&condfullFromBtToVisit, &mutexBtToVisit);
		}
		// release the lock
		pthread_mutex_unlock(&mutexBtToVisit);
		pthread_yield();
		pthread_yield();
	}
}

//skill2      BatteryLevel
void *skill2Fsm(void *threadid)
{
	static componentSignal _currentComp2Ack = COMP_ABSENT;
	static commandSignal _cmdFromScm = CMD_ABSENT;
	static int _currBatteryValue = 1000;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "BatteryReaderBatteryLevelClient";
	const char *componentName = "BatteryComponent";
#endif
	//initialize status to ABSENT
	pthread_mutex_lock(&mutexSkill2ReadWrite);
	sharedSkill2Status = SIG_ABSENT;
	pthread_mutex_unlock(&mutexSkill2ReadWrite);
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_BR[0] = BatteryReaderComponentFunction
	//_messageToCCM_BR[1] = thread_id
	static int _messageToCCM_BR[2] = {0, 0};
	while (1)
	{

		switch (currentSkill2State)
		{
		case SKILL_COND_IDLE:
			//wait for start signal from scm
			//reset status to ABSENT
			pthread_mutex_lock(&mutexSkill2ReadWrite);
			sharedSkill2Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill2ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_IDLE\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexSCM2ToSkill2);
			// check if the buffer  is empty
			while (countSCM2ToSkill2 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM2ToSkill2, &mutexSCM2ToSkill2);
			}
			//read the value from scm
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM2ToSkill2, &countSCM2ToSkill2, &condFullFromSCM2ToSkill2);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
#ifdef SIMULATE
			const char *sigFromCondition = getCommandSignalName(_cmdFromScm);
			printf("			%s Command from Condition node is  :%s:  \n", funcName, sigFromCondition);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM2ToSkill2);

			if (_cmdFromScm == CMD_START)
			{
				//go to GET state and wait for a new command
				currentSkill2State = SKILL_COND_GET;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_GET:
//In this state the skill ask for the battery level by sending
//the level command to the BatteryReader component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_GET\n", funcName);
			fflush(stdout);
#endif
			//send level command to CCM in order to get battery level value
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_BR);
			//  send level command to CCM
			_messageToCCM_BR[0] = level;
			_messageToCCM_BR[1] = _taskid;
			insertToTail(MAX_BUFF_BR_ROWS, MAX_BUFF_BR_COLS, bufferSkillsToCCM_BR, &frontIndex_CCM_BR, &rearIndex_CCM_BR, _messageToCCM_BR, &countSkillsToCCM_BR);
#ifdef SIMULATE
			const char *sigToComp = getBatteryReaderFuncName(_messageToCCM_BR[0]);
			printf("			%s Skill cmd  to CCM_BR is : %s\n", funcName, sigToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *sigToComp = getBatteryReaderFuncName(_messageToCCM_BR[0]);
			printf("From     : /%s\nTo       : /%s\nCommand  : %s\nArguments:\n", funcName, componentName, sigToComp);
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", sigToComp);
			// printf("Arguments:\n");
			fflush(stdout);
#endif
			// send command to CCM
			hasBeenReadSkill2 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_BR);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill2)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill2ToCCM_BR, &mutexSkillsToCCM_BR);
			}

			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_BR);
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s CCM BR read the command\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_BRToSkill2);
			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_BRToSkill2 == 0)
			{
				// wait for CCM response
				pthread_cond_wait(&condNotEmptyFromCCM_BRToSkill2, &mutexCCM_BRToSkill2);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
//here the skill has received something through the channel
#ifdef MONITOR
			propertyOne();
#endif
#if defined(MONITOR2)
			printf("Skill2FSM battery = %d\n", bufferCCM_BRToSkill2[1]);
			propertyTwo();
#endif
#if defined(MONITOR3)
			printf("Skill2FSM battery = %d\n", bufferCCM_BRToSkill2[1]);
			propertyThree();
#endif
			//get the value
			//read component ack (buffer[0]) and data  (buffer[1])
			_currentComp2Ack = getCompAck(bufferCCM_BRToSkill2);
			_currBatteryValue = getLevel(bufferCCM_BRToSkill2);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_BRToSkill2, &countCCM_BRToSkill2);
#ifdef SIMULATE
			printf("			%s Signal sent to CCM\n", funcName);
			fflush(stdout);
#endif
			//release the lock
			pthread_mutex_unlock(&mutexCCM_BRToSkill2);
			switch (_currentComp2Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s Waiting for Component answer...\n", funcName);
				fflush(stdout);
#endif
				currentSkill2State = SKILL_COND_GET;
				break;
			case COMP_OK:
#ifdef SIMULATE
				printf("			%s Component ack: COMP_OK\n", funcName);
				printf("			%s batteryLevel  = %d\n", funcName, _currBatteryValue);
				fflush(stdout);
#endif
				if (_currBatteryValue > 300)
				{
					//currentSkill2State = SKILL_COND_RECEIVING_SUCCESS;
					//set current status to Success
					pthread_mutex_lock(&mutexSkill2ReadWrite);
					sharedSkill2Status = SIG_SUCCESS;
					pthread_mutex_unlock(&mutexSkill2ReadWrite);
					currentSkill2State = SKILL_COND_SUCCESS;
				}
				else
				{
					//currentSkill2State = SKILL_COND_RECEIVING_FAILURE;
					//set current status to Failure
					pthread_mutex_lock(&mutexSkill2ReadWrite);
					sharedSkill2Status = SIG_FAILED;
					pthread_mutex_unlock(&mutexSkill2ReadWrite);
					currentSkill2State = SKILL_COND_FAILURE;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill2State = SKILL_COND_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//set current status to Success
			pthread_mutex_lock(&mutexSkill2ReadWrite);
			sharedSkill2Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill2ReadWrite);
			currentSkill2State = SKILL_COND_SUCCESS;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_SUCCESS:
//send success signal to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM2ToSkill2);
			// check if the buffer  is empty
			while (countSCM2ToSkill2 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM2ToSkill2, &mutexSCM2ToSkill2);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM2ToSkill2, &countSCM2ToSkill2, &condFullFromSCM2ToSkill2);
			// release the lock
			pthread_mutex_unlock(&mutexSCM2ToSkill2);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill2State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//set current status to Failure
			pthread_mutex_lock(&mutexSkill2ReadWrite);
			sharedSkill2Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill2ReadWrite);
			currentSkill2State = SKILL_COND_FAILURE;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_FAILURE:
//send failure signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM2ToSkill2);
			// check if the buffer  is empty
			while (countSCM2ToSkill2 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM2ToSkill2, &mutexSCM2ToSkill2);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM2ToSkill2, &countSCM2ToSkill2, &condFullFromSCM2ToSkill2);
			// release the lock
			pthread_mutex_unlock(&mutexSCM2ToSkill2);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill2State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//  skill3 BatteryNotCharging
void *skill3Fsm(void *threadid)
{
	static componentSignal _currentComp3Ack = COMP_ABSENT;
	static commandSignal _cmdFromScm = CMD_ABSENT;
	static ChargingStatus _currChargingStatus = BATTERY_NOT_CHARGING;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "BatteryReaderBatteryNotChargingClient";
	const char *componentName = "BatteryComponent";
#endif
	//initialize status to ABSENT
	pthread_mutex_lock(&mutexSkill3ReadWrite);
	sharedSkill3Status = SIG_ABSENT;
	pthread_mutex_unlock(&mutexSkill3ReadWrite);
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_BR[0] = BatteryReaderComponentFunction
	//_messageToCCM_BR[1] = thread_id
	static int _messageToCCM_BR[2] = {0, 0};
	while (1)
	{

		switch (currentSkill3State)
		{
		case SKILL_COND_IDLE:
			//wait for start signal from scm
			//reset status to ABSENT
			pthread_mutex_lock(&mutexSkill3ReadWrite);
			sharedSkill3Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill3ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_IDLE\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexSCM3ToSkill3);

			// check if the buffer  is empty
			while (countSCM3ToSkill3 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM3ToSkill3, &mutexSCM3ToSkill3);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read the value from scm
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM3ToSkill3, &countSCM3ToSkill3, &condFullFromSCM3ToSkill3);
#ifdef SIMULATE
			const char *sigFromCondition = getCommandSignalName(_cmdFromScm);
			printf("			%s Command from Condition node is  :%s:  \n", funcName, sigFromCondition);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM3ToSkill3);

			if (_cmdFromScm == CMD_START)
			{
				//go to GET state and wait for a new command
				currentSkill3State = SKILL_COND_GET;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_GET:
//wait for an answer from component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_GET\n", funcName);
			fflush(stdout);
#endif
			//send charging_status command to CCM in order to get charging status
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_BR);
			//  send charging_status command to CCM
			_messageToCCM_BR[0] = charging_status;
			_messageToCCM_BR[1] = _taskid;
			insertToTail(MAX_BUFF_BR_ROWS, MAX_BUFF_BR_COLS, bufferSkillsToCCM_BR, &frontIndex_CCM_BR, &rearIndex_CCM_BR, _messageToCCM_BR, &countSkillsToCCM_BR);
#ifdef SIMULATE
			const char *sigToComp = getBatteryReaderFuncName(_messageToCCM_BR[0]);
			printf("			%s Skill cmd  to CCM_BR is : %s\n", funcName, sigToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *sigToComp = getBatteryReaderFuncName(_messageToCCM_BR[0]);
			printf("From     : /%s\nTo       : /%s\nCommand  : %s\nArguments:\n", funcName, componentName, sigToComp);
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", sigToComp);
			// printf("Arguments:\n");
			fflush(stdout);
#endif
			// send command to CCM
			hasBeenReadSkill3 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_BR);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill3)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill3ToCCM_BR, &mutexSkillsToCCM_BR);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_BR);
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s CCM BR read the command\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_BRToSkill3);

			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_BRToSkill3 == 0)
			{
				// wait for CCM response
				pthread_cond_wait(&condNotEmptyFromCCM_BRToSkill3, &mutexCCM_BRToSkill3);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//here the skill has received something through the channel
			//get the value
			//read component ack (buffer[0]) and data  (buffer[1])
			_currentComp3Ack = getCompAck(bufferCCM_BRToSkill3);
			_currChargingStatus = getChargingStatus(bufferCCM_BRToSkill3);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_BRToSkill3, &countCCM_BRToSkill3);
#ifdef SIMULATE
			printf("			%s Signal sent to CCM\n", funcName);
			fflush(stdout);
#endif
			//release the lock
			pthread_mutex_unlock(&mutexCCM_BRToSkill3);
			switch (_currentComp3Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s Waiting for Component answer...\n", funcName);
				fflush(stdout);
#endif
				currentSkill3State = SKILL_COND_GET;
				break;
			case COMP_OK:
#ifdef SIMULATE
				printf("			%s Component ack: COMP_OK\n", funcName);
				printf("			%s Charging_Status  = %s\n", funcName, getBatteryNotChargingName(_currChargingStatus));
				fflush(stdout);
#endif
				if (_currChargingStatus == BATTERY_CHARGING)
				{
					currentSkill3State = SKILL_COND_RECEIVING_FAILURE;
				}
				else
				{
					currentSkill3State = SKILL_COND_RECEIVING_SUCCESS;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill3State = SKILL_COND_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//set current status to Success
			pthread_mutex_lock(&mutexSkill3ReadWrite);
			sharedSkill3Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill3ReadWrite);
			currentSkill3State = SKILL_COND_SUCCESS;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_SUCCESS:
//send success signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM3ToSkill3);

			// check if the buffer  is empty
			while (countSCM3ToSkill3 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM3ToSkill3, &mutexSCM3ToSkill3);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM3ToSkill3, &countSCM3ToSkill3, &condFullFromSCM3ToSkill3);
			// release the lock
			pthread_mutex_unlock(&mutexSCM3ToSkill3);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill3State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//set current status to Failure
			pthread_mutex_lock(&mutexSkill3ReadWrite);
			sharedSkill3Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill3ReadWrite);
			currentSkill3State = SKILL_COND_FAILURE;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_FAILURE:
//send failure signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM3ToSkill3);

			// check if the buffer  is empty
			while (countSCM3ToSkill3 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM3ToSkill3, &mutexSCM3ToSkill3);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM3ToSkill3, &countSCM3ToSkill3, &condFullFromSCM3ToSkill3);
			// release the lock
			pthread_mutex_unlock(&mutexSCM3ToSkill3);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill3State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//skill AtDestination
void *skill5Fsm(void *threadid)
{
	static componentSignal _currentComp5Ack = COMP_ABSENT;
	static commandSignal _cmdFromScm = CMD_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "GoToIsAtClient";
	const char *componentName = "GoToComponent";
#endif
	//initialize status to ABSENT
	pthread_mutex_lock(&mutexSkill5ReadWrite);
	sharedSkill5Status = SIG_ABSENT;
	pthread_mutex_unlock(&mutexSkill5ReadWrite);
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_GoTo[0] = GoToComponentFunction
	//_messageToCCM_GoTo[1] = thread_id
	//_messageToCCM_GoTo[2] = destination
	static int _messageToCCM_GoTo[3] = {0, 0, 0};
	while (1)
	{

		switch (currentSkill5State)
		{
		case SKILL_COND_IDLE:
			//wait for start signal from scm
			//reset status to ABSENT
			pthread_mutex_lock(&mutexSkill5ReadWrite);
			sharedSkill5Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill5ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_IDLE\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexSCM5ToSkill5);
			// check if the buffer  is empty
			while (countSCM5ToSkill5 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM5ToSkill5, &mutexSCM5ToSkill5);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read the value from scm
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM5ToSkill5, &countSCM5ToSkill5, &condFullFromSCM5ToSkill5);
#ifdef SIMULATE
			const char *sigFromCondition = getCommandSignalName(_cmdFromScm);
			printf("			%s Command from Condition node is  :%s:  \n", funcName, sigFromCondition);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM5ToSkill5);

			if (_cmdFromScm == CMD_START)
			{
				//go to GET state and wait for a new command
				currentSkill5State = SKILL_COND_GET;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_GET:
//wait for an answer from component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_GET\n", funcName);
			fflush(stdout);
#endif
			//send charging_status command to CCM in order to get charging status
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send isAtLocation command to CCM
			_messageToCCM_GoTo[0] = isAtLocation;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = KITCHEN;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			const char *sigToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill cmd  to CCM_GoTo is : %s\n", funcName, sigToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			const char *dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// send command to CCM
			hasBeenReadSkill5 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill5)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill5ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}

			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s CCM_GoTo read the command\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_GoToToSkill5);

			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_GoToToSkill5 == 0)
			{
				// wait for CCM response
				pthread_cond_wait(&condNotEmptyFromCCM_GoToToSkill5, &mutexCCM_GoToToSkill5);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
#if defined(MONITOR3)
			printf("IsAtDestination\n");
			propertyThree();
#endif
			//here the skill has received something through the channel
			//get the value
			//read component ack (buffer[0]) and data  (buffer[1])
			//getxxx function reset the buffer value to absent
			_currentComp5Ack = getCompAck(bufferCCM_GoToToSkill5);
			int currIsAtLocation = getIsAtLocation(bufferCCM_GoToToSkill5);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_GoToToSkill5, &countCCM_GoToToSkill5);
#ifdef SIMULATE
			printf("			%s Signal sent to CCM\n", funcName);
			fflush(stdout);
#endif
			//release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToSkill5);
			switch (_currentComp5Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s Waiting for Component answer...\n", funcName);
				fflush(stdout);
#endif
				currentSkill5State = SKILL_COND_GET;
				break;
			case COMP_OK:
#ifdef SIMULATE
				printf("			%s Component ack: COMP_OK\n", funcName);
				printf("			%s IsAtDestination  = %s\n", funcName, currIsAtLocation == 1 ? "true" : "false");
				fflush(stdout);
#endif
				if (currIsAtLocation == 0)
				{
					currentSkill5State = SKILL_COND_RECEIVING_FAILURE;
				}
				else
				{
					currentSkill5State = SKILL_COND_RECEIVING_SUCCESS;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill5State = SKILL_COND_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//set current status to Success
			pthread_mutex_lock(&mutexSkill5ReadWrite);
			sharedSkill5Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill5ReadWrite);
			currentSkill5State = SKILL_COND_SUCCESS;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_SUCCESS:
//send success signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM5ToSkill5);
			// check if the buffer  is empty
			while (countSCM5ToSkill5 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM5ToSkill5, &mutexSCM5ToSkill5);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM5ToSkill5, &countSCM5ToSkill5, &condFullFromSCM5ToSkill5);
			// release the lock
			pthread_mutex_unlock(&mutexSCM5ToSkill5);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill5State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//set current status to Failure
			pthread_mutex_lock(&mutexSkill5ReadWrite);
			sharedSkill5Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill5ReadWrite);
			currentSkill5State = SKILL_COND_FAILURE;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_FAILURE:
//send failure signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM5ToSkill5);
			// check if the buffer  is empty
			while (countSCM5ToSkill5 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM5ToSkill5, &mutexSCM5ToSkill5);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM5ToSkill5, &countSCM5ToSkill5, &condFullFromSCM5ToSkill5);
			// release the lock
			pthread_mutex_unlock(&mutexSCM5ToSkill5);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill5State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//Skill AtChargingStation
void *skill6Fsm(void *threadid)
{
	static componentSignal _currentComp6Ack = COMP_ABSENT;
	static commandSignal _cmdFromScm = CMD_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "GoToIsAtClient";
	const char *componentName = "GoToComponent";
#endif
	//initialize status to ABSENT
	pthread_mutex_lock(&mutexSkill6ReadWrite);
	sharedSkill6Status = SIG_ABSENT;
	pthread_mutex_unlock(&mutexSkill6ReadWrite);
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_GoTo[0] = GoToComponentFunction
	//_messageToCCM_GoTo[1] = thread_id
	//_messageToCCM_GoTo[2] = destination
	static int _messageToCCM_GoTo[3] = {0, 0, 0};
	while (1)
	{

		switch (currentSkill6State)
		{
		case SKILL_COND_IDLE:
			//wait for start signal from scm
			//reset status to ABSENT
			pthread_mutex_lock(&mutexSkill6ReadWrite);
			sharedSkill6Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill6ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_IDLE\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexSCM6ToSkill6);
			// check if the buffer  is empty
			while (countSCM6ToSkill6 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM6ToSkill6, &mutexSCM6ToSkill6);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read the value from scm
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM6ToSkill6, &countSCM6ToSkill6, &condFullFromSCM6ToSkill6);
#ifdef SIMULATE
			const char *sigFromCondition = getCommandSignalName(_cmdFromScm);
			printf("			%s Command from Condition node is  :%s:  \n", funcName, sigFromCondition);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM6ToSkill6);

			if (_cmdFromScm == CMD_START)
			{
				//go to GET state and wait for a new command
				currentSkill6State = SKILL_COND_GET;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_GET:
//wait for an answer from component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_GET\n", funcName);
			fflush(stdout);
#endif
			//send charging_status command to CCM in order to get charging status
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send isAtLocation command to CCM
			_messageToCCM_GoTo[0] = isAtLocation;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = CHARGING_STATION;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			const char *sigToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill cmd  to CCM_GoTo is : %s\n", funcName, sigToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			const char *dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// send command to CCM
			hasBeenReadSkill6 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill6)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill6ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}

			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s CCM_GoTo read the command\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_GoToToSkill6);
			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_GoToToSkill6 == 0)
			{
				// wait for CCM response
				pthread_cond_wait(&condNotEmptyFromCCM_GoToToSkill6, &mutexCCM_GoToToSkill6);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//here the skill has received something through the channel
			//get the value
			//read component ack (buffer[0]) and data  (buffer[1])
			_currentComp6Ack = getCompAck(bufferCCM_GoToToSkill6);
			int currIsAtLocation = getIsAtLocation(bufferCCM_GoToToSkill6);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_GoToToSkill6, &countCCM_GoToToSkill6);
#ifdef SIMULATE
			printf("			%s Signal sent to CCM\n", funcName);
			fflush(stdout);
#endif
			//release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToSkill6);
			switch (_currentComp6Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s Waiting for Component answer...\n", funcName);
				fflush(stdout);
#endif
				currentSkill6State = SKILL_COND_GET;
				break;
			case COMP_OK:
#ifdef SIMULATE
				printf("			%s Component ack: COMP_OK\n", funcName);
				printf("			%s IsAtDestination  = %s\n", funcName, currIsAtLocation == 1 ? "true" : "false");
				fflush(stdout);
#endif
				if (currIsAtLocation == 0)
				{
					currentSkill6State = SKILL_COND_RECEIVING_FAILURE;
				}
				else
				{
					currentSkill6State = SKILL_COND_RECEIVING_SUCCESS;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill6State = SKILL_COND_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//set current status to Success
			pthread_mutex_lock(&mutexSkill6ReadWrite);
			sharedSkill6Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill6ReadWrite);
			currentSkill6State = SKILL_COND_SUCCESS;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_SUCCESS:
//send success signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM6ToSkill6);
			// check if the buffer  is empty
			while (countSCM6ToSkill6 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM6ToSkill6, &mutexSCM6ToSkill6);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM6ToSkill6, &countSCM6ToSkill6, &condFullFromSCM6ToSkill6);
			// release the lock
			pthread_mutex_unlock(&mutexSCM6ToSkill6);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill6State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_COND_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//set current status to Failure
			pthread_mutex_lock(&mutexSkill6ReadWrite);
			sharedSkill6Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill6ReadWrite);
			currentSkill6State = SKILL_COND_FAILURE;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_COND_FAILURE:
//send failure signal   to condition node and wait for ack
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_COND_FAILURE\n", funcName);
			fflush(stdout);
#endif
			//  waiting for command ok
			// acquire the lock
			pthread_mutex_lock(&mutexSCM6ToSkill6);
			// check if the buffer  is empty
			while (countSCM6ToSkill6 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM6ToSkill6, &mutexSCM6ToSkill6);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromScm = getCommandValueAndWakeUpNoMutex(bufferSCM6ToSkill6, &countSCM6ToSkill6, &condFullFromSCM6ToSkill6);
			// release the lock
			pthread_mutex_unlock(&mutexSCM6ToSkill6);

			if (_cmdFromScm == CMD_OK)
			{
				//go to IDLE state and wait for a new command
				currentSkill6State = SKILL_COND_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//skill1 GoToDestination
void *skill1Fsm(void *threadid)
{
	static componentSignal _currentComp1Ack = COMP_ABSENT;
	static GoToStatus _currentComponentStatus = NOT_STARTED;
	static commandSignal _cmdFromSCM = CMD_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "GoToGoToClient";
	const char *componentName = "GoToComponent";
#endif
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_GoTo[0] = GoToComponentFunction
	//_messageToCCM_GoTo[1] = thread_id
	//_messageToCCM_GoTo[2] = destination
	static int _messageToCCM_GoTo[3] = {0, 0, 0};
	while (1)
	{

		switch (currentSkill1State)
		{
		case SKILL_ACT_IDLE:
			//set the current skill ack signal equal to Absent
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			sharedSkill1Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_IDLE\n", funcName);
			fflush(stdout);
#endif
			//wait for start command from SCM
			// acquire the lock
			pthread_mutex_lock(&mutexSCM1ToSkill1);
			// check if the buffer  is empty
			while (countSCM1ToSkill1 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM1ToSkill1, &mutexSCM1ToSkill1);
			}
#if defined(MONITORTHREAD)
// fprintf(out_file,"%s", _tName);
// fflush(out_file);
#endif
			//read the value from SCM
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromSCM = getCommandValueAndWakeUpNoMutex(bufferSCM1ToSkill1, &countSCM1ToSkill1, &condFullFromSCM1ToSkill1);
#ifdef SIMULATE
			const char *sigFromAction = getCommandSignalName(_cmdFromSCM);
			printf("			%s Command from SCM is  :%s:  \n", funcName, sigFromAction);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM1ToSkill1);

			if (_cmdFromSCM == CMD_START)
			{
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				//set running signal to action node
				pthread_mutex_lock(&mutexSkill1ReadWrite);
				sharedSkill1Status = SIG_RUNNING;
				pthread_mutex_unlock(&mutexSkill1ReadWrite);
				//go to send_request in order to send start signal to component
				currentSkill1State = SKILL_ACT_SEND_REQUEST;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_SEND_REQUEST:
//send goTo command in order to start the component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_SEND_REQUEST\n", funcName);
			fflush(stdout);
#endif
			//set value to action node equal to RUNNING
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			sharedSkill1Status = SIG_RUNNING;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
			//Start the component
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send goTo command to CCM in order to start navigation
			_messageToCCM_GoTo[0] = goTo;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = KITCHEN;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM_GoTo is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			const char *dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// signal the SCM in order to forward the start command
			hasBeenReadSkill1 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill1)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill1ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
#ifdef SIMULATE
			printf("			%s CCM GoTo read the command\n", funcName);
			fflush(stdout);
#endif
			//go to GET state if no stop command has been received
			currentSkill1State = SKILL_ACT_GET;
			//check if a STOP command arrives from CCM node
			_cmdFromSCM = watchCommandValue(bufferSCM1ToSkill1, &mutexSCM1ToSkill1);
			if (_cmdFromSCM == CMD_STOP)
			{
#ifdef SIMULATE
				printf("			%s Skill received a CMD_STOP from CCM_GoTo\n", funcName);
				fflush(stdout);
#endif
				//read buffer value and send signal in order wake up the SCM (empties the channel)
				getCommandValueAndWakeUp(bufferSCM1ToSkill1, &countSCM1ToSkill1, &mutexSCM1ToSkill1, &condFullFromSCM1ToSkill1);
				currentSkill1State = SKILL_ACT_HALT;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_ACT_GET:
//send getStatus to CCM and wait for an answer from component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_GET\n", funcName);
			fflush(stdout);
#endif
			//set value to action node equal to RUNNING
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			sharedSkill1Status = SIG_RUNNING;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
			//  send goTo command to CCM in order to start navigation
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			_messageToCCM_GoTo[0] = getStatus;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = KITCHEN;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM_GoTo is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// signal the SCM in order to forward the start command
			hasBeenReadSkill1 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill1)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill1ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
#ifdef SIMULATE
			printf("			%s CCM GoTo read the command\n", funcName);
			fflush(stdout);
#endif
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s Wait for an answer from CCM_GoTo\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_GoToToSkill1);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_GoToToSkill1 == 0)
			{
				// wait for component signal
				pthread_cond_wait(&condNotEmptyFromCCM_GoToToSkill1, &mutexCCM_GoToToSkill1);
			}
#if defined(MONITOR2)
			printf("Skill1FSM\n");
			propertyTwo();
#endif
#if defined(MONITOR3)
			printf("Skill1FSM\n");
			propertyThree();
#endif
			_currentComp1Ack = getCompAck(bufferCCM_GoToToSkill1);
			_currentComponentStatus = getGoToStatus(bufferCCM_GoToToSkill1);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_GoToToSkill1, &countCCM_GoToToSkill1);
			// release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToSkill1);
#ifdef SIMULATE
			printf("			%s CCM GoTo sent an answer to skill1\n", funcName);
			fflush(stdout);
#endif
			//check if a STOP command arrives from SCM node
			_cmdFromSCM = watchCommandValue(bufferSCM1ToSkill1, &mutexSCM1ToSkill1);
			if (_cmdFromSCM == CMD_STOP)
			{
#ifdef SIMULATE
				printf("			%s Skill received a CMD_STOP signal\n", funcName);
				fflush(stdout);
#endif
				//read buffer value and send signal in order wake up the SCM (empties the channel)
				getCommandValueAndWakeUp(bufferSCM1ToSkill1, &countSCM1ToSkill1, &mutexSCM1ToSkill1, &condFullFromSCM1ToSkill1);
				currentSkill1State = SKILL_ACT_HALT;
				pthread_yield();
				pthread_yield();
				break;
			}
			//if no stop signal has been received, based on CCM response choose next state
			switch (_currentComp1Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s COMP_ABSENT  Component is running...\n", funcName);
				fflush(stdout);
#endif
				currentSkill1State = SKILL_ACT_GET;
				break;
			case COMP_OK:
				if (_currentComponentStatus == SUCCESS)
				{
#ifdef SIMULATE
					printf("			%s COMP_OK  <SUCCESS> Component has finished successfully!!\n", funcName);
					fflush(stdout);
#endif
					currentSkill1State = SKILL_ACT_RECEIVING_SUCCESS;
				}
				else if (_currentComponentStatus == RUNNING)
				{
#ifdef SIMULATE
					printf("			%s COMP_OK <RUNNING> Component is running...\n", funcName);
					fflush(stdout);
#endif
					currentSkill1State = SKILL_ACT_GET;
				}
				else if (_currentComponentStatus == NOT_STARTED || _currentComponentStatus == ABORT)
				{
#ifdef SIMULATE
					printf("			%s Component is NOT_STARTED/ABORT \n", funcName);
					fflush(stdout);
#endif
					currentSkill1State = SKILL_ACT_RECEIVING_FAILURE;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill1State = SKILL_ACT_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			//sleep(1);
			break;

		case SKILL_ACT_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			sharedSkill1Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
			currentSkill1State = SKILL_ACT_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_WAIT_COMMAND:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//wait for a command from SCM (CMD_OK or CMD_STOP)
			// acquire the lock
			pthread_mutex_lock(&mutexSCM1ToSkill1);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// check if the buffer  is empty
			while (countSCM1ToSkill1 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM1ToSkill1, &mutexSCM1ToSkill1);
			}
			//read the value from SCM
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromSCM = getCommandValueAndWakeUpNoMutex(bufferSCM1ToSkill1, &countSCM1ToSkill1, &condFullFromSCM1ToSkill1);
#ifdef SIMULATE
			sigFromAction = getCommandSignalName(_cmdFromSCM);
			printf("			%s Command from CCM_GoTo is  :%s:  \n", funcName, sigFromAction);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM1ToSkill1);

			//if command equal to STOP go to SKILL_ACT_HALT
			if (_cmdFromSCM == CMD_STOP)
			{
				currentSkill1State = SKILL_ACT_HALT;
			}
			else if (_cmdFromSCM == CMD_OK)
			{
				// command has been read
				currentSkill1State = SKILL_ACT_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_ACT_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			sharedSkill1Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
			currentSkill1State = SKILL_ACT_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_HALT:
//stop the component and send stopped signal to action node
#ifdef SIMULATE
			printf("			%s Skill state = HALT\n", funcName);
			fflush(stdout);
#endif
			//Stop the component
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send goTo command to CCM in order to start navigation
			_messageToCCM_GoTo[0] = halt;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = KITCHEN;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			dest = "kitchen";
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, dest, componentName, cmdToComp, dest);
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", dest);
			fflush(stdout);
#endif
			// signal the SCM in order to forward the stop command
			hasBeenReadSkill1 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill1)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill1ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
			//command has been read
			currentSkill1State = SKILL_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//skill4 GoToChargingStation
void *skill4Fsm(void *threadid)
{
	static componentSignal _currentComp4Ack = COMP_ABSENT;
	static GoToStatus _currentComponentStatus = NOT_STARTED;
	static commandSignal _cmdFromSCM = CMD_ABSENT;
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "GoToGoToClient";
	const char *componentName = "GoToComponent";
#endif
	thread_id _taskid = (long)threadid;
#if defined(MONITORTHREAD)
	const char *_tName = getThreadIdName(_taskid);
#endif
	//_messageToCCM_GoTo[0] = GoToComponentFunction
	//_messageToCCM_GoTo[1] = thread_id
	//_messageToCCM_GoTo[2] = destination
	static int _messageToCCM_GoTo[3] = {0, 0, 0};
	while (1)
	{

		switch (currentSkill4State)
		{
		case SKILL_ACT_IDLE:
			//set the current skill ack signal equal to Absent
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			sharedSkill4Status = SIG_ABSENT;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_IDLE\n", funcName);
			fflush(stdout);
#endif
			//wait for start signal from action
			// acquire the lock
			pthread_mutex_lock(&mutexSCM4ToSkill4);

			// check if the buffer is empty
			while (countSCM4ToSkill4 == 0)
			{
				// wait until the start command arrives
				pthread_cond_wait(&condNotEmptyFromSCM4ToSkill4, &mutexSCM4ToSkill4);
			}
#if defined(MONITORTHREAD)
// fprintf(out_file,"%s", _tName);
// fflush(out_file);
#endif
			//read the value from SCM
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromSCM = getCommandValueAndWakeUpNoMutex(bufferSCM4ToSkill4, &countSCM4ToSkill4, &condFullFromSCM4ToSkill4);
#ifdef SIMULATE
			const char *sigFromAction = getCommandSignalName(_cmdFromSCM);
			printf("			%s Command from SCM is  :%s:  \n", funcName, sigFromAction);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM4ToSkill4);

			if (_cmdFromSCM == CMD_START)
			{
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				//set running signal to action node
				pthread_mutex_lock(&mutexSkill4ReadWrite);
				sharedSkill4Status = SIG_RUNNING;
				pthread_mutex_unlock(&mutexSkill4ReadWrite);
				//go to send_request in order to send start signal to component
				currentSkill4State = SKILL_ACT_SEND_REQUEST;
			}
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_SEND_REQUEST:
//send goTo command in order to start the component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_SEND_REQUEST\n", funcName);
			fflush(stdout);
#endif
			//set value to action node equal to RUNNING
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			sharedSkill4Status = SIG_RUNNING;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
			//Start the component
			//acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send goTo command to CCM in order to start navigation
			_messageToCCM_GoTo[0] = goTo;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = CHARGING_STATION;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM_GoTo is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			const char *cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			const char *dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// signal the SCM in order to forward the start command
			hasBeenReadSkill4 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill4)
			{
				// wait for component signal
				pthread_cond_wait(&condFullFromSkill4ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
#ifdef SIMULATE
			printf("			%s CCM GoTo read the command\n", funcName);
			fflush(stdout);
#endif
			//go to GET state if no stop command has been received
			currentSkill4State = SKILL_ACT_GET;
			//check if a STOP command arrives from SCM node
			_cmdFromSCM = watchCommandValue(bufferSCM4ToSkill4, &mutexSCM4ToSkill4);
			if (_cmdFromSCM == CMD_STOP)
			{
#ifdef SIMULATE
				printf("			%s Skill received a CMD_STOP from SCM\n", funcName);
				fflush(stdout);
#endif
				//read buffer value and send signal in order wake up the SCM (empties the channel)
				getCommandValueAndWakeUp(bufferSCM4ToSkill4, &countSCM4ToSkill4, &mutexSCM4ToSkill4, &condFullFromSCM4ToSkill4);
				currentSkill4State = SKILL_ACT_HALT;
			}

			pthread_yield();
			pthread_yield();
			break;

		case SKILL_ACT_GET:
//send getStatus to CCM and wait for an answer from component
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_GET\n", funcName);
			fflush(stdout);
#endif
			//set value to action node equal to RUNNING
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			sharedSkill4Status = SIG_RUNNING;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
			//  send goTo command to CCM in order to start navigation
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			_messageToCCM_GoTo[0] = getStatus;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = CHARGING_STATION;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM_GoTo is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			dest = getDestinationName(_messageToCCM_GoTo[2]);
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, toLower(dest, strlen(dest)), componentName, cmdToComp, toLower(dest, strlen(dest)));
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", toLower(dest, strlen(dest)));
			fflush(stdout);
#endif
			// signal the SCM in order to forward the start command
			hasBeenReadSkill4 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			//wait until the CCM read the command
			while (!hasBeenReadSkill4)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill4ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
#ifdef SIMULATE
			printf("			%s CCM GoTo read the command\n", funcName);
			fflush(stdout);
#endif
//now the skill wait for the answer from the CCM
#ifdef SIMULATE
			printf("			%s Wait for an answer from CCM_GoTo\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexCCM_GoToToSkill4);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// wait until the CCM send the answer (i.e. the channel is empty)
			while (countCCM_GoToToSkill4 == 0)
			{
				// wait for component signal
				pthread_cond_wait(&condNotEmptyFromCCM_GoToToSkill4, &mutexCCM_GoToToSkill4);
			}
			_currentComp4Ack = getCompAck(bufferCCM_GoToToSkill4);
			_currentComponentStatus = getGoToStatus(bufferCCM_GoToToSkill4);
			//signal the CCM
			sendSignalAndDecreaseCounter(&condFullFromCCM_GoToToSkill4, &countCCM_GoToToSkill4);
			// release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToSkill4);
#ifdef SIMULATE
			printf("			%s CCM_GoTo sent an answer to skill4\n", funcName);
			fflush(stdout);
#endif
			//check if a STOP command arrives from SCM node
			_cmdFromSCM = watchCommandValue(bufferSCM4ToSkill4, &mutexSCM4ToSkill4);
			if (_cmdFromSCM == CMD_STOP)
			{
#ifdef SIMULATE
				printf("			%s Skill received a CMD_STOP from SCM\n", funcName);
				fflush(stdout);
#endif
				//read buffer value and send signal in order wake up the SCM (empties the channel)
				getCommandValueAndWakeUp(bufferSCM4ToSkill4, &countSCM4ToSkill4, &mutexSCM4ToSkill4, &condFullFromSCM4ToSkill4);
				currentSkill4State = SKILL_ACT_HALT;
				pthread_yield();
				pthread_yield();
				break;
			}
			//if no stop signal has been received, based on CCM response choose next state
			switch (_currentComp4Ack)
			{
			case COMP_ABSENT:
#ifdef SIMULATE
				printf("			%s COMP_ABSENT  Component is running...\n", funcName);
				fflush(stdout);
#endif
				currentSkill4State = SKILL_ACT_GET;
				break;
			case COMP_OK:
				if (_currentComponentStatus == SUCCESS)
				{
#ifdef SIMULATE
					printf("			%s COMP_OK  <SUCCESS> Component has finished successfully!!\n", funcName);
					fflush(stdout);
#endif
					currentSkill4State = SKILL_ACT_RECEIVING_SUCCESS;
				}
				else if (_currentComponentStatus == RUNNING)
				{
#ifdef SIMULATE
					printf("			%s COMP_OK <RUNNING> Component is running...\n", funcName);
					fflush(stdout);
#endif
					currentSkill4State = SKILL_ACT_GET;
				}
				else if (_currentComponentStatus == NOT_STARTED || _currentComponentStatus == ABORT)
				{
#ifdef SIMULATE
					printf("			%s Component is NOT_STARTED/ABORT \n", funcName);
					fflush(stdout);
#endif
					currentSkill4State = SKILL_ACT_RECEIVING_FAILURE;
				}
				break;
			case COMP_FAIL:
#ifdef SIMULATE
				printf("			%s Component has finished failing (COMP_FAIL)\n", funcName);
				fflush(stdout);
#endif
				currentSkill4State = SKILL_ACT_RECEIVING_FAILURE;
				break;
			default:
				break;
			}

			pthread_yield();
			pthread_yield();
			//sleep(1);
			break;

		case SKILL_ACT_RECEIVING_SUCCESS:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_RECEIVING_SUCCESS\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			sharedSkill4Status = SIG_SUCCESS;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
			currentSkill4State = SKILL_ACT_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_WAIT_COMMAND:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//wait for a command from SCM (CMD_OK or CMD_STOP)
			// acquire the lock
			pthread_mutex_lock(&mutexSCM4ToSkill4);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
			// check if the buffer  is empty
			while (countSCM4ToSkill4 == 0)
			{
				// wait until the command arrive
				pthread_cond_wait(&condNotEmptyFromSCM4ToSkill4, &mutexSCM4ToSkill4);
			}
			//read the value from SCM
			//read buffer value and send signal in order wake up the SCM (empties the channel)
			_cmdFromSCM = getCommandValueAndWakeUpNoMutex(bufferSCM4ToSkill4, &countSCM4ToSkill4, &condFullFromSCM4ToSkill4);
#ifdef SIMULATE
			sigFromAction = getCommandSignalName(_cmdFromSCM);
			printf("			%s Command from CCM_GoTo is  :%s:  \n", funcName, sigFromAction);
			fflush(stdout);
#endif
			// release the lock
			pthread_mutex_unlock(&mutexSCM4ToSkill4);

			//if command equal to STOP go to SKILL_ACT_HALT
			if (_cmdFromSCM == CMD_STOP)
			{
				currentSkill4State = SKILL_ACT_HALT;
			}
			else if (_cmdFromSCM == CMD_OK)
			{
				// command has been read
				currentSkill4State = SKILL_ACT_IDLE;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SKILL_ACT_RECEIVING_FAILURE:
#ifdef SIMULATE
			printf("			%s Skill state = SKILL_ACT_RECEIVING_FAILURE\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			sharedSkill4Status = SIG_FAILED;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
			currentSkill4State = SKILL_ACT_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();

			break;

		case SKILL_ACT_HALT:
//stop the component and send stopped signal to action node
#ifdef SIMULATE
			printf("			%s Skill state = HALT\n", funcName);
			fflush(stdout);
#endif
			//Stop the component
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);
			//  send goTo command to CCM in order to start navigation
			_messageToCCM_GoTo[0] = halt;
			_messageToCCM_GoTo[1] = _taskid;
			_messageToCCM_GoTo[2] = CHARGING_STATION;
			insertToTail(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageToCCM_GoTo, &countSkillsToCCM_GoTo);
#ifdef SIMULATE
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			printf("			%s Skill command  to CCM is : %s\n", funcName, cmdToComp);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
			cmdToComp = getGoToFuncName(_messageToCCM_GoTo[0]);
			dest = "charging_station";
			printf("From     : /%s/%s\nTo       : /%s\nCommand  : %s\nArguments: %s\n", funcName, dest, componentName, cmdToComp, dest);
			// printf("To       : /%s\n", componentName);
			// printf("Command  : %s\n", cmdToComp);
			// printf("Arguments: %s\n", dest);
			fflush(stdout);
#endif
			// signal the SCM in order to forward the stop command
			hasBeenReadSkill4 = false;
			pthread_cond_signal(&condNotEmptyFromSkillsToCCM_GoTo);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the CCM read the command
			while (!hasBeenReadSkill4)
			{
				// wait for CCM signal
				pthread_cond_wait(&condFullFromSkill4ToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}
			//here the CCM has received and read the command
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
			// command has been read
			currentSkill4State = SKILL_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//protocol between action node and skill can receive send_start, send_ok, send_stop and request_ack command
void *scmAction1Skill1Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _actionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM1Skill1State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM1Skill1State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT
			_skillAck = SIG_ABSENT;
			//waiting for a command from action node
			// acquire the lock
			pthread_mutex_lock(&mutexAction1ToSCM1);

			// wait until a command arrives
			while (countAction1ToSCM1 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromAction1ToSCM1, &mutexAction1ToSCM1);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start,send_stop, send_ok or request_ack)
			//read the value from Action node
			//remove the command from the buffer (empties the channel)
			_actionCmd = getSkillRequestAndWakeUpNoMutex(bufferAction1ToSCM1, &countAction1ToSCM1, &condFullFromAction1ToSCM1);
			// release the lock
			pthread_mutex_unlock(&mutexAction1ToSCM1);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_actionCmd);
			printf("		%s Command from action1 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif

			//based on command value choose the next state
			if (_actionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM1Skill1State = SCM_FWD_START;
			}
			else if (_actionCmd == send_stop)
			{
				_skillCmd = CMD_STOP;
				currentSCM1Skill1State = SCM_FWD_STOP;
			}
			else if (_actionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM1Skill1State = SCM_FWD_OK;
			}
			else if (_actionCmd == request_ack)
			{
				currentSCM1Skill1State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM1ToSkill1);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM1ToSkill1, &monitorSkill1InternalCmd, &countSCM1ToSkill1);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM1ToSkill1);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM1ToSkill1 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM1ToSkill1, &mutexSCM1ToSkill1);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM1ToSkill1);
			//skill has been started  (status RUNNING)
			currentSCM1Skill1State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_STOP:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_STOP\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM1ToSkill1);
			//send CMD_STOP  to skill (_skillCmd is equal to CMD_STOP)
			setCommandValueAndCounter(_skillCmd, bufferSCM1ToSkill1, &monitorSkill1InternalCmd, &countSCM1ToSkill1);
			//send signal   in order to wake up the skill
			pthread_cond_signal(&condNotEmptyFromSCM1ToSkill1);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM1ToSkill1 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM1ToSkill1, &mutexSCM1ToSkill1);
			}
			//unlock
			pthread_mutex_unlock(&mutexSCM1ToSkill1);
			//return to WAIT_COMMAND state
			currentSCM1Skill1State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//action node asks for skill status (possible values are SUCCESS, RUNNING or FAILURE)
			//the SCM reads the skill shared buffer and return to action node
			pthread_mutex_lock(&mutexSkill1ReadWrite);
			_skillAck = sharedSkill1Status;
			pthread_mutex_unlock(&mutexSkill1ReadWrite);
			//action node is still waiting for a response
			//Send the response to Action node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM1ToAction1);
			//set the current skill status to action node buffer
			//set the response to action node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM1ToAction1, &countSCM1ToAction1);
			//signal the action node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM1ToAction1);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the action node reads and notify
			while (countSCM1ToAction1 == 1)
			{
				// wait for action node signal
				pthread_cond_wait(&condFullFromSCM1ToAction1, &mutexSCM1ToAction1);
			}
			//action node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM1ToAction1);
			//return to WAIT_COMMAND state
			currentSCM1Skill1State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM1ToSkill1);
			//send CMD_OK  to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM1ToSkill1, &monitorSkill1InternalCmd, &countSCM1ToSkill1);
			//send signal wake up the skill in order to wake up the skill
			pthread_cond_signal(&condNotEmptyFromSCM1ToSkill1);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM1ToSkill1 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM1ToSkill1, &mutexSCM1ToSkill1);
			}
			//unlock
			pthread_mutex_unlock(&mutexSCM1ToSkill1);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM1Skill1State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//protocol between condition node and skill can only receive send_start, send_ok and request_ack command
void *scmCondition2Skill2Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _conditionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM2Skill2State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM2Skill2State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
//condition command
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT
			_skillAck = SIG_ABSENT;
			//waiting for a command from condition node
			// acquire the lock
			pthread_mutex_lock(&mutexCondition2ToSCM2);
			// wait until a command arrives
			while (countCondition2ToSCM2 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromCondition2ToSCM2, &mutexCondition2ToSCM2);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start, send_ok or request_ack)
			//read the command from Condition node
			//remove the command from the buffer (empties the channel)
			_conditionCmd = getSkillRequestAndWakeUpNoMutex(bufferCondition2ToSCM2, &countCondition2ToSCM2, &condFullFromCondition2ToSCM2);
			// release the lock
			pthread_mutex_unlock(&mutexCondition2ToSCM2);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_conditionCmd);
			printf("		%s Command from condition2 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif
			//based on command value, choose the next state
			if (_conditionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM2Skill2State = SCM_FWD_START;
			}
			else if (_conditionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM2Skill2State = SCM_FWD_OK;
			}
			else if (_conditionCmd == request_ack)
			{
				currentSCM2Skill2State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM2ToSkill2);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM2ToSkill2, &monitorSkill2InternalCmd, &countSCM2ToSkill2);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM2ToSkill2);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM2ToSkill2 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM2ToSkill2, &mutexSCM2ToSkill2);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM2ToSkill2);
			//skill has been started
			currentSCM2Skill2State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//condition node asks for skill status (possible values are SUCCESS or FAILURE)
			//the protocol reads the skill shared buffer and return to condition node
			//wait until sharedSkill2Status is not equal to absent
			while (_skillAck == SIG_ABSENT)
			{
				pthread_mutex_lock(&mutexSkill2ReadWrite);
				_skillAck = sharedSkill2Status;
				pthread_mutex_unlock(&mutexSkill2ReadWrite);
			}
			//condition node is still waiting for a response
			//Send the response to Condition node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM2ToCondition2);
			//set the current skill status to condition node buffer
			//set the response to condition node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM2ToCondition2, &countSCM2ToCondition2);
			//signal the condition node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM2ToCondition2);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the condition reads and notify
			while (countSCM2ToCondition2 == 1)
			{
				// wait for condition node signal
				pthread_cond_wait(&condFullFromSCM2ToCondition2, &mutexSCM2ToCondition2);
			}
			//condition node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM2ToCondition2);
			//return to WAIT_COMMAND state
			currentSCM2Skill2State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM2ToSkill2);
			//send CMD_OK to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM2ToSkill2, &monitorSkill2InternalCmd, &countSCM2ToSkill2);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM2ToSkill2);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM2ToSkill2 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM2ToSkill2, &mutexSCM2ToSkill2);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM2ToSkill2);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM2Skill2State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//protocol between condition node and skill can only receive send_start, send_ok and request_ack command
void *scmCondition3Skill3Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _conditionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM3Skill3State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM3Skill3State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
//condition command
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT to action node
			_skillAck = SIG_ABSENT;
			//waiting for a command from condition node
			// acquire the lock
			pthread_mutex_lock(&mutexCondition3ToSCM3);
			// wait until a command arrives
			while (countCondition3ToSCM3 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromCondition3ToSCM3, &mutexCondition3ToSCM3);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start, send_ok or request_ack)
			//read the command from Condition node
			//remove the command from the buffer (empties the channel)
			_conditionCmd = getSkillRequestAndWakeUpNoMutex(bufferCondition3ToSCM3, &countCondition3ToSCM3, &condFullFromCondition3ToSCM3);
			// release the lock
			pthread_mutex_unlock(&mutexCondition3ToSCM3);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_conditionCmd);
			printf("		%s Command from condition3 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif

			//based on command value, choose the next state
			if (_conditionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM3Skill3State = SCM_FWD_START;
			}
			else if (_conditionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM3Skill3State = SCM_FWD_OK;
			}
			else if (_conditionCmd == request_ack)
			{
				currentSCM3Skill3State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM3ToSkill3);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM3ToSkill3, &monitorSkill3InternalCmd, &countSCM3ToSkill3);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM3ToSkill3);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM3ToSkill3 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM3ToSkill3, &mutexSCM3ToSkill3);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM3ToSkill3);
			//skill has been started
			currentSCM3Skill3State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//condition node asks for skill status (possible values are SUCCESS or FAILURE)
			//the protocol reads the skill shared buffer and return to condition node
			//wait until skillAck is not equal to absent
			while (_skillAck == SIG_ABSENT)
			{
				pthread_mutex_lock(&mutexSkill3ReadWrite);
				_skillAck = sharedSkill3Status;
				pthread_mutex_unlock(&mutexSkill3ReadWrite);
			}

			//condition node is still waiting for a response
			//Send the response to Condition node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM3ToCondition3);
			//set the current skill status to condition node buffer
			//set the response to condition node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM3ToCondition3, &countSCM3ToCondition3);
			//signal the condition node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM3ToCondition3);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the condition reads and notify
			while (countSCM3ToCondition3 == 1)
			{
				// wait for condition node signal
				pthread_cond_wait(&condFullFromSCM3ToCondition3, &mutexSCM3ToCondition3);
			}
			//condition node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM3ToCondition3);
			//return to WAIT_COMMAND state
			currentSCM3Skill3State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM3ToSkill3);
			//send CMD_OK to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM3ToSkill3, &monitorSkill3InternalCmd, &countSCM3ToSkill3);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM3ToSkill3);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM3ToSkill3 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM3ToSkill3, &mutexSCM3ToSkill3);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM3ToSkill3);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM3Skill3State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

//protocol between action node and skill can receive send_start, send_ok, send_stop and request_ack command
void *scmAction4Skill4Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _actionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM4Skill4State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM4Skill4State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT
			_skillAck = SIG_ABSENT;
			//waiting for a command from action node
			// acquire the lock
			pthread_mutex_lock(&mutexAction4ToSCM4);
			// wait until a command arrives
			while (countAction4ToSCM4 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromAction4ToSCM4, &mutexAction4ToSCM4);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start,send_stop, send_ok or request_ack)
			//read the value from Action node
			//remove the command from the buffer (empties the channel)
			_actionCmd = getSkillRequestAndWakeUpNoMutex(bufferAction4ToSCM4, &countAction4ToSCM4, &condFullFromAction4ToSCM4);
			// release the lock
			pthread_mutex_unlock(&mutexAction4ToSCM4);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_actionCmd);
			printf("		%s Command from action4 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif

			//based on command value choose the next state
			if (_actionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM4Skill4State = SCM_FWD_START;
			}
			else if (_actionCmd == send_stop)
			{
				_skillCmd = CMD_STOP;
				currentSCM4Skill4State = SCM_FWD_STOP;
			}
			else if (_actionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM4Skill4State = SCM_FWD_OK;
			}
			else if (_actionCmd == request_ack)
			{
				currentSCM4Skill4State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM4ToSkill4);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM4ToSkill4, &monitorSkill4InternalCmd, &countSCM4ToSkill4);
			//  signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM4ToSkill4);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM4ToSkill4 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM4ToSkill4, &mutexSCM4ToSkill4);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM4ToSkill4);
			//skill has been started (status RUNNING))
			currentSCM4Skill4State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_STOP:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_STOP\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM4ToSkill4);
			//send CMD_STOP  to skill (_skillCmd is equal to CMD_STOP)
			setCommandValueAndCounter(_skillCmd, bufferSCM4ToSkill4, &monitorSkill4InternalCmd, &countSCM4ToSkill4);
			//send signal wake up the skill in order to wake up the skill
			pthread_cond_signal(&condNotEmptyFromSCM4ToSkill4);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM4ToSkill4 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM4ToSkill4, &mutexSCM4ToSkill4);
			}
			// unlock
			pthread_mutex_unlock(&mutexSCM4ToSkill4);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM4Skill4State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//action node asks for skill status (possible values are SUCCESS, RUNNING or FAILURE)
			//the SCM reads the skill shared buffer and return to action node
			pthread_mutex_lock(&mutexSkill4ReadWrite);
			_skillAck = sharedSkill4Status;
			pthread_mutex_unlock(&mutexSkill4ReadWrite);
			//action node is still waiting for a response
			//Send the response to Action node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM4ToAction4);
			//set the current skill status to action buffer
			//set the response to action node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM4ToAction4, &countSCM4ToAction4);
			//signal the action node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM4ToAction4);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until action node reads and notify
			while (countSCM4ToAction4 == 1)
			{
				//  wait for action node signal
				pthread_cond_wait(&condFullFromSCM4ToAction4, &mutexSCM4ToAction4);
			}
			//action node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM4ToAction4);
			//return to WAIT_COMMAND state
			currentSCM4Skill4State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM4ToSkill4);
			//send CMD_OK   to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM4ToSkill4, &monitorSkill4InternalCmd, &countSCM4ToSkill4);
			//send signal to internal skill in order to wake up the skill
			pthread_cond_signal(&condNotEmptyFromSCM4ToSkill4);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM4ToSkill4 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM4ToSkill4, &mutexSCM4ToSkill4);
			}
			//unlock
			pthread_mutex_unlock(&mutexSCM4ToSkill4);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM4Skill4State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

void *scmCondition5Skill5Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _conditionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM5Skill5State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM5Skill5State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
//condition command
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT
			_skillAck = SIG_ABSENT;
			//waiting for a command from condition node
			// acquire the lock
			pthread_mutex_lock(&mutexCondition5ToSCM5);
			// wait until a command arrives
			while (countCondition5ToSCM5 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromCondition5ToSCM5, &mutexCondition5ToSCM5);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start, send_ok or request_ack)
			//read the command from Condition node
			//remove the command from the buffer (empties the channel)
			_conditionCmd = getSkillRequestAndWakeUpNoMutex(bufferCondition5ToSCM5, &countCondition5ToSCM5, &condFullFromCondition5ToSCM5);
			// release the lock
			pthread_mutex_unlock(&mutexCondition5ToSCM5);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_conditionCmd);
			printf("		%s Command from condition5 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif

			//based on command value, choose the next state
			if (_conditionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM5Skill5State = SCM_FWD_START;
			}
			else if (_conditionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM5Skill5State = SCM_FWD_OK;
			}
			else if (_conditionCmd == request_ack)
			{
				currentSCM5Skill5State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM5ToSkill5);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM5ToSkill5, &monitorSkill5InternalCmd, &countSCM5ToSkill5);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM5ToSkill5);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM5ToSkill5 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM5ToSkill5, &mutexSCM5ToSkill5);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM5ToSkill5);
			//skill has been started
			currentSCM5Skill5State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//condition node asks for skill status (possible values are SUCCESS or FAILURE)
			//the protocol reads the skill shared buffer and return to condition node
			//wait until skillAck is not equal to absent
			while (_skillAck == SIG_ABSENT)
			{
				pthread_mutex_lock(&mutexSkill5ReadWrite);
				_skillAck = sharedSkill5Status;
				pthread_mutex_unlock(&mutexSkill5ReadWrite);
			}

			//condition node is still waiting for a response
			//Send the response to Condition node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM5ToCondition5);
			//set the current skill status to condition node buffer
			//set the response to condition node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM5ToCondition5, &countSCM5ToCondition5);
			//signal the condition node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM5ToCondition5);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the condition reads and notify
			while (countSCM5ToCondition5 == 1)
			{
				// wait for condition node signal
				pthread_cond_wait(&condFullFromSCM5ToCondition5, &mutexSCM5ToCondition5);
			}
			//condition node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM5ToCondition5);
			//return to WAIT_COMMAND state
			currentSCM5Skill5State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM5ToSkill5);
			//send CMD_OK to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM5ToSkill5, &monitorSkill5InternalCmd, &countSCM5ToSkill5);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM5ToSkill5);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM5ToSkill5 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM5ToSkill5, &mutexSCM5ToSkill5);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM5ToSkill5);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM5Skill5State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

void *scmCondition6Skill6Execution(void *threadid)
{
	static skillSignal _skillAck = SIG_ABSENT;
	static commandSignal _skillCmd = CMD_ABSENT;
	static skill_request _conditionCmd = absent_command;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{

		switch (currentSCM6Skill6State)
		{
		case SCM_INIT:
#ifdef SIMULATE
			printf("		%s  state = SCM_INIT\n", funcName);
			fflush(stdout);
#endif
			currentSCM6Skill6State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_WAIT_COMMAND:
//condition command
#ifdef SIMULATE
			printf("		%s  state = SCM_WAIT_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//set ack_ABSENT
			_skillAck = SIG_ABSENT;
			//waiting for a command from condition node
			// acquire the lock
			pthread_mutex_lock(&mutexCondition6ToSCM6);
			// wait until a command arrives
			while (countCondition6ToSCM6 == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromCondition6ToSCM6, &mutexCondition6ToSCM6);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//at this point a command has arrived (possible values are send_start, send_ok or request_ack)
			//read the command from Condition node
			//remove the command from the buffer (empties the channel)
			_conditionCmd = getSkillRequestAndWakeUpNoMutex(bufferCondition6ToSCM6, &countCondition6ToSCM6, &condFullFromCondition6ToSCM6);
			// release the lock
			pthread_mutex_unlock(&mutexCondition6ToSCM6);
#ifdef SIMULATE
			const char *sigSkillCmdS = getSkillRequestName(_conditionCmd);
			printf("		%s Command from condition6 node is  :%s:  \n", funcName, sigSkillCmdS);
			fflush(stdout);
#endif

			//based on command value, choose the next state
			if (_conditionCmd == send_start)
			{
				_skillCmd = CMD_START;
				currentSCM6Skill6State = SCM_FWD_START;
			}
			else if (_conditionCmd == send_ok)
			{
				_skillCmd = CMD_OK;
				currentSCM6Skill6State = SCM_FWD_OK;
			}
			else if (_conditionCmd == request_ack)
			{
				currentSCM6Skill6State = SCM_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_START:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_START\n", funcName);
			fflush(stdout);
#endif
			//starts the skill
			pthread_mutex_lock(&mutexSCM6ToSkill6);
			//send start   to skill (_skillCmd is equal to CMD_START)
			setCommandValueAndCounter(_skillCmd, bufferSCM6ToSkill6, &monitorSkill6InternalCmd, &countSCM6ToSkill6);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM6ToSkill6);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM6ToSkill6 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM6ToSkill6, &mutexSCM6ToSkill6);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM6ToSkill6);
			//skill has been started
			currentSCM6Skill6State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_RETURN_DATA:
#ifdef SIMULATE
			printf("		%s  state = SCM_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//condition node asks for skill status (possible values are SUCCESS or FAILURE)
			//the protocol reads the skill shared buffer and return to condition node
			//wait until skillAck is not equal to absent
			while (_skillAck == SIG_ABSENT)
			{
				pthread_mutex_lock(&mutexSkill6ReadWrite);
				_skillAck = sharedSkill6Status;
				pthread_mutex_unlock(&mutexSkill6ReadWrite);
			}

			//condition node is still waiting for a response
			//Send the response to Condition node
			// acquire the lock
			pthread_mutex_lock(&mutexSCM6ToCondition6);
			//set the current skill status to condition node buffer
			//set the response to condition node through the shared variable
			setSkillAckAndCounter(&_skillAck, bufferSCM6ToCondition6, &countSCM6ToCondition6);
			//signal the condition node in order to read the answer
			pthread_cond_signal(&condNotEmptyFromSCM6ToCondition6);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the condition reads and notify
			while (countSCM6ToCondition6 == 1)
			{
				// wait for condition node signal
				pthread_cond_wait(&condFullFromSCM6ToCondition6, &mutexSCM6ToCondition6);
			}
			//condition node has read the answer
			// lock release
			pthread_mutex_unlock(&mutexSCM6ToCondition6);
			//return to WAIT_COMMAND state
			currentSCM6Skill6State = SCM_WAIT_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case SCM_FWD_OK:
#ifdef SIMULATE
			printf("		%s  state = SCM_FWD_OK\n", funcName);
			fflush(stdout);
#endif
			pthread_mutex_lock(&mutexSCM6ToSkill6);
			//send CMD_OK to skill (_skillCmd is equal to CMD_OK)
			setCommandValueAndCounter(_skillCmd, bufferSCM6ToSkill6, &monitorSkill6InternalCmd, &countSCM6ToSkill6);
			// signal and wakeup the skill in order to read the command
			pthread_cond_signal(&condNotEmptyFromSCM6ToSkill6);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			// wait until the skill reads and notify
			while (countSCM6ToSkill6 == 1)
			{
				// wait for skill signal
				pthread_cond_wait(&condFullFromSCM6ToSkill6, &mutexSCM6ToSkill6);
			}
			//unlock the mutex
			pthread_mutex_unlock(&mutexSCM6ToSkill6);
			//command has been read
			//return to WAIT_COMMAND state
			currentSCM6Skill6State = SCM_WAIT_COMMAND;

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

void *ccmGoToExecution(void *threadid)
{
	//variables for managing skill response
	static componentSignal _simulatorAck = COMP_ABSENT;
	static GoToStatus _simulatorStatus = NOT_STARTED;
	static int _simulatorIsAtLocation = 0;
	//local variable to store command from skill
	static GoToComponentFunction _cmdFromSkill = goTo_absentFunction;
	//local variable to store skill threadId
	static thread_id _skillThreadid = absentID;
	//local variable to store skill destination
	static destination _skillDestination = ABSENT_LOCATION;
	static destination _actualDestination = ABSENT_LOCATION;
	//variable representing command to navigator
	static NavigationCommand _simulatorCmd = navigation_absentCmd;
	//variable used to store skill message locally
	//_messageFromSkill[0] = GoToComponentFunction
	//_messageFromSkill[1] = thread_id
	//_messageFromSkill[2] = destination
	static int _messageFromSkill[3];
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif

#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "GoToComponent";
	const char *skillName1 = "GoToGoToClient";
	const char *skillName2 = "GoToIsAtClient";
#endif
	while (1)
	{

		switch (currentCCM_GoToComponentState)
		{
		case CCM_ACT_IDLE:
#ifdef SIMULATE
			printf("				%s  state = CCM_ACT_IDLE\n", funcName);
			fflush(stdout);
#endif
			//reset buffer to navigation
			_simulatorCmd = navigation_absentCmd;
			_skillDestination = ABSENT_LOCATION;
			//wait for command from skill
			//acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_GoTo);

			// wait until a command arrives
			while (countSkillsToCCM_GoTo == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSkillsToCCM_GoTo, &mutexSkillsToCCM_GoTo);
			}

//read message from skill
#if defined(MONITOR2)
			printf("ccmGoToExecution\n");
			propertyTwo();
#endif
#if defined(MONITOR3)
			printf("ccmGoToExecution\n");
			propertyThree();
#endif
			//extract message from front
			deleteFromFront(MAX_BUFF_GOTO_ROWS, MAX_BUFF_GOTO_COLS, bufferSkillsToCCM_GoTo, &frontIndex_CCM_GoTo, &rearIndex_CCM_GoTo, _messageFromSkill, &countSkillsToCCM_GoTo);
			_cmdFromSkill = _messageFromSkill[0];
			_skillThreadid = _messageFromSkill[1];
			_skillDestination = _messageFromSkill[2];
			//send signal to skill
			if (_skillThreadid == skill1_id)
			{
				pthread_cond_signal(&condFullFromSkill1ToCCM_GoTo);
				hasBeenReadSkill1 = true;
			}
			else if (_skillThreadid == skill4_id)
			{
				pthread_cond_signal(&condFullFromSkill4ToCCM_GoTo);
				hasBeenReadSkill4 = true;
			}
			else if (_skillThreadid == skill5_id)
			{
				pthread_cond_signal(&condFullFromSkill5ToCCM_GoTo);
				hasBeenReadSkill5 = true;
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
			}
			else if (_skillThreadid == skill6_id)
			{
				pthread_cond_signal(&condFullFromSkill6ToCCM_GoTo);
				hasBeenReadSkill6 = true;
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
			}
			// release the lock
			pthread_mutex_unlock(&mutexSkillsToCCM_GoTo);
#ifdef SIMULATE
			const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
			const char *threadIdName = getThreadIdName(_skillThreadid);
			const char *destinationName = getDestinationName(_skillDestination);
			printf("				%s Command from skill %s is  :%s:  -- destination = %s\n", funcName, threadIdName, sigSkillCmdS, destinationName);
			fflush(stdout);
#endif
			//based on command value choose the next state
			if (_cmdFromSkill == goTo)
			{
				_simulatorCmd = start_navigation;
				currentCCM_GoToComponentState = CCM_ACT_STARTING;
			}
			else if (_cmdFromSkill == halt)
			{
#ifdef SIMULATE
			printf("				%s  AN HALT ARRIVED FROM A SKILL\n", funcName);
			fflush(stdout);
#endif				
				_simulatorCmd = stop_navigation;
				currentCCM_GoToComponentState = CCM_ACT_STOPPING;
			}
			else if (_cmdFromSkill == isAtLocation)
			{
				currentCCM_GoToComponentState = CCM_ACT_RETURN_IS_AT_LOCATION;
			}
			else if (_cmdFromSkill == getStatus)
			{
				currentCCM_GoToComponentState = CCM_ACT_RETURN_DATA;
			}
			pthread_yield();
			pthread_yield();
			break;

		case CCM_ACT_STARTING:
		;
#ifdef SIMULATE
			printf("				%s  state = CCM_ACT_STARTING\n", funcName);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				const char *skillDest1 = getDestinationName(_skillDestination);
				const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s \n", funcName, skillName1, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)));
				fflush(stdout);
#endif
			//send start_navigation to the component
			pthread_mutex_lock(&mutexCCM_GoToToNavigation);
			//send start   to simulator (_simulatorCmd is equal to start_navigation)
			setNavigationCmd(_simulatorCmd, _skillDestination, bufferCCM_GoToToNavigation, &monitorCCM_GoToToNavigationCmd);
			//reset buffer to navigation
			_simulatorCmd = navigation_absentCmd;
			_skillDestination = ABSENT_LOCATION;
			// release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToNavigation);

#ifdef SIMULATE
			printf("				%s  simulator has been received start_navigation command\n", funcName);
			fflush(stdout);
#endif

			//simulator has been received start_navigation command
			currentCCM_GoToComponentState = CCM_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		case CCM_ACT_STOPPING:
#ifdef SIMULATE
			printf("				%s  state = CCM_ACT_STOPPING\n", funcName);
			fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				skillDest1 = getDestinationName(_skillDestination);
				sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s \n", funcName, skillName1, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)));
				fflush(stdout);
#endif
			//send stop_navigation to the component
			pthread_mutex_lock(&mutexCCM_GoToToNavigation);
			//send stop  to simulator (_simulatorCmd is equal to stop_navigation)
			setNavigationCmd(_simulatorCmd, _skillDestination, bufferCCM_GoToToNavigation, &monitorCCM_GoToToNavigationCmd);
			//reset buffer to navigation
			_simulatorCmd = navigation_absentCmd;
			_skillDestination = ABSENT_LOCATION;
			// release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToNavigation);
			//simulator has been received stop_navigation command

			currentCCM_GoToComponentState = CCM_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		case CCM_ACT_RETURN_IS_AT_LOCATION:
#ifdef SIMULATE
			printf("				%s  state = CCM_ACT_RETURN_IS_AT_LOCATION\n", funcName);
			fflush(stdout);
#endif
			//check if the destination is equal to the current destination (shared variable with navigator)
			pthread_mutex_lock(&mutexComponentReadWrite);
			_actualDestination = currentSimLocation;
			pthread_mutex_unlock(&mutexComponentReadWrite);
#ifdef SIMULATE
			const char *skillDest = getDestinationName(_skillDestination);
			const char *actDest = getDestinationName(_actualDestination);
#endif
			if (_skillDestination == _actualDestination)
			{
#ifdef SIMULATE
				printf("				%s  _skillDestination = %s :==: currentSimLocation = %s\n", funcName, skillDest, actDest);
				fflush(stdout);
#endif
				_simulatorAck = COMP_OK;
				pthread_mutex_lock(&mutexComponentReadWrite);
				_simulatorIsAtLocation = navigationCompSharedBuffer[2];
				pthread_mutex_unlock(&mutexComponentReadWrite);
			}
			else
			{
#ifdef SIMULATE
				printf("				%s  _skillDestination = %s :!=: currentSimLocation = %s\n", funcName, skillDest, actDest);
				fflush(stdout);
#endif
				_simulatorAck = COMP_OK;
				_simulatorIsAtLocation = 0;
			}
			//set the response to skills
			if (_skillThreadid == skill5_id)
			{
#ifdef SIMULATE
				printf("				%s  set response to skill 5 : IsAtLocation = %d\n", funcName, _simulatorIsAtLocation);
				fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				const char *skillDest1 = getDestinationName(_skillDestination);
				const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s %d\n", funcName, skillName2, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)), _simulatorIsAtLocation);
				// printf("Reply    : %s\n", sigSkillCmdS);
				// printf("Arguments: %s %d\n", toLower(skillDest1, strlen(skillDest1)), _simulatorIsAtLocation);
				fflush(stdout);
#endif
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_GoToToSkill5);
				//  send simulator ack response  to skill
				//set the current isAtLocation status to skill buffer
				setComponentAckIsAtLocationAndCounter(_simulatorAck, _simulatorIsAtLocation, bufferCCM_GoToToSkill5, &monitorComp5Ack, &countCCM_GoToToSkill5);
				// signal the skill 5
				pthread_cond_signal(&condNotEmptyFromCCM_GoToToSkill5);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				// wait until the skill reads the data
				while (countCCM_GoToToSkill5 == 1)
				{
					// wait for skill signal
					pthread_cond_wait(&condFullFromCCM_GoToToSkill5, &mutexCCM_GoToToSkill5);
				}
				// release the lock
				pthread_mutex_unlock(&mutexCCM_GoToToSkill5);
			}
			else if (_skillThreadid == skill6_id)
			{
#ifdef SIMULATE
				printf("				%s  set response to skill 6 : IsAtLocation = %d\n", funcName, _simulatorIsAtLocation);
				fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				const char *skillDest1 = getDestinationName(_skillDestination);
				const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s %d\n", funcName, skillName2, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)), _simulatorIsAtLocation);
				// printf("Reply    : %s\n", sigSkillCmdS);
				// printf("Arguments: %s %d\n", toLower(skillDest1, strlen(skillDest1)), _simulatorIsAtLocation);
				fflush(stdout);
#endif
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_GoToToSkill6);
				//  send simulator ack response  to skill
				//set the current simulator status to skill buffer
				setComponentAckIsAtLocationAndCounter(_simulatorAck, _simulatorIsAtLocation, bufferCCM_GoToToSkill6, &monitorComp6Ack, &countCCM_GoToToSkill6);
				// signal the skill 6
				pthread_cond_signal(&condNotEmptyFromCCM_GoToToSkill6);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				// wait until the skill reads the data
				while (countCCM_GoToToSkill6 == 1)
				{
					// wait for skill signal
					pthread_cond_wait(&condFullFromCCM_GoToToSkill6, &mutexCCM_GoToToSkill6);
				}
				// release the lock
				pthread_mutex_unlock(&mutexCCM_GoToToSkill6);
			}
			currentCCM_GoToComponentState = CCM_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		case CCM_ACT_RETURN_DATA:
#ifdef SIMULATE
			printf("				%s  state = CCM_ACT_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//check if the destination is equal to the current destination (shared variable with navigator)
			pthread_mutex_lock(&mutexComponentReadWrite);
			_actualDestination = currentSimLocation;
			pthread_mutex_unlock(&mutexComponentReadWrite);
#ifdef SIMULATE
			const char *skillDest1 = getDestinationName(_skillDestination);
			const char *actDest1 = getDestinationName(_actualDestination);
#endif
			if (_skillDestination == _actualDestination)
			{
#ifdef SIMULATE
				printf("				%s  _skillDestination = %s :==: currentSimLocation = %s\n", funcName, skillDest1, actDest1);
				fflush(stdout);
#endif
				//wait until simulator_ack is not equal to absent
				while (_simulatorAck == COMP_ABSENT)
				{
					pthread_mutex_lock(&mutexComponentReadWrite);
					_simulatorAck = navigationCompSharedBuffer[0];
					pthread_mutex_unlock(&mutexComponentReadWrite);
				}
				pthread_mutex_lock(&mutexComponentReadWrite);
				_simulatorStatus = navigationCompSharedBuffer[1];
				pthread_mutex_unlock(&mutexComponentReadWrite);
			}
			else
			{
#ifdef SIMULATE
				printf("				%s  _skillDestination = %s :!=: currentSimLocation = %s\n", funcName, skillDest1, actDest1);
				fflush(stdout);
#endif
				_simulatorAck = COMP_OK;
				_simulatorStatus = ABORT;
			}
			//set the response to skills
			if (_skillThreadid == skill1_id)
			{
#ifdef SIMULATE
				printf("				%s  set response to skill 1\n", funcName);
				fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				const char *skillDest1 = getDestinationName(_skillDestination);
				const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s %d\n", funcName, skillName1, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)), _simulatorStatus);
				// printf("To       : /%s/%s\n", skillName1, toLower(skillDest1, strlen(skillDest1)));
				// printf("Reply    : %s\n", sigSkillCmdS);
				// printf("Arguments: %s %d\n", toLower(skillDest1, strlen(skillDest1)), _simulatorStatus);
				fflush(stdout);
#endif
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_GoToToSkill1);
				//  send simulator ack response  to skill
				//set the current simulator status to skill buffer
				setComponentAckGoToStatusAndCounter(_simulatorAck, _simulatorStatus, bufferCCM_GoToToSkill1, &monitorComp1Ack, &countCCM_GoToToSkill1);
				// signal the skill 1
				pthread_cond_signal(&condNotEmptyFromCCM_GoToToSkill1);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
				// wait until the skill reads the data
				while (countCCM_GoToToSkill1 == 1)
				{
					// wait for skill signal
					pthread_cond_wait(&condFullFromCCM_GoToToSkill1, &mutexCCM_GoToToSkill1);
				}
				// release the lock
				pthread_mutex_unlock(&mutexCCM_GoToToSkill1);
			}
			else if (_skillThreadid == skill4_id)
			{
#ifdef SIMULATE
				printf("				%s  set response to skill 4\n", funcName);
				fflush(stdout);
#endif
#ifdef SIMULATE_CHIARA
				const char *skillDest1 = getDestinationName(_skillDestination);
				const char *sigSkillCmdS = getGoToFuncName(_cmdFromSkill);
				printf("From     : /%s\nTo       : /%s/%s\nReply    : %s\nArguments: %s %d\n", funcName, skillName1, toLower(skillDest1, strlen(skillDest1)), sigSkillCmdS, toLower(skillDest1, strlen(skillDest1)), _simulatorStatus);
				// printf("To       : /%s/%s\n", skillName1, toLower(skillDest1, strlen(skillDest1)));
				// printf("Reply    : %s\n", sigSkillCmdS);
				// printf("Arguments: %s %d\n", toLower(skillDest1, strlen(skillDest1)), _simulatorStatus);
				fflush(stdout);
#endif
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_GoToToSkill4);
				//  send simulator ack response  to skill
				//set the current simulator status to skill buffer
				setComponentAckGoToStatusAndCounter(_simulatorAck, _simulatorStatus, bufferCCM_GoToToSkill4, &monitorComp4Ack, &countCCM_GoToToSkill4);
				// signal the skill 4
				pthread_cond_signal(&condNotEmptyFromCCM_GoToToSkill4);
#if defined(MONITORTHREAD)
//fprintf(out_file,"%s", _tName);
//fflush(out_file);
#endif
				// wait until the skill reads the data
				while (countCCM_GoToToSkill4 == 1)
				{
					// wait for skill signal
					pthread_cond_wait(&condFullFromCCM_GoToToSkill4, &mutexCCM_GoToToSkill4);
				}
				// release the lock
				pthread_mutex_unlock(&mutexCCM_GoToToSkill4);
			}
			currentCCM_GoToComponentState = CCM_ACT_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

void *ccmBrExecution(void *threadid)
{
	//local variable to store skill function
	static BatteryReaderComponentFunction _skillFunction = batteryReader_absentFunction;
	//local variable to store skill thread id
	static thread_id _skillThreadid = absentID;
	//local variable used to return data to skill
	static componentSignal _comp2Ack = COMP_ABSENT; //BatteryLevel
	static int comp2Data = 0;						//BatteryLevel
	//variable used to store message from skill
	//_commandFromSkill[0] = BatteryReaderComponentFunction
	//_commandFromSkill[1] = thread_id
	static int _commandFromSkill[2];
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
#ifdef SIMULATE_CHIARA
	const char *funcName = "BatteryComponent";
	const char *skillName2 = "BatteryReaderBatteryLevelClient";
	const char *skillName3 = "BatteryReaderBatteryNotChargingClient";

#endif
	while (1)
	{

		switch (currentCCM_BrComponentState)
		{
		case CCM_COND_IDLE:
#ifdef SIMULATE
			printf("				%s  state = CCM_COND_IDLE\n", funcName);
			fflush(stdout);
#endif
			// acquire the lock
			pthread_mutex_lock(&mutexSkillsToCCM_BR);
			// check if the buffer command skill is empty
			while (countSkillsToCCM_BR == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromSkillsToCCM_BR, &mutexSkillsToCCM_BR);
			}
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s", _tName);
			fflush(out_file);
#endif
			//read message from skill
			deleteFromFront(MAX_BUFF_BR_ROWS, MAX_BUFF_BR_COLS, bufferSkillsToCCM_BR, &frontIndex_CCM_BR, &rearIndex_CCM_BR, _commandFromSkill, &countSkillsToCCM_BR);
			_skillFunction = _commandFromSkill[0];
			_skillThreadid = _commandFromSkill[1];
			//send signal to skill
			if (_skillThreadid == skill2_id)
			{
				pthread_cond_signal(&condFullFromSkill2ToCCM_BR);
				hasBeenReadSkill2 = true;
			}
			else if (_skillThreadid == skill3_id)
			{
				pthread_cond_signal(&condFullFromSkill3ToCCM_BR);
				hasBeenReadSkill3 = true;
			}
			// release the lock
			pthread_mutex_unlock(&mutexSkillsToCCM_BR);
#ifdef SIMULATE
			const char *sigFromSkills = getBatteryReaderFuncName(_skillFunction);
			const char *threadIdName = getThreadIdName(_skillThreadid);
			printf("				%s Command from %s is  :%s:  \n", funcName, threadIdName, sigFromSkills);
			fflush(stdout);
#endif
			if (_skillFunction == level)
			{
				//go to return data
				currentCCM_BrComponentState = CCM_COND_RETURN_DATA;
			}
			else if (_skillFunction == charging_status)
			{
				//do something and go to return data
				currentCCM_BrComponentState = CCM_COND_RETURN_STATUS;
			}
			pthread_yield();
			pthread_yield();
			break;

		case CCM_COND_RETURN_DATA:
#ifdef SIMULATE
			printf("				%s  state = CCM_COND_RETURN_DATA\n", funcName);
			fflush(stdout);
#endif
			//send ack and data to skill2
			//set local variable with current battery value
			pthread_mutex_lock(&mutexComponentReadWrite);
			_comp2Ack = batteryCompSharedBufferLevel[0];
			comp2Data = batteryCompSharedBufferLevel[1];
			pthread_mutex_unlock(&mutexComponentReadWrite);
			if (_skillThreadid == skill2_id)
			{
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_BRToSkill2);
				//put ack_OK and battery level value to buffer channel (CCM,SK2)
				setComponentAckDataIntAndCounter(_comp2Ack, comp2Data, bufferCCM_BRToSkill2, &monitorComp2Ack, &countCCM_BRToSkill2);
				//signal the skill in order to read the buffer
				pthread_cond_signal(&condNotEmptyFromCCM_BRToSkill2);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
#ifdef SIMULATE_CHIARA
			const char *sigFromSkills = getBatteryReaderFuncName(_skillFunction);
			printf("From     : /%s\nTo       : /%s\nReply    : %s\nArguments: %d\n", funcName, skillName2, sigFromSkills, comp2Data);
			// printf("To       : /%s\n", skillName2);
			// printf("Reply    : %s\n", sigFromSkills);
			// printf("Arguments: %d\n", comp2Data);
			fflush(stdout);
#endif
				// wait until the skill reads the values
				while (countCCM_BRToSkill2 == 1)
				{
					// wait for component signal
					pthread_cond_wait(&condFullFromCCM_BRToSkill2, &mutexCCM_BRToSkill2);
				}
				//skill has read the buffer
				// release the lock
				pthread_mutex_unlock(&mutexCCM_BRToSkill2);
			}
			else if (_skillThreadid == skill3_id)
			{ 
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_BRToSkill3);
				//put ack_OK and battery level value to buffer channel (CCM,SK2)
				setComponentAckDataIntAndCounter(_comp2Ack, comp2Data, bufferCCM_BRToSkill3, &monitorComp3Ack, &countCCM_BRToSkill3);
				//signal the skill in order to read the buffer
				pthread_cond_signal(&condNotEmptyFromCCM_BRToSkill3);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				// wait until the skill reads the values
				while (countCCM_BRToSkill3 == 1)
				{
					// wait for component signal
					pthread_cond_wait(&condFullFromCCM_BRToSkill3, &mutexCCM_BRToSkill3);
				}
				//skill has read the buffer
				// release the lock
				pthread_mutex_unlock(&mutexCCM_BRToSkill3);
			}
			currentCCM_BrComponentState = CCM_COND_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		case CCM_COND_RETURN_STATUS:
#ifdef SIMULATE
			printf("				%s  state = CCM_COND_RETURN_STATUS\n", funcName);
			fflush(stdout);
#endif
			//send ack and data to skill3
			//set local variable with current battery status
			pthread_mutex_lock(&mutexComponentReadWrite);
			_comp2Ack = batteryCompSharedBufferStatus[0];
			comp2Data = batteryCompSharedBufferStatus[1];
			pthread_mutex_unlock(&mutexComponentReadWrite);
			if (_skillThreadid == skill2_id)
			{
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_BRToSkill2);
				//put ack_OK and battery level value to buffer channel (CCM,SK2)
				setComponentAckDataIntAndCounter(_comp2Ack, comp2Data, bufferCCM_BRToSkill2, &monitorComp2Ack, &countCCM_BRToSkill2);
				//signal the skill in order to read the buffer
				pthread_cond_signal(&condNotEmptyFromCCM_BRToSkill2);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
				// wait until the skill reads the values
				while (countCCM_BRToSkill2 == 1)
				{
					// wait for component signal
					pthread_cond_wait(&condFullFromCCM_BRToSkill2, &mutexCCM_BRToSkill2);
				}
				//skill has read the buffer
				// release the lock
				pthread_mutex_unlock(&mutexCCM_BRToSkill2);
			}
			else if (_skillThreadid == skill3_id)
			{
				// acquire the lock
				pthread_mutex_lock(&mutexCCM_BRToSkill3);
				//put ack_OK and battery level value to buffer channel (CCM,SK2)
				setComponentAckDataIntAndCounter(_comp2Ack, comp2Data, bufferCCM_BRToSkill3, &monitorComp3Ack, &countCCM_BRToSkill3);
				//signal the skill in order to read the buffer
				pthread_cond_signal(&condNotEmptyFromCCM_BRToSkill3);
#if defined(MONITORTHREAD)
				fprintf(out_file, "%s", _tName);
				fflush(out_file);
#endif
#ifdef SIMULATE_CHIARA
			const char *sigFromSkills = getBatteryReaderFuncName(_skillFunction);
			printf("From     : /%s\nTo       : /%s\nReply    : %s\nArguments: %d\n", funcName, skillName3, sigFromSkills, comp2Data);
			//printf("To       : /%s\n", skillName3);
			// printf("Reply    : %s\n", sigFromSkills);
			// printf("Arguments: %d\n", comp2Data);
			fflush(stdout);
#endif
				// wait until the skill reads the values
				while (countCCM_BRToSkill3 == 1)
				{
					// wait for component signal
					pthread_cond_wait(&condFullFromCCM_BRToSkill3, &mutexCCM_BRToSkill3);
				}
				//skill has read the buffer
				// release the lock
				pthread_mutex_unlock(&mutexCCM_BRToSkill3);
			}
			currentCCM_BrComponentState = CCM_COND_IDLE;
			pthread_yield();
			pthread_yield();
			break;

		case CCM_COND_RETURN_FAIL:
#ifdef SIMULATE
			printf("				%s  state = CCM_COND_RETURN_FAIL\n", funcName);
			fflush(stdout);
#endif

			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
	}
}

void *refreshExecution(void *threadid)
{
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	static NavigationCommand _simulatorCmd = navigation_absentCmd;
	//static destination _destinationWatched = ABSENT_LOCATION;
	static destination _actualDestination = ABSENT_LOCATION;
	// init of simulator shared  data
	pthread_mutex_lock(&mutexComponentReadWrite);
	// shared buffer for navigation
	navigationCompSharedBuffer[0] = COMP_ABSENT;
	navigationCompSharedBuffer[1] = NOT_STARTED;
	navigationCompSharedBuffer[2] = 0;
	// shared buffer for battery level
	batteryCompSharedBufferLevel[0] = COMP_OK;
	batteryCompSharedBufferLevel[1] = simInfo.batteryLevel;
	// shared buffer for charging status
	batteryCompSharedBufferStatus[0] = COMP_OK;
	batteryCompSharedBufferStatus[1] = BATTERY_NOT_CHARGING;
	pthread_mutex_unlock(&mutexComponentReadWrite);
	while (1)
	{

#ifdef SIMULATE
		printf("						%s Refresh Data Called\n", funcName);
		fflush(stdout);
#endif
		//lock the mutex
		pthread_mutex_lock(&mutexComponentReadWrite);
#if defined(MONITORTHREAD)
		// pthread_mutex_lock(&mutexTime);
		// gettimeofday(&end, NULL);
		// long elapsTime = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		// start = end;
		// pthread_mutex_unlock(&mutexTime);
		const char *_tName = getThreadIdName(_taskid);
		fprintf(out_file, "%s", _tName);
		fflush(out_file);
#endif
		// check value of signals from skills
		// 1) Check bufferCCM_GoToNavigation
		pthread_mutex_lock(&mutexCCM_GoToToNavigation);
		//read the command value from CCM_GoTo
		_simulatorCmd = getNavigationCommand(bufferCCM_GoToToNavigation);
		_actualDestination = getDestination(bufferCCM_GoToToNavigation);
#ifdef SIMULATE
		const char *_dName = getDestinationName(_actualDestination);
		const char *_sName = getNavigationCommandName(_simulatorCmd);
		printf("						%s Refresh Data _actualDestination = %s --_simulatorCmd = %s\n", funcName, _dName, _sName);
		fflush(stdout);
#endif
		//reset the buffer values
		resetNavigationCmdValue(bufferCCM_GoToToNavigation);
		pthread_mutex_unlock(&mutexCCM_GoToToNavigation);

		// Simulator command == START NAVIGATION
		if (_simulatorCmd == start_navigation)
		{

			if (simInfo.currentDestination == ABSENT_LOCATION)
			{
#ifdef SIMULATE
		printf("						%s Refresh Data --> simInfo.currentDestination == ABSENT_LOCATION\n", funcName);
		fflush(stdout);
#endif
				// il robot e' fermo (nessuna destinazione impostata)
				simInfo.currentDestination = _actualDestination;
				currentSimLocation = _actualDestination;
				navigationCompSharedBuffer[0] = COMP_OK;
				navigationCompSharedBuffer[1] = RUNNING;
				navigationCompSharedBuffer[2] = 0;
			}
			else if (simInfo.currentDestination != _actualDestination)
			{
#ifdef SIMULATE
		printf("						%s Refresh Data --> simInfo.currentDestination != _actualDestination\n", funcName);
		fflush(stdout);
#endif
				// la destinazione e' diversa da quella salvata
				simInfo.currentDestination = _actualDestination;
				currentSimLocation = _actualDestination;
				simInfo.isArrived = false;
				navigationCompSharedBuffer[0] = COMP_OK;
				navigationCompSharedBuffer[1] = RUNNING;
				navigationCompSharedBuffer[2] = 0;
			}
		}
		else if (_simulatorCmd == stop_navigation)
		{
			// modificato: in principio era solo:  simInfo.currentDestination == _actualDestination
			// aggiunta condizione che dovrebbe gnorare lo stop se è a destinazione
			// e modificare la current destination de viene inviata una nuova destinazione
			if ((simInfo.currentDestination == _actualDestination && simInfo.isArrived == false) ||
			    (simInfo.currentDestination != _actualDestination && simInfo.isArrived == true))
			{
#ifdef SIMULATE
		printf("						%s Refresh Data --> simInfo.currentDestination == _actualDestination\n", funcName);
		fflush(stdout);
#endif
				simInfo.currentDestination = ABSENT_LOCATION;
				simInfo.isArrived = false;
				navigationCompSharedBuffer[0] = COMP_OK;
				navigationCompSharedBuffer[1] = ABORT;
				navigationCompSharedBuffer[2] = 0;
			}
		}

		//arrived to kitchen
		if (simInfo.currentDestination == KITCHEN && simInfo.cyclesToDestination == 0)
		{
#ifdef SIMULATE
			printf("						%s simInfo.currentDestination == KITCHEN &&  simInfo.cyclesToDestination == 0\n", funcName);
			fflush(stdout);
#endif
			simInfo.isArrived = true;
			simInfo.atDestination = 1;
			simInfo.cyclesToDestination = simInfo.cyclesToDestinationPlanned;
		} //arrived to chargingStation
		else if (simInfo.currentDestination == CHARGING_STATION && simInfo.cyclesToCharging == 0)
		{
#ifdef SIMULATE
			printf("						%s simInfo.currentDestination == CHARGING_STATION &&  simInfo.cyclesToCharging == 0\n", funcName);
			fflush(stdout);
#endif
			simInfo.isArrived = true;
			simInfo.atChargingStation = 1;
			simInfo.cyclesToCharging = simInfo.cyclesToChargingPlanned;
			simInfo.cyclesToDestination = (simInfo.chargingStationSign == 1) ? simInfo.cyclesToDestination + simInfo.cyclesToChargingPlanned
																			 : abs(simInfo.cyclesToDestination - simInfo.cyclesToChargingPlanned);
		}
		else if (simInfo.currentDestination == CHARGING_STATION && simInfo.cyclesToCharging != 0)
		{
#ifdef SIMULATE
			printf("						%s simInfo.currentDestination == CHARGING_STATION &&  simInfo.cyclesToCharging != 0\n", funcName);
			fflush(stdout);
#endif
			simInfo.atDestination = 0;
		}
		//BATTERY LEVEL
		if (simInfo.atChargingStation == 0)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atChargingStation == 0\n", funcName);
			fflush(stdout);
#endif
			//at each call decrease battery level
			simInfo.batteryLevel = simInfo.batteryLevel - simInfo.batteryDrop;
			batteryCompSharedBufferLevel[0] = COMP_OK;
			batteryCompSharedBufferLevel[1] = simInfo.batteryLevel;
		}
		else
		{
#ifdef SIMULATE
			printf("						%s simInfo.atChargingStation == 1 (else BATTERY LEVEL), %d\n", funcName, simInfo.currentDestination);
			fflush(stdout);
#endif
			//if the robot is at charging station increase the battery level
			simInfo.batteryLevel = simInfo.batteryLevel + simInfo.batteryRise;
			if (simInfo.batteryLevel > 1000)
			{
				simInfo.batteryLevel = 1000;
			}
			batteryCompSharedBufferLevel[0] = COMP_OK;
			batteryCompSharedBufferLevel[1] = simInfo.batteryLevel;
		}
		//BATTERY STATUS
		if (simInfo.atChargingStation == 1 && simInfo.batteryLevel < 1000)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atChargingStation == 1 && simInfo.batteryLevel < 100 BATTERY STATUS\n", funcName);
			fflush(stdout);
#endif
			batteryCompSharedBufferStatus[0] = COMP_OK;
			batteryCompSharedBufferStatus[1] = BATTERY_CHARGING;
		}
		else
		{
#ifdef SIMULATE
			printf("						%s ELSE simInfo.atChargingStation == 1 && simInfo.batteryLevel < 100\n", funcName);
			fflush(stdout);
#endif
			simInfo.atChargingStation = 0;
			batteryCompSharedBufferStatus[0] = COMP_OK;
			batteryCompSharedBufferStatus[1] = BATTERY_NOT_CHARGING;
		}

		if (simInfo.atDestination == 1 && simInfo.currentDestination == KITCHEN)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atDestination == 1\n", funcName);
			fflush(stdout);
#endif
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = SUCCESS;
			navigationCompSharedBuffer[2] = simInfo.atDestination;
		}
		else if (simInfo.atChargingStation == 1 && simInfo.currentDestination == CHARGING_STATION)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atChargingStation == 1 (and CHARGING STATION)\n", funcName);
			fflush(stdout);
#endif
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = SUCCESS;
			navigationCompSharedBuffer[2] = simInfo.atChargingStation;
		}
		else if (simInfo.atDestination == 0 && simInfo.atChargingStation == 0 && simInfo.currentDestination == KITCHEN)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atDestination == 0 && simInfo.atChargingStation == 0 && simInfo.currentDestination == KITCHEN : %d :\n", funcName, simInfo.cyclesToDestination);
			fflush(stdout);
#endif
			simInfo.cyclesToDestination = simInfo.cyclesToDestination - 1;
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = RUNNING;
			navigationCompSharedBuffer[2] = 0;
		}
		else if (simInfo.atDestination == 0 && simInfo.atChargingStation == 0 && simInfo.currentDestination == CHARGING_STATION)
		{
#ifdef SIMULATE
			printf("						%s simInfo.atDestination == 0 && simInfo.atChargingStation == 0 && simInfo.currentDestination == CHARGING_STATION\n", funcName);
			fflush(stdout);
#endif
			simInfo.cyclesToCharging = simInfo.cyclesToCharging - 1;
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = RUNNING;
			navigationCompSharedBuffer[2] = 0;
		}
		else
		{
#ifdef SIMULATE
			printf("						%s ELSE simInfo.atChargingStation == %d\n", funcName, simInfo.atChargingStation);
			fflush(stdout);
#endif
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = RUNNING;
			navigationCompSharedBuffer[2] = 0;
		}
		//unlock the mutex
		pthread_mutex_unlock(&mutexComponentReadWrite);
		pthread_yield();
		pthread_yield();
		usleep(5000);
	}
}

/* void *componentExecution(void *threadid)
{
	//variable representing command to navigator
	static NavigationCommand _simulatorCmd = navigation_absentCmd;
	static destination _destinationWatched = ABSENT_LOCATION;
	static destination _actualDestination = ABSENT_LOCATION;
#if defined(MONITORTHREAD)
	thread_id _taskid = (long)threadid;
	const char *_tName = getThreadIdName(_taskid);
#endif
#ifdef SIMULATE
	const char *funcName = __func__;
#endif
	while (1)
	{
#if defined(MONITORTHREAD)
		fprintf(out_file, "%s ", _tName);
		fflush(out_file);
#endif
		switch (currentCompState)
		{
		case INIT_SIMULATOR:
#ifdef SIMULATE
			printf("					%s Component state = INIT_SIMULATOR\n", funcName);
			fflush(stdout);
#endif
			currentCompState = WAIT_GO_TO_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		case WAIT_GO_TO_COMMAND:
#ifdef SIMULATE
			printf("					%s Component state = WAIT_GO_TO_COMMAND\n", funcName);
			fflush(stdout);
#endif
			//waiting for start_navigation and location
			// acquire the lock in order to start GoToDestination
			pthread_mutex_lock(&mutexCCM_GoToToNavigation);
			// check if the buffer tick is empty
			while (countCCM_GoToToNavigation == 0)
			{
				// wait on the CV
				pthread_cond_wait(&condNotEmptyFromCCM_GoToToNavigation, &mutexCCM_GoToToNavigation);
			}
			//read the command value from CCM_GoTo
			_simulatorCmd = getNavigationCommand(bufferCCM_GoToToNavigation);
			_actualDestination = getDestination(bufferCCM_GoToToNavigation);
#if defined(MONITORTHREAD)
			fprintf(out_file, "%s                             WAIT_GO_TO_COMMAND \n", _tName);
			fflush(out_file);
#endif
#ifdef SIMULATE
			const char *sigFromSkill = getNavigationCommandName(_simulatorCmd);
			const char *destFromSkill = getDestinationName(_actualDestination);
			printf("					%s Cmd from CCM_GoTo is  :%s: -- destination: %s \n", funcName, sigFromSkill, destFromSkill);
			fflush(stdout);
#endif

			//Simulator has been started..wake up the protocol skill
			getNavigationCmdValueAndWakeUpNoMutex(bufferCCM_GoToToNavigation, &countCCM_GoToToNavigation, &condFullFromCCM_GoToToNavigation);
			if (_simulatorCmd == start_navigation)
			{
#ifdef SIMULATE
				printf("					%s The robot has been started\n", funcName);
				fflush(stdout);
#endif
				//put COMP_OK to the buffer
				// pthread_mutex_lock(&mutexComponentReadWrite);
				// simInfo.currentDestination = _actualDestination;
				// currentSimLocation = _actualDestination;
				// navigationCompSharedBuffer[0] = COMP_OK;
				// navigationCompSharedBuffer[1] = RUNNING;
				// pthread_mutex_unlock(&mutexComponentReadWrite);
				currentCompState = GO_TO_DESTINATION;
			}
			// release the lock
			pthread_mutex_unlock(&mutexCCM_GoToToNavigation);
			pthread_yield();
			pthread_yield();
			break;

		case GO_TO_DESTINATION:
#ifdef SIMULATE
			printf("					%s Component state = GO_TO_DESTINATION\n", funcName);
			fflush(stdout);
#endif
			//in questo stato il robot si sta muovendo verso la sua destinazione(kitchen o charging station)
			//e rimane in questo stato fino a quando riceve uno stop (il robot deve andare a caricarsi
			//e quindi deve interrompere la destinazione verso kitchen oppure e' arrivato un comando di stop)
			//oppure se e' arrivato a destinazione e cambia stato
			//watch the channel if a stop_navigation has been arrived
			_simulatorCmd = watchNavigationCommandValue(bufferCCM_GoToToNavigation, &mutexCCM_GoToToNavigation);
			_destinationWatched = watchDestinationValue(bufferCCM_GoToToNavigation, &mutexCCM_GoToToNavigation);
#ifdef SIMULATE
			sigFromSkill = getNavigationCommandName(_simulatorCmd);
			destFromSkill = getDestinationName(_destinationWatched);
			printf("					%s Watching Cmd from CCM_GoTo is  :%s: -- destination: %s\n", funcName, sigFromSkill, destFromSkill);
			fflush(stdout);
#endif
			if (_simulatorCmd == stop_navigation)
			{
				//wake up CCM
				getNavigationCmdValueAndWakeUp(bufferCCM_GoToToNavigation, &countCCM_GoToToNavigation, &mutexCCM_GoToToNavigation, &condFullFromCCM_GoToToNavigation);

				if (_destinationWatched == _actualDestination)
				{
#ifdef SIMULATE
					printf("					%s GO_TO_DESTINATION --> stop_navigation && _destinationWatched == _actualDestination\n", funcName);
					fflush(stdout);
#endif
					currentCompState = STOPPING;
				}
				else
				{
#ifdef SIMULATE
					printf("					%s GO_TO_DESTINATION --> stop_navigation && _destinationWatched != _actualDestination\n", funcName);
					fflush(stdout);
#endif
					pthread_mutex_lock(&mutexComponentReadWrite);
					bool isArrived = simInfo.isArrived;
					pthread_mutex_unlock(&mutexComponentReadWrite);
					if (isArrived)
					{
						currentCompState = DESTINATION_REACHED;
					}
					else
					{
						currentCompState = GO_TO_DESTINATION;
					}
				}
			}
			else if (_simulatorCmd == start_navigation)
			{
				//wake up ccm
				getNavigationCmdValueAndWakeUp(bufferCCM_GoToToNavigation, &countCCM_GoToToNavigation, &mutexCCM_GoToToNavigation, &condFullFromCCM_GoToToNavigation);

				if (_destinationWatched != _actualDestination)
				{
#ifdef SIMULATE
					printf("					%s GO_TO_DESTINATION --> start_navigation && _destinationWatched != _actualDestination\n", funcName);
					fflush(stdout);
#endif
					_actualDestination = _destinationWatched;
					//put COMP_OK to the buffer
					pthread_mutex_lock(&mutexComponentReadWrite);
					simInfo.currentDestination = _actualDestination;
					currentSimLocation = _actualDestination;
					simInfo.isArrived = false;
					navigationCompSharedBuffer[0] = COMP_OK;
					navigationCompSharedBuffer[1] = RUNNING;
					pthread_mutex_unlock(&mutexComponentReadWrite);
					currentCompState = GO_TO_DESTINATION;
				}
				else
				{
#ifdef SIMULATE
					printf("					%s GO_TO_DESTINATION --> start_navigation && _destinationWatched == _actualDestination\n", funcName);
					fflush(stdout);
#endif
					pthread_mutex_lock(&mutexComponentReadWrite);
					bool isArrived = simInfo.isArrived;

					pthread_mutex_unlock(&mutexComponentReadWrite);
					if (isArrived)
					{
						currentCompState = DESTINATION_REACHED;
					}
					else
					{
						currentCompState = GO_TO_DESTINATION;
					}
					//sleep(1);
				}
			}
			else
			{
#ifdef SIMULATE
				destFromSkill = getDestinationName(_actualDestination);
				printf("					%s GO_TO_DESTINATION --> check if it has arrived at destination : %s\n", funcName, destFromSkill);
				fflush(stdout);
#endif
				pthread_mutex_lock(&mutexComponentReadWrite);
				bool isArrived = simInfo.isArrived;
				pthread_mutex_unlock(&mutexComponentReadWrite);
				if (isArrived)
				{
#ifdef SIMULATE
					destFromSkill = getDestinationName(_actualDestination);
					printf("					%s GO_TO_DESTINATION --> The robot has reached the destination %s\n", funcName, destFromSkill);
					fflush(stdout);
#endif
					currentCompState = DESTINATION_REACHED;
				}
				else
				{
#ifdef SIMULATE
					destFromSkill = getDestinationName(_actualDestination);
					printf("					%s GO_TO_DESTINATION --> The robot is still running towards the destination %s\n", funcName, destFromSkill);
					fflush(stdout);
#endif
					currentCompState = GO_TO_DESTINATION;
				}
			}
			pthread_yield();
			pthread_yield();

			break;

		case DESTINATION_REACHED:
#ifdef SIMULATE
			printf("					%s Component state = DESTINATION_REACHED\n", funcName);
			fflush(stdout);
#endif
			//in questo stato il robot ha raggiunto la destinazione (kitchen oppure charging station)
			//e trasmette successo al CCM_GoTo
			//Il robot continua a consumare energia anche nello stato destinationReached
			//set SUCCESS
			pthread_mutex_lock(&mutexComponentReadWrite);
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = SUCCESS;
			pthread_mutex_unlock(&mutexComponentReadWrite);
			//watch the channel if a stop_navigation has been arrived
			_simulatorCmd = watchNavigationCommandValue(bufferCCM_GoToToNavigation, &mutexCCM_GoToToNavigation);
			_destinationWatched = watchDestinationValue(bufferCCM_GoToToNavigation, &mutexCCM_GoToToNavigation);
#ifdef SIMULATE
			sigFromSkill = getNavigationCommandName(_simulatorCmd);
			destFromSkill = getDestinationName(_destinationWatched);
			printf("					%s Watching Cmd from CCM_GoTo is  :%s: -- destination: %s\n", funcName, sigFromSkill, destFromSkill);
			fflush(stdout);
#endif
			if (_simulatorCmd == stop_navigation)
			{
				//wake up CCM
				getNavigationCmdValueAndWakeUp(bufferCCM_GoToToNavigation, &countCCM_GoToToNavigation, &mutexCCM_GoToToNavigation, &condFullFromCCM_GoToToNavigation);

				if (_destinationWatched == _actualDestination)
				{
#ifdef SIMULATE
					printf("					%s DESTINATION_REACHED -> stop_navigation && _destinationWatched == _actualDestination\n", funcName);
					fflush(stdout);
#endif
					currentCompState = STOPPING;
				}
				else
				{
#ifdef SIMULATE
					printf("					%s DESTINATION_REACHED --> stop_navigation && _destinationWatched != _actualDestination\n", funcName);
					fflush(stdout);
#endif
					currentCompState = DESTINATION_REACHED;
					//sleep(1);
				}
			}
			else if (_simulatorCmd == start_navigation)
			{
				//wake up CCM
				getNavigationCmdValueAndWakeUp(bufferCCM_GoToToNavigation, &countCCM_GoToToNavigation, &mutexCCM_GoToToNavigation, &condFullFromCCM_GoToToNavigation);

				if (_destinationWatched != _actualDestination)
				{
#ifdef SIMULATE
					printf("					%s DESTINATION_REACHED --> start_navigation && _destinationWatched != _actualDestination\n", funcName);
					fflush(stdout);
#endif
					_actualDestination = _destinationWatched;
					//
					pthread_mutex_lock(&mutexComponentReadWrite);
					simInfo.currentDestination = _actualDestination;
					currentSimLocation = _actualDestination;
					simInfo.isArrived = false;
					navigationCompSharedBuffer[0] = COMP_OK;
					navigationCompSharedBuffer[1] = RUNNING;
					pthread_mutex_unlock(&mutexComponentReadWrite);
					currentCompState = GO_TO_DESTINATION;
				}
				else
				{
#ifdef SIMULATE
					printf("					%s DESTINATION_REACHED --> start_navigation && _destinationWatched == _actualDestination\n", funcName);
					fflush(stdout);
#endif
					currentCompState = DESTINATION_REACHED;
					//sleep(1);
				}
			}
			else
			{
#ifdef SIMULATE
				printf("					%s DESTINATION_REACHED --> still in destination reached\n", funcName);
				fflush(stdout);
#endif
				currentCompState = DESTINATION_REACHED;
				//sleep(1);
			}
			pthread_yield();
			pthread_yield();
			break;

		case STOPPING:
#ifdef SIMULATE
			printf("					%s Component state = STOP_SIMULATION\n", funcName);
			fflush(stdout);
#endif
			//il simulatore entra in questo stato se e' stato inviato un comando di
			//stop e quindi se deve terminare la navigazione. Il simulatore una volta
			//comunicato al CCM_GoTo lo stato ABORT, ritorna nello
			//stato wait_go_to_command in attesa di un nuovo start per una nuova destinazione
			//set ABORT
			pthread_mutex_lock(&mutexComponentReadWrite);
			simInfo.isArrived = false;
			navigationCompSharedBuffer[0] = COMP_OK;
			navigationCompSharedBuffer[1] = ABORT;
			pthread_mutex_unlock(&mutexComponentReadWrite);
#ifdef SIMULATE
			printf("					%s STOP_SIMULATION -> The robot has stopped\n", funcName);
			fflush(stdout);
#endif
			//go to wait command
			currentCompState = WAIT_GO_TO_COMMAND;
			pthread_yield();
			pthread_yield();
			break;

		default:
			break;
		}
		usleep(10000);
	}
}
 */
int main(int argc, char *argv[])
{
	// if (argc != 2)
	// {
	// 	printf("Usage: ./file.out  outputFile.txt\n");
	// 	exit(1);
	// }
	// char *fileName = argv[1];
	// gettimeofday(&start, NULL);
	// out_file = fopen(fileName, "w");
//FILE *choosen_parameter_file = fopen("choosen_param.txt", "a");
#if defined(SIMULATE) || defined(SIMULATE_CHIARA)|| defined(MONITOR) || defined(MONITOR2) || defined(MONITOR3)

	srandom(time(NULL));
	simInfo.isArrived = false;
	simInfo.atDestination = 0;
	simInfo.atChargingStation = 0;
	simInfo.currentDestination = ABSENT_LOCATION;
	simInfo.batteryLevel = 1000;
	simInfo.cyclesToDestinationPlanned = 100;
	simInfo.cyclesToDestination = simInfo.cyclesToDestinationPlanned;
	simInfo.batteryDrop = randomInteger(5, 10); ////SAFE=10
	simInfo.batteryRise = 100;
	simInfo.cyclesToChargingPlanned = randomInteger(1, 9); ///SAFE=5
	simInfo.cyclesToCharging = simInfo.cyclesToChargingPlanned;
	simInfo.chargingStationSign = 0; /////////////    randomInteger(0,1);
	//fprintf(choosen_parameter_file,"batteryDrop = %d -- cyclesToChargingPlanned = %d\n",simInfo.batteryDrop,simInfo.cyclesToChargingPlanned);
	//fflush(choosen_parameter_file);

#endif

	thread_id threadIdArray[17] = {bt_id, skill1_id, skill2_id, skill3_id, skill4_id, scmCondition2_id,
								   scmCondition3_id, scmAction1_id, scmAction4_id, ccmBr_id, ccmGoTo_id, //component_id ,
								   visit_id, skill5_id, skill6_id, scmCondition5_id, scmCondition6_id, refresh_id};

	// pthread_t visitThread, btThread,  skill1Thread,  skill2Thread, skill3Thread,  skill4Thread, componentThread, refreshThread,
	// 		scmAction1Skill1Thread,scmCondition2Skill2Thread, scmCondition3Skill3Thread,scmAction4Skill4Thread,
	//         ccmGoToThread, ccmBrThread, skill5Thread,  skill6Thread,scmCondition5Skill5Thread, scmCondition6Skill6Thread;

	pthread_t threadArray[17];

	pthreadFun pthreadFunArray[17] = {&btExecution, &skill1Fsm, &skill2Fsm, &skill3Fsm, &skill4Fsm, &scmCondition2Skill2Execution,
									  &scmCondition3Skill3Execution, &scmAction1Skill1Execution, &scmAction4Skill4Execution, &ccmBrExecution,
									  &ccmGoToExecution, //&componentExecution,
									  &visitExecution, &skill5Fsm, &skill6Fsm, &scmCondition5Skill5Execution,
									  &scmCondition6Skill6Execution, &refreshExecution};

	int threadPosition[17] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

	shuffle(threadPosition, 17);

	for (int i = 0; i < 17; i++)
	{
		long currPos = threadPosition[i];
		//thread_id _taskid = (long) threadIdArray[currPos-1];
		//const char* _tName = getThreadIdName(_taskid);
		//fprintf(out_file, "currPos = %ld -- threadID = %s\n",currPos,_tName);
		//fflush(out_file);
		pthread_create(&threadArray[currPos - 1], NULL, pthreadFunArray[currPos - 1], (void *)threadIdArray[currPos - 1]);
	}

	for (int i = 0; i < 17; i++)
	{
		int currPos = threadPosition[i];
		pthread_join(threadArray[currPos - 1], NULL);
	}

	fclose(out_file);
	//fclose(choosen_parameter_file);
	pthread_exit(NULL);

	return 0;
}
