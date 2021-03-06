/*
    FreeRTOS V8.2.1 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?".  Have you defined configASSERT()?  *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *   Investing in training allows your team to be as productive as       *
     *   possible as early as possible, lowering your overall development    *
     *   cost, and enabling you to bring a more robust product to market     *
     *   earlier than would otherwise be possible.  Richard Barry is both    *
     *   the architect and key author of FreeRTOS, and so also the world's   *
     *   leading authority on what is the world's most popular real time     *
     *   kernel for deeply embedded MCU designs.  Obtaining your training    *
     *   from Richard ensures your team will gain directly from his in-depth *
     *   product knowledge and years of usage experience.  Contact Real Time *
     *   Engineers Ltd to enquire about the FreeRTOS Masterclass, presented  *
     *   by Richard Barry:  http://www.FreeRTOS.org/contact
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *    You are receiving this top quality software for free.  Please play *
     *    fair and reciprocate by reporting any suspected issues and         *
     *    participating in the community forum:                              *
     *    http://www.FreeRTOS.org/support                                    *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* FreeRTOS includes. */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include <stdio.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_io.h"

/* Application includes*/
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"

#include "lwip/udp.h"
#include "xil_cache.h"
#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif

void print_app_header();
int start_application();
void lwip_init();



#define DELAY_100_MILLISECONDS	100UL
#define DELAY_1_SECOND		1000UL

/*-----------------------------------------------------------*/
#define MY_COUNTER 0x43C10000 //Address of base register of My_Counter IP
#define MY_OUTPUT 0x43C00000  //Address of base register of My_Output IP

/* The Tx and Rx tasks as described at the top of this file. */
static void ReadSpeed( void *pvParameters );
static void SendData( void *pvParameters );
static void ReceiveData(void *pvParameters);
static void DriveFan(void *pvParameters);
/*-----------------------------------------------------------*/

/* The queue used by the Tx and Rx tasks, as described at the top of this
file. */
static TaskHandle_t xReadSpeed;
static TaskHandle_t xSendData;
static TaskHandle_t xReceiveData;
static TaskHandle_t xDriveFan;
static QueueHandle_t xQueue = NULL;
static QueueHandle_t xQueue2 = NULL;
int value_old, value_new;


/* Imported Variables*/
static struct netif server_netif;
struct netif *echo_netif;
extern struct ip_addr ip_remote;
char data[10];
char sendmsg[]="SensorRPM ";
struct pbuf *p;
extern struct udp_pcb *pcb;
int count;
long recvd_num;

void
print_ip(char *msg, struct ip_addr *ip)
{
	xil_printf(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}


void
print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

void udp_echo_recv(void *arg, struct udp_pcb *pcb_recv, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	if (p != NULL)
    {
		xil_printf("Receiving Data\n");
    	const char space = ' ';
    	char *ptr, *ret;

    	ret = strchr(p->payload, space);
    	recvd_num = strtol(ret+1, &ptr, 10);

    }
}


int main( void )
{


	xil_printf( "Welcome\n" );


	xTaskCreate( 	ReadSpeed, 					/* The function that implements the task. */
					( const char * ) "Sensor", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY,			/* The task runs at the idle priority. */
					&xReadSpeed );

	xTaskCreate( SendData,
				 ( const char * ) "UDP_Send",
				 configMINIMAL_STACK_SIZE*2,
				 NULL,
				 tskIDLE_PRIORITY + 1,
				 &xSendData );

	xTaskCreate( ReceiveData,
					 ( const char * ) "UDP_Receive",
					 configMINIMAL_STACK_SIZE,
					 NULL,
					 tskIDLE_PRIORITY,
					 &xReceiveData );

	xTaskCreate( DriveFan,
						 ( const char * ) "PWM",
						 configMINIMAL_STACK_SIZE,
						 NULL,
						 tskIDLE_PRIORITY+1,
						 &xDriveFan );

	/* Create the queue used by the tasks.  The Rx task has a higher priority
	than the Tx task, so will preempt the Tx task and remove values from the
	queue as soon as the Tx task writes to the queue - therefore the queue can
	never have more than one item in it. */
	xQueue = xQueueCreate( 	1,						/* There is only one space in the queue. */
							sizeof( int ) );	/* Each space in the queue is large enough to hold a uint32_t. */

	xQueue2 = xQueueCreate(1, sizeof(int));
	/* Check the queue was created. */
	configASSERT( xQueue );
	configASSERT( xQueue2 );





	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}


/*-----------------------------------------------------------*/
static void ReadSpeed( void *pvParameters )
{
const TickType_t x100ms = pdMS_TO_TICKS( DELAY_100_MILLISECONDS );

	for( ;; )
	{
		value_old = Xil_In32(MY_COUNTER);
		/* Delay for 1 second. */
		vTaskDelay( x100ms );
		value_new =	Xil_In32(MY_COUNTER);
		value_new = value_new - value_old;
		/* Send the next value on the queue.  The queue should always be
		empty at this point so a block time of 0 is used. */
		xQueueSend( xQueue,			/* The queue being written to. */
					&value_new, /* The address of the data being sent. */
					0UL );			/* The block time. */
	}
}

/*-----------------------------------------------------------*/
static void SendData( void *pvParameters )
{
int output, output_rpm;
struct ip_addr ipaddr, netmask, gw;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	echo_netif = &server_netif;
	#if defined (__arm__) && !defined (ARMR5)
	#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
		ProgramSi5324();
		ProgramSfpPhy();
	#endif
	#endif

	/* Define this board specific macro in order perform PHY reset on ZCU102 */
	#ifdef XPS_BOARD_ZCU102
		IicPhyReset();
	#endif


	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   1, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);

	print_app_header();

	lwip_init();

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
							&gw, mac_ethernet_address,
							PLATFORM_EMAC_BASEADDR))
		{
			xil_printf("Error adding N/W interface\n\r");

		}

	netif_set_default(echo_netif);

	/* now enable interrupts */
	//platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(echo_netif);


	xil_printf("Configuring default IP of 192.168.1.10\r\n");
	IP4_ADDR(&(echo_netif->ip_addr),  192, 168,   1, 10);
	IP4_ADDR(&(echo_netif->netmask), 255, 255, 255,  0);
	IP4_ADDR(&(echo_netif->gw),      192, 168,   1,  1);


	ipaddr.addr = echo_netif->ip_addr.addr;
	gw.addr = echo_netif->gw.addr;
	netmask.addr = echo_netif->netmask.addr;


	print_ip_settings(&ipaddr, &netmask, &gw);

	/* start the application (web server, rxtest, txtest, etc..) */
	start_application();
	for( ;; )
	{
		/* Block to wait for data arriving on the queue. */
		xQueueReceive( 	xQueue,				/* The queue being read. */
						&output,	/* Data is read into this address. */
						portMAX_DELAY );	/* Wait without a timeout for data. */

		/* Print the received data. */
		xil_printf( "%d\n", output );
		output_rpm = output * 600;

		while(count!=8000)
			{
				xemacif_input(echo_netif);
				count++;
			}
		sprintf(data,"%d",output_rpm);

		strcat(sendmsg,data);
			u16_t pbuflen = strlen(sendmsg+1);
			p = pbuf_alloc(PBUF_TRANSPORT, pbuflen, PBUF_POOL);

			if (!p)
			{
				xil_printf("error allocating pbuf \r\n");
			}

			memcpy(p->payload, sendmsg, pbuflen+1);
			udp_send(pcb, p);
			//xil_printf("\n SEND UDP Packets \r\n");
			count = 0;
			strcpy(sendmsg,"SensorRPM ");
			pbuf_free(p);


	}
}

static void ReceiveData(void *pvParameters)
{
	err_t err_recv;

	const TickType_t x100ms = pdMS_TO_TICKS( DELAY_100_MILLISECONDS );
	unsigned port_recv = 17224;
	struct udp_pcb *pcb_recv;

	pcb_recv = udp_new();
	if (!pcb)
	{
		xil_printf("Error creating PCB. Out of Memory\n\r");
	}

	err_recv = udp_bind(pcb_recv, IP_ADDR_ANY, port_recv);
	if (err_recv != ERR_OK)
	{
		xil_printf("Unable to bind to port %d: err = %d\n\r", port_recv, err_recv);
	}

	IP4_ADDR(&ip_remote, 192, 168, 1, 1);

	err_recv = udp_connect(pcb_recv, &ip_remote, port_recv);
	if (err_recv != 0)
	{
		xil_printf("Failed %d\r\n", err_recv);
	}

	for(;;)
	{
		udp_recv(pcb_recv, udp_echo_recv, NULL);

		xQueueSend( xQueue2,&recvd_num, 0UL );

		vTaskDelay( x100ms );

	}

}

static void DriveFan(void *pvParameters)
{
	int output;
	for(;;)
 	{
		xil_printf("PWM Task\n");

		xQueueReceive( 	xQueue2,&output, portMAX_DELAY );

		Xil_Out32(MY_OUTPUT, output*2);


 	}
}

