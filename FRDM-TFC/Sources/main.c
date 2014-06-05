#include "derivative.h" /* include peripheral declarations */
#include "TFC\TFC.h"
#include "Math.h"
float comp[128],thr,mean=0,sd=0,sum=0,temp1=0,flag=0,pos=0,spos=0,Result[128];
float p,ccen=85,cen=0;
int lw=0,l=0;
float difference=0;
float derivative=0,proportional=0,integral=0,integrald=0,rate=0,prevposition=0,control=0;

//pid parameters
float kp=0.27;
float kd=0.05;
float ki=0;

//sauvola parameters
float k=1.0;//1.7
float r=2200;//1700
//int c=0;

/*void print()
{
int i=0;
	if(TFC_Ticker[0]>100 && LineScanImageReady==1)						
	{
		TFC_Ticker[0] = 0;
		LineScanImageReady=0;
	//	TERMINAL_PRINTF("\r\n");
		TERMINAL_PRINTF("L:");										
		for(i=0;i<128;i++)							
		{									
			TERMINAL_PRINTF("%X,",LineScanImage0[i]);
			//TERMINAL_PRINTF("\r\n",LineScanImage1[i]);
		}
		for(i=0;i<128;i++)
		{
			 TERMINAL_PRINTF("%X",(int)pos*10);
			if(i==127)
				TERMINAL_PRINTF("\r\n",LineScanImage1[i]);
			else
				TERMINAL_PRINTF(",",LineScanImage1[i]);
		}																
	}	
}
*/
void Linecheck()
{ 
	lw=0;
	mean=0;
	int i=0,j=0;
	for(i=0;i<128;i++)
	{
		Result[i]=LineScanImage0Buffer[1][i];
		temp1=Result[i]/128;
		mean+=temp1;		
	}
sum=0;
	for(i=0;i<128;i++)
	{
		
		temp1=mean-Result[i];
		sum+=temp1*temp1/128;
	
	}
sd=sqrt(sum);
thr=mean*(1+k*(sd/r-1));

	for (i=0;i<128;i++)
	{
		if(Result[i]<thr)
		{
			comp[i]=1;
			lw=lw+1;;
		}
		else
		{
			comp[i]=0;
		}
	}
//print();
if(lw<25)
{
	
	for(i=0;i<128;i++)
	{
		if(comp[i]==1)
		{
			
			for(j=1;j<3;j++)                    ////assumed line width is of 10samples
			{
				if(comp[i+j]==0)
			    	{
					flag=1;
					break;	
			    	}	
			  		
			}	
			  	  
			if(flag==0)
			{	
				pos=(i+(lw/2));
			  	lw=0;
			  	break;
			}
			else if(flag==1)
			{
				pos=pos;
			}
				
			  	
		}
		flag=0;	
		}
}
else
{
	difference=prevposition;
}
				
}

void pid()
{
	 difference=ccen-pos;
		//-------------------------PID Algorithm-----------------------------------
		if(difference>5 || difference<-5)	
		{

			//-----------------Proportional------------------------
			proportional = difference * kp;
			//-------------------Integral--------------------------
			integral += difference;
			integrald = integral * ki;
			//------------------Derivative-------------------------
			rate = -prevposition + difference;
			derivative = rate * kd;
			//--------------------Control--------------------------
			control = proportional+derivative+integrald;
			integral /= 1.3;
			//-----------------------------------------------------		
				spos=cen-control;	
			//--------------------PID Ends-------------------------
	     }
	     
	     else
	     {
	     	spos=cen;
	     }
	     
	     prevposition=difference;
	    spos=spos/35; 
	 	
	    if(spos>1)
	 	{
	 	 spos=1; //right	
	 	}
	 	else if(spos<-1)
	 	{
	 	 spos=-1;	//left
	 	}
	 	TFC_SetServo(0,spos);     
}

void fwd(float x)
{
TFC_HBRIDGE_ENABLE;	
TFC_SetMotorPWM(-x,-x);
}

int main(void)
{
	//uint32_t t,i=0;
	
	TFC_Init();
	
	for(;;)
	{	   
		
		TFC_Task();
		Linecheck();
		pid();
		print();
		if(TFC_PUSH_BUTTON_0_PRESSED)
			start:
			while(1)
			{
				if(TFC_PUSH_BUTTON_1_PRESSED)
				{
					while(1)
						{
						fwd(0);
					//	TFC_HBRIDGE_DISABLE;
						//fwd(0);
						if(TFC_PUSH_BUTTON_0_PRESSED)
						{	
							goto start;
						}
						}
				}
				else
				{
					Linecheck();
					pid();
									
					if(difference<7 && difference>-7)
					{
						l++;
						if(l>9)
						{
						//TFC_SetServo(0,0);     
						kp=0.08;
						kd=0;
						ki=0;
						fwd(0.55);
						l=0;
						}
					}
					
					else
						{
						//TFC_SetServo(0,spos);     
						kp=0.27;
						kd=0.05;
						ki=0;
						fwd(0.40);
						l=0;
						}
						
				}
			}
/*		//TFC_Task must be called in your main loop.  This keeps certain processing happy (I.E. Serial port queue check)
			TFC_Task();

			//This Demo program will look at the middle 2 switch to select one of 4 demo modes.
			//Let's look at the middle 2 switches
			switch((TFC_GetDIP_Switch()>>1)&0x03)
			{
			default:
			case 0 :
				//Demo mode 0 just tests the switches and LED's
				if(TFC_PUSH_BUTTON_0_PRESSED)
					TFC_BAT_LED0_ON;
				else
					TFC_BAT_LED0_OFF;
				
				if(TFC_PUSH_BUTTON_1_PRESSED)
					TFC_BAT_LED3_ON;
				else
					TFC_BAT_LED3_OFF;
				
				
				if(TFC_GetDIP_Switch()&0x01)
					TFC_BAT_LED1_ON;
				else
					TFC_BAT_LED1_OFF;
				
				if(TFC_GetDIP_Switch()&0x08)
					TFC_BAT_LED2_ON;
				else
					TFC_BAT_LED2_OFF;
				
				break;
					
			case 1:
				
				//Demo mode 1 will just move the servos with the on-board potentiometers
				if(TFC_Ticker[0]>=20)
				{
					TFC_Ticker[0] = 0; //reset the Ticker
					//Every 20 mSeconds, update the Servos
					TFC_SetServo(0,TFC_ReadPot(0));
					TFC_SetServo(1,TFC_ReadPot(1));
				}
				//Let's put a pattern on the LEDs
				if(TFC_Ticker[1] >= 125)
				{
					TFC_Ticker[1] = 0;
					t++;
					if(t>4)
					{
						t=0;
					}			
					TFC_SetBatteryLED_Level(t);
				}
				
				TFC_SetMotorPWM(0,0); //Make sure motors are off
				TFC_HBRIDGE_DISABLE;
			

				break;
				
			case 2 :
				
				//Demo Mode 2 will use the Pots to make the motors move
				TFC_HBRIDGE_ENABLE;
				TFC_SetMotorPWM(TFC_ReadPot(0),TFC_ReadPot(1));
						
				//Let's put a pattern on the LEDs
				if(TFC_Ticker[1] >= 125)
					{
						TFC_Ticker[1] = 0;
							t++;
							if(t>4)
							{
								t=0;
							}			
						TFC_SetBatteryLED_Level(t);
					}
				break;
			
			case 3 :
			
				//Demo Mode 3 will be in Freescale Garage Mode.  It will beam data from the Camera to the 
				//Labview Application
				
		
				if(TFC_Ticker[0]>100 && LineScanImageReady==1)
					{
					 TFC_Ticker[0] = 0;
					 LineScanImageReady=0;
					 TERMINAL_PRINTF("\r\n");
					 TERMINAL_PRINTF("L:");
					 
					 	if(t==0)
					 		t=3;
					 	else
					 		t--;
					 	
						 TFC_SetBatteryLED_Level(t);
						
						 for(i=0;i<128;i++)
						 {
								 TERMINAL_PRINTF("%X,",LineScanImage0[i]);
						 }
						
						 for(i=0;i<128;i++)
						 {
								 TERMINAL_PRINTF("%X",LineScanImage1[i]);
								 if(i==127)
									 TERMINAL_PRINTF("\r\n",LineScanImage1[i]);
								 else
									 TERMINAL_PRINTF(",",LineScanImage1[i]);
						}										
							
					}
					


				break;
			}
	}
	
	return 0;*/
	}
}
