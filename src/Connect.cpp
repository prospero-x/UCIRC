
#include <iostream>
#include "CameraCenter.h"
#include <string>
#include <time.h> // For jpeg filename timestamp 
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdio.h>


#define BUFF_SIZE 2
#define NUM_PIXELS 512*640
#define TIME_VALS 6
#define PICS_PER_BURST 4
#define COMMAND_SIZE 1024
#define NUM_CAMERAS 2

#define DEST_HOST "192.168.1.20:"
#define DEST_DIR "data/"

#define DATA_DIR "/opt/workswell/wic_sdk/sample/build/data"
#define LICENSE_DIR "/opt/workswell/wic_sdk/sample/build/license"

/*
 * Variables which will be changed with received message-commands.
 */
int PICS_PER_FILE = 10;
int CaptureInterval = 60; // Seconds between pictures
bool Night = true; // Only take pictures when night == true

/*
 * Variable to keep track of image number
 */
int ImageIndex = 0;


/* 
 * Prepare file strings
 */
static char *_path_to_data;
static char *filename1; 
static char *filename2;
static char *suffix1;
static char *suffix2;

/* These Global Variables will contain the all the information for each camera */
Camera* Camera1;
Camera* Camera2;
CameraCenter* cam_center;

void CheckOnCameras();

char *SetDirectoryPath();

char *UpdateFileName(int n);

struct tm *Time();

char *TimeStamp();

u_int16_t *TimeArray();

void *MessageCenter(void*);

void *Capture(void *threadid);

void WriteBuffer( u_int16_t *buffer, long pic_id);

void SendFiles();

void Connect();

void Disconnect();

static int num_cameras = 0;

int main() {
    	 
    /* Set the global data-path variable */
    _path_to_data = SetDirectoryPath();  

	/* Create a list of connected camera objects */
	cam_center = new CameraCenter(LICENSE_DIR);

	/* Set the global num_cameras variable */
    num_cameras = cam_center -> getCameras().size();	
   
    /* Connect the Cameras */ 
    Connect();
    Camera* cam = Camera1;

    /* At any moment, each camera has a unique file that it is 
     * writing */
    filename1 = UpdateFileName(0);
    filename2 = UpdateFileName(1);
  
       
   
    CheckOnCameras();

    /* Spawn two threads so cameras can take pictures simultaneously */
	pthread_t camera_threads[num_cameras];
	int *camera_ptr[num_cameras];  
	for(long t = 0; t < num_cameras; t++){
		int rc = pthread_create(&camera_threads[t],NULL, Capture, (void*)t);
		if(rc){
			cout << "ERROR: return code from pthread_create() is " << rc << endl;
			exit(-1);
		}
	}
	for(int i = 0; i < num_cameras; i++)
		pthread_join(camera_threads[i], (void**)&(camera_ptr[0]));
    
	Disconnect();
	return 0;

	
}

/*
   In the case of camera disconnection, reconnect the camera.
*/
void CheckOnCameras(){
	/* Refresh the number of cameras in case one breaks */
    	num_cameras = cam_center -> getCameras().size();

    	if (num_cameras == 0)
    	{
    		/* Send an error message, or reboot, or DO SOMETHING*/
    		perror("No Cameras!");
    		exit(-1);
    	}

    	char status1[20], status2[20];
    	snprintf(status1, sizeof(Camera1->GetStatus()), Camera1->GetStatus().c_str());
    	if(!strcmp(status1, "Connected")){
    		Connect();
        }
    	if (num_cameras == 2){
    		snprintf(status2, sizeof(Camera2->GetStatus()), Camera2->GetStatus().c_str());
    		if(!strcmp(status2, "Connected")){
    			Connect();
			}
    	}

}

/*
   Update the filename to reflect the the current time. 
*/
char *UpdateFileName(int n){

	char filename[200];
	filename[0] = '\0';
	strcat(filename, _path_to_data);
	const char* id;
	if (n == 0){
		id = "_0";
		suffix1 = TimeStamp();
		strcat(suffix1, id);
		strcat(filename, suffix1);
	}
	else{
		id = "_1";
		suffix2 = TimeStamp();
		strcat(suffix2, id);
		strcat(filename, suffix2);
	}
	return strdup(filename);
}


/* SetDirectoryPath: return a string of the current directory path */
char* SetDirectoryPath(){
	char cwd[] = DATA_DIR; 
	char* rv = (char*)malloc(sizeof(char)*1024);
	strcpy(rv, cwd);
	return rv;
}

/* Grab the current time from the CPU and save it in a tm struct */
struct tm *Time(){
	struct tm* timeinfo = (struct tm*)malloc(sizeof(struct tm));
	time_t c;
	time(&c);
	timeinfo = localtime(&c);
	return timeinfo;
}

/* TimeStamp: return a string of the current time */
char *TimeStamp(){
	char* timestamp = (char*)malloc(sizeof(char)*64);
	struct tm *TimeInfo = Time();
	snprintf(timestamp, 64, "/%d_%d_%d_%d_%d_%d",TimeInfo -> tm_year + 1900, 
    	                                              TimeInfo -> tm_mon + 1, 
    	                                              TimeInfo -> tm_mday, 
    	                                              TimeInfo -> tm_hour, 
    	                                              TimeInfo -> tm_min, 
    	                                              TimeInfo -> tm_sec);
	return timestamp;
}

/* Return an array of 16-bit numbers representing the current time */
u_int16_t *TimeArray(){
	u_int16_t *timearray = (u_int16_t*)malloc(sizeof(u_int16_t)*6);
	struct tm *TimeInfo = Time();
	timearray[0] = TimeInfo -> tm_year + 1900;
	timearray[1] = TimeInfo -> tm_mon + 1;
	timearray[2] = TimeInfo -> tm_mday;
	timearray[3] = TimeInfo -> tm_hour;
	timearray[4] = TimeInfo -> tm_min;
	timearray[5] = TimeInfo -> tm_sec;
	return timearray;
}

/* Connect one or both cameras */
void Connect(){
	if( num_cameras == 0)
		exit(-1);
	Camera1 = cam_center->getCameras().at(0);
    int i = Camera1->Connect();
	if(num_cameras == 2){
		Camera2 = cam_center->getCameras().at(1);
		i = Camera2->Connect();
	}
}

/* Disconnect one or both cameras */
void Disconnect(){
	if (num_cameras == 0)
		exit(-1);
	Camera1->Disconnect();
	if(num_cameras == 2){
		Camera2->Disconnect();
    }

}

/* Capture: take a picture with the camera indicated by the thread ID */
void *Capture(void* thread_id){
	struct timeval one, two, three, four;
	long tid = (long)thread_id;     	
    Camera* camera;
    if(tid == 0)
    	camera = Camera1;
    if(tid == 1)
    	camera = Camera2;

    /* Picture number 1 */
    camera->StartAcquisition();
	u_int16_t *buffer1 = (u_int16_t *)camera->RetreiveBuffer();
	gettimeofday(&one, NULL);
	camera->ReleaseBuffer();

	/* Picutre number 2 */
	u_int16_t *buffer2 = (u_int16_t *)camera->RetreiveBuffer();
	gettimeofday(&two, NULL);
	camera->ReleaseBuffer();

	/* Picutre number 3 */
	u_int16_t *buffer3 = (u_int16_t *)camera->RetreiveBuffer();
	gettimeofday(&three, NULL);
	camera->ReleaseBuffer();

	/* Picutre number 4 */
	u_int16_t *buffer4 = (u_int16_t *)camera->RetreiveBuffer();
	gettimeofday(&four, NULL);
	camera->ReleaseBuffer();
	camera->StopAcquisition();

	/* Superimpose all four images */
	int i;
	u_int16_t buffer[NUM_PIXELS]; 
	for (i = 0; i < NUM_PIXELS; i++){
		buffer[i] = buffer1[i] + buffer2[i] + buffer3[i] + buffer4[i];
	}
	WriteBuffer(buffer, tid);
	pthread_exit(NULL);
}

/* Write the raw pixel buffer to the data file. */
void WriteBuffer( u_int16_t* buffer, long pic_id){
	char* filename = (pic_id == 0 ? filename1 : filename2);
	FILE* fp = fopen(filename, "a"); 
	fwrite(TimeArray(), BUFF_SIZE, TIME_VALS, fp);       
	fwrite(buffer, BUFF_SIZE, NUM_PIXELS, fp);
	fclose(fp);
}

