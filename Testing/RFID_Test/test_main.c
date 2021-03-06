/*
 * main.c
 *
 *  by Will Dolan, June 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "rfid.h"
#include "curl_req.h"
#include "led_gpio.h"
#include "camera.h"
#include <errno.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <unistd.h>

// #define x_ms 500L
void sleep_for_x_ms(int x_ms);
void readIP();
//void get_sid();

char RFID_UID[10];
char stid[10];
char IP[64];

/* Main :
 *      First, initialize RFID and GPIO components
 *      If the -IP flag is raised, display the IP address, read from stdin
 *      Then, enter loop. Wait for RFID, then 
 *      send a //request to JMN DB w/ RFID, then take appropriate action
 *      based on response. Look for help/photo button depress throughout loop.
*/
int main(int argc, char *argv[]) 
{
//	get_sid();
	char *RFID_UID = (char *)calloc(10,1);
	char *JMN_resp = (char *)calloc(512,1);

	init_RFID();
	init_GPIO();
	delay(9000);
	activate_LCD();

	// Collect the command line arguments to assign SID and Display current IP address
	printf("\nWe should start seeing arguments here...");
	int i=0;
	if (argc > 1) {
		printf("\ncmdline args count=%d", argc);
		for (i=1; i< argc; i++) {
			printf("\narg%d= ", i);
			printf("%s", argv[i]);
			if(strcmp(argv[i], "-STID") == 0) {
				i++;
				printf("\narg%d= ", i);
				printf("%s", argv[i]);
				strcpy(stid, argv[i]);
				printf("\nSuccessfully assigned station %s", stid);
			}
			else if(strcmp(argv[i], "-IP") == 0) {
				i++;
				// Check if there is an IP attached
				if (argv[i]){
					// Grab the first IP
					printf("\narg%d= ", i);	
					printf("%s", argv[i]);
					strcpy(IP, argv[i]);
					i++;
					// Check for second
					if (argv[i]){
						// Grab the Second IP
						printf("\narg%d= ", i);	
						printf("%s", argv[i]);
						strcat(IP, " ");
						strcat(IP, argv[i]);
					}
					// Send that info to displayIP()
					displayIP(IP);
				}
			}
		}
	}

//	if(argc > 1) {
//		if(strcmp(argv[1], "-IP") == 0) {
//		        readIP();
//		}
//    }

	int status = 0;
	int use_time = 0;
	// int admin_help = 0;
	char use_time_s[16];
	// int initHelpState = readHelp(0);
	// int initPhotoState = readPhoto(0);
	
	while(1) {
		sleep_for_x_ms(500);
		RFID_refresh();	
		status = look_for_RFID();
		// printf("Current Status: %d\n", status);

        if(status == 1) {
            get_RFID(RFID_UID);
            printf("New tag: %s\n", RFID_UID);             
			if(strcmp(RFID_UID, "") == 0) {
				display("RFID read error","Try again.");
				delay(1000);
		        display("Waiting for", "RFID...");
			}
			else {
				// Just set to true for any RFID. Just checking to see RFID reads
				// JMN_resp = "T Brian";
				strcpy(JMN_resp, "True Brian");
				// ReqJMN(JMN_resp, RFID_UID, "1", "begin", stid);

				if (strchr(JMN_resp,'T') != NULL) {
					time_t begin_t = beginUse(JMN_resp);
					printf("\nStarting the new while status = 1 loop..."); 
					In_Use:while(status == 1){
						sleep_for_x_ms(500);	
						// if(readHelp(initHelpState) == 1){
						// 	if(admin_help == 0) {
						// 		sendHelp(RFID_UID);
						// 		ReqJMN(JMN_resp, RFID_UID, "4", "help_email", stid);
						// 		admin_help = 1;
						// 	} 
						// 	else {
						// 		contact_admin();
						// 		ReqJMN(JMN_resp, "0", "4", "contact_admin", stid);
						// 		// ReqJMN(JMN_resp, RFID_UID, "4", "contact_admin", stid);
						// 	}
						// }
						// if(readPhoto(initPhotoState) == 1){
						// 	display("Photo Snapped!","Uploading...");
						// 	char *time_file = (char *)calloc(25,1);
						// 	takePicture(time_file, RFID_UID);
						// 	//ReqJMN(JMN_resp, RFID_UID, "5", time_file, stid);
						// 	free(time_file);
						// 	display("Image uploaded!","See email.");
						// 	delay(2000);
						// 	display("Commence","Use...");
						// }
						//need to call twice! Don't Touch!! *** It's a weird thing. No idea why yet. but it works.
						status = look_for_RFID();
						// printf("\nStatus Check 1: %d", status);
						status = look_for_RFID();
						// printf("\nStatus Check 2: %d", status);
						//need to call twice! Don't Touch!! ***

						// Need to call again to be sure. Some hiccups have been occuring that cause a momentary skip. Not enough to kick someone off a machine, but it messes with our event logging. 
						if(status == 0){
							printf("\nLost RFID. Double checking that RFID is gone");
							int j = 0;
							for (j=1; j < 11; j++) {
								printf("\nError Check: %d",j);
								RFID_refresh();
								delay(500);	// Delay a second, giving chance to pick up the card again
								if(RFID_comparison(RFID_UID) == 0){
									printf("\nRFID didn't change. Error was in check.\n");
									status = 1;
									goto In_Use;
								}
							}
							// End of double check
						}
					}
					use_time = endUse(begin_t);
					printf("\nTime until removal: %d\n",use_time);
					sprintf(use_time_s, "%d", use_time);
					// ReqJMN(JMN_resp, RFID_UID, "2", use_time_s, stid);
					// admin_help = 0;
				}
				else if (strchr(JMN_resp,'E') != NULL) {
					rejectUse();
				} 
				else noUserHandler();

				RFID_refresh();
			}
		}

		// if(readHelp(initHelpState) == 1) {
		// 	contact_admin();
		// 	ReqJMN(JMN_resp, "0", "4", "contact_admin", stid);
		// }
	}
	RFID_end();
	GPIO_end();
	free(RFID_UID);
	free(JMN_resp);
	return 0;
}


//void get_sid()
//{
//	FILE *fp = fopen("sid.txt", "r");
//	if(fp == NULL) {
//		printf("Error opening station identifier\n");
//	}
//	else if(fgets(stid, 10, fp) != NULL) {
//		printf("Successfully assigned station %s \n", stid);
//		fclose(fp);
//	}
// }

void sleep_for_x_ms(int x_ms)
{
	struct timespec tim, tim2;
	tim.tv_sec  = 0;
	tim.tv_nsec = (x_ms * 1000000L); 
	nanosleep(&tim , &tim2);
}

//void readIP()
//{
//        char input[64];
//        fgets(input,64,stdin);
//        displayIP(input);
//}
//char *//ReqJMN(char *RFID, char *//req, char *info, char* station);

