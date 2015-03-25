#include "derivative.h" /* include peripheral declarations */
#include "TFC\TFC.h"

#define BORDER 			300
#define STDOFFSET 		5	//Offset in LineScanImage-Array - Cutting of the noise
#define STDLENGTH 		96	//Effective Duration of LineScanImage-Arrays
#define PIXELPERINCH 	7	
#define DISTANCE 		14 	//Distance between wheel to focal in INCH
#define MAXWHITE 		3	//
#define TTL				100
#define MINCURVESPEED	0.6
#define BRAKECOUNTERMAX 25
#define STRAIGHTCOUNTERINIT 70

float old_diff = 0;
float old_steer;
int border = BORDER;
float derivative=0,proportional=0,integral=0,integrald=0,rate=0,prevposition=0,control=0;
float speed = 0;


bool overStart = false;
uint16_t lastMode = 0; 
uint16_t ttl = TTL;
float angle = 0;
bool slowdown = false;
bool hill = false;
uint16_t startCounter=0;
uint16_t hillCounter = 0;
uint16_t hillBrakeCounter=0;
uint16_t brakeCounter = 0;
uint16_t straightCounter = 0;
uint16_t cameraImageCounter = 0;
uint16_t endOfRoundCounter = 0;
uint16_t brakeDelayCounter = 0;

bool startLeft = false;
bool startRight = false;
uint16_t straightCounterMax=0;

bool isBlack(uint16_t index)
{
	return LineScanImage0[index] < border;
}

bool isWhite(uint16_t index)
{
	return !isBlack(index);
}

bool isCentrical(uint16_t index)
{
	return (index > 40 && index < 88);
//	return (index > 30 && index < 98);
}

bool isHill()
{
	uint16_t range = 10;
	int16_t k;
	uint16_t sum = 0;
	//if(isCentrical(old_diff))
	//{
		for(k=0; k<range; k++)
		{
			if(isBlack(63-k))
			{
				sum++;
			}
			
			if(isBlack(64+k))
			{
				sum++;
			}
		}
	//}
	
	if(sum >= 8 && sum < 12)
	{
		TERMINAL_PRINTF("HILL");
		hillCounter++;
		return true;
	}
	hillCounter=0;
	return false;
}

bool isStart(uint16_t index)
{
	bool tmpLeft = false;
	bool tmpRight = false;
	
	if(isBlack(index-(uint16)(2.5*PIXELPERINCH)) && isWhite(index-PIXELPERINCH))
	{
		tmpLeft = true;
	}
	if(isBlack(index+(uint16)(2.5*PIXELPERINCH)) && isWhite(index+PIXELPERINCH))
	{
		tmpRight= true;
	}
	if(startLeft && tmpRight)
	{
		return true;
	}
	
	if(startRight && tmpLeft)
	{
		return true;
	}
	
	if(tmpRight && tmpLeft)
	{
		return true;
	}
	
	startLeft = tmpLeft;
	startRight = tmpRight;

	return false;
}

void checkModeChange(uint16_t currentMode)
{
	if(currentMode != lastMode)
	{
		lastMode = currentMode;
		overStart = false;
		ttl = TTL;
		hill = false;
	}
}

void disableMotor() 
{
	TFC_SetMotorPWM(0, 0);
	TFC_HBRIDGE_DISABLE;
}

void enableMotor() 
{
	TFC_HBRIDGE_ENABLE;
}

void resetImage()
{
	LineScanImageReady=0;
}

void resetTicker()
{
	TFC_Ticker[0] = 0;
}

bool checkImageReadiness()
{
	return LineScanImageReady==1;
}

float getInnerSpeed(float outerSpeed, float angle)
{
	if(angle < 20.2 && angle > -20.2)
		return outerSpeed;
	return ((0+cos(angle))*outerSpeed+0.25);
}

uint16_t findBlack_IT(uint16_t offset, uint16_t length)
{
	int i,j;
	uint16_t halfLength = (uint16_t)(length/2.0);
	for(i=0; i<5; i++)
	{
		for(j=0; j<pow(2, i+1); j=j+2)
		{
			if(old_diff<=0)
			{	
				if(isBlack(offset + (j+1)*halfLength))
					return offset + (j+1)*halfLength;
			}
			else
			{
				if(isBlack(offset + (pow(2, i+1)-j-1)*halfLength))
					return offset + (pow(2, i+1)-j-1)*halfLength;
			}
		}
		halfLength = halfLength / 2;
	}
	return 200;
}

uint16_t findBlack(uint16_t offset, uint16_t length, uint16_t ttl)
{
	if(ttl==0)
		return 0;
	uint16_t halfLength = (uint16_t)(length/2.0);
	if (isBlack(offset + halfLength) || isBlack(offset + halfLength -1))
		return offset+halfLength;


	uint16_t left = findBlack(offset, halfLength, ttl-1);
	uint16_t right = findBlack(offset + halfLength, halfLength, ttl-1);

	if(left>0)
		return left;
	else
		return right;
}

int IMPROVEDfindBlack()
{
	int i = 0, left, right;

	for(i = 0; i <= 48; i++)
	{
		left = 63+old_diff-i;
		if(left>=STDOFFSET && isBlack(left))
		{
			return left;
		}

		right = 63+old_diff+i;
		if(right<(STDOFFSET+STDLENGTH) && isBlack(right))
		{
			return right;
		}
	}

	return old_diff<-20? STDOFFSET : old_diff>20? STDOFFSET+STDLENGTH-1 : 63;
}

void streamCurrentImageDataInPuttyFormat()
{
	if(cameraImageCounter % 25 == 0)
	{
		int i;
		for(i=0;i<128;i++)
		{
			int image = (int)LineScanImage0[i];

			if(image < border)
				TERMINAL_PRINTF("I", LineScanImage0[i]);
			else
				TERMINAL_PRINTF("_", LineScanImage0[i]);
			if(i==62 || i==64)
				TERMINAL_PRINTF(" ");
			if(i==STDOFFSET || i==STDOFFSET+STDLENGTH+1)
				TERMINAL_PRINTF("   ");
			if(i==127)
				TERMINAL_PRINTF("\r\n\n");
		}
		cameraImageCounter = 0;
	}
}

void print()
{

	LineScanImageReady=0;
	TERMINAL_PRINTF("\r\n");
	TERMINAL_PRINTF("L:");
	uint32_t i=0;
	for(i=0;i<128;i++)
	{
		TERMINAL_PRINTF("%i,", (int)angle);
		TERMINAL_PRINTF("\r\n");
	}

	for(i=0;i<128;i++)
	{
		//TERMINAL_PRINTF("%X", LineScanImage0[i]);
		if(i==127)
			TERMINAL_PRINTF("\r\n", LineScanImage1[i]);
		else
			TERMINAL_PRINTF(",", LineScanImage1[i]);
	}
}

void calibrateLightSensor() 
{
	uint16_t min = 10000;
	uint16_t i;
	for (i = 10; i < 118; i++) 
	{
		if (LineScanImage0[i] < min) 
		{
			min = LineScanImage0[i];
		}
	}
	border = (uint16_t) (min * 1.45);
	TERMINAL_PRINTF("Min: %d ", min);
	TERMINAL_PRINTF("Border: %d \r\n", border);
}

void calibrateLightSensorAtPoint(uint16_t index)
{
	border = LineScanImage0[index]*1.2;
	TERMINAL_PRINTF("New Border: %d \r\n", border);
}

float calculateSpeed(float speed, float steer)
{
	float factor = (steer < 0 ? 1.0 + steer : 1.0 - steer);
	float newSpeed;
	if (speed > MINCURVESPEED)  
	{
		newSpeed = factor * (speed - MINCURVESPEED) + MINCURVESPEED;
	} 
	else
	{
		newSpeed = speed;
	}

	return newSpeed;
}

float calculateSpeed_II(float speed, float steer)
{
	float newSpeed;
	if(speed < 0.7) //MINCURVESPEED)
		return speed;
		
	
	if(steer > 1 || steer < -1)
	{		
		if(brakeCounter < BRAKECOUNTERMAX && straightCounter > straightCounterMax)
		{
			//TERMINAL_PRINTF("STRAIGHT: %i ", straightCounter);
			brakeCounter++;	
			newSpeed = 0.4;
			TFC_BAT_LED0_ON;
			TFC_BAT_LED1_ON;
			TFC_BAT_LED2_ON;
			TFC_BAT_LED3_ON;
		}
		else
		{
			newSpeed = MINCURVESPEED;	
			TFC_BAT_LED0_OFF;
			TFC_BAT_LED1_OFF;
			TFC_BAT_LED2_OFF;
			TFC_BAT_LED3_OFF;
		}
	
		if(brakeCounter >= BRAKECOUNTERMAX && straightCounter > straightCounterMax)
		{
			straightCounter = 0;
			TFC_BAT_LED0_OFF;
			TFC_BAT_LED1_OFF;
			TFC_BAT_LED2_OFF;
			TFC_BAT_LED3_OFF;
		}
	}
	else
	{
		newSpeed = speed;
		brakeCounter = 0;
		TFC_BAT_LED0_OFF;
		TFC_BAT_LED1_OFF;
		TFC_BAT_LED2_OFF;
		TFC_BAT_LED3_OFF;
	}
	
	if(steer < 0.6 && steer > -0.6) //0.5
	{
		straightCounter++;
	}
	
	if(((steer > 0.6 || steer < -0.6) && brakeCounter == 0) || speed == 0.0)
	{
		straightCounter=0;
	}
	
	if(straightCounter>(straightCounterMax*0.25))
			TFC_BAT_LED0_ON;
	if(straightCounter>(straightCounterMax*0.50))
			TFC_BAT_LED1_ON;
	if(straightCounter>(straightCounterMax*0.75))
			TFC_BAT_LED2_ON;
	if(straightCounter>straightCounterMax)
			TFC_BAT_LED3_ON;
	
	//TERMINAL_PRINTF("NewSpeed: %i\r\n",(int)(newSpeed*100));
	return newSpeed;
}

uint16_t getLineMid(uint16_t index)
{
	int i;
	uint16_t left = 0, right = 0;

	uint16_t length = 7;

	for(i=0; i<length; i++)
	{
		if((index-i) >= STDOFFSET && isWhite(index-i))
			left++;
		if((index+i) < STDOFFSET+STDLENGTH && isWhite(index+i))
			right++;
		
		if((left >= MAXWHITE && right >= MAXWHITE) || (index-i) < STDOFFSET || (index+i) >= (STDOFFSET+STDLENGTH))
			break;
	}

	return  (int16_t)(index - (float)(right-left)/2.0);
}

uint16_t getMiddleIndex()
{
	uint16_t startIndex = IMPROVEDfindBlack();//findBlack_IT(STDOFFSET, STDLENGTH);  
	//TERMINAL_PRINTF("Start: %i \t", startIndex);
	uint16_t middleIndex = getLineMid(startIndex);
	//TERMINAL_PRINTF("Middle: %i \n\r", middleIndex);
	return middleIndex;
}

float calculateSteering(uint16_t middleIndex)
{
	float kp=0,ki=0,kd=0;
	float steer;
	float diff;
	if (middleIndex == 200) // Event of an error for the case that everything is white. 
	{
		//diff = old_diff < 0 ? -48 : 48;
		old_diff = old_diff<0? -48 : 48;
		return old_diff<0? -1.0 : 1.0;
	} 
	else
	{
		diff = middleIndex - 63.5;
	}

	old_diff = (int16_t)diff;  //diff>0? (float)diff*32.0/26.0 : 
	//TERMINAL_PRINTF("diff: %i \t", (int)(diff*1000));
	//Calculate steering value
	float diffinch = (float) diff / (float) PIXELPERINCH;
	float rad = atan(diffinch / (float) DISTANCE);
	angle = (float) 180 / 3.14159265 * rad;

	//pid parameters
	if( angle > 15)
		{
			kp=0.027;
			kd=0.01;
			ki=0;
		}
	else if(angle<-15)
		{
			kp=0.0185;
	//		kd=0.0;
		//	ki=0;
		}
	else
		kp=0.016;
	
	//kp=angle>10?0.017:angle<-10?0.03:0.02;
	
		//-------------------------PID Algorithm-----------------------------------
		if(angle>5 || angle<-5)	
		{
			speed=0.5;
			//-----------------Proportional------------------------
			proportional = angle * kp;
			//-------------------Integral--------------------------
			integral += angle;
			integrald = integral * ki;
			//------------------Derivative-------------------------
			rate = -prevposition + angle;
			derivative = rate * kd;
			//--------------------Control--------------------------
			control = proportional+derivative+integrald;
			integral /= 1.3;
			//-----------------------------------------------------		
			steer=control;	
			//if(steer<0)
				//steer=steer-0.05;
			//--------------------PID Ends-------------------------
		}	     
		else
		{
			speed=0.6;
			steer=0;
		}   
		prevposition=angle;
		return steer<-1.0? -1.0 : steer>1.0? 1.0 : steer;

	
	/*	float steer = angle > 0 ? angle / 47.5 : angle / 55.5;
	
		if(angle>15 && angle<-18)
			steer=old_steer;
	
		old_steer=steer;
	//steer = angle > 55.5 ? 1 : angle < -70.5 ? -1 : steer;
	//float steer = angle > 25.5 ? 1 : angle < -25.5 ? -1 : angle / 45.5;
	//float steer = angle > 32 ? 1 : angle < -25.5 ? -1 : angle < 0 ? angle / 25.5 : angle / 32.0;
	//TERMINAL_PRINTF("steer: %i\r\n", (int)(steer*100));
	
	if(steer < 0.2 && steer > -0.2)
	{
		steer = steer/2;
		//steer = steer*steer*2.5;
	}
	return steer<-1.0? -1.0 : steer>1.0? 1.0 : steer;*/
}


void checkStart(middleIndex)
{
	if (isCentrical(middleIndex) && isStart(middleIndex))
	{
		overStart = true;
	}
//	if (isStart(middleIndex))
//	{
//		overStart = true;
//	}
}

void drive(float speed)
{
	//float border=0;
	
	uint16_t middleIndex = getMiddleIndex();
	//if(middleIndex >50 && middleIndex < 70)
		//calibrateLightSensorAtPoint(middleIndex);
	//checkStart(middleIndex);
	float steer = calculateSteering(middleIndex);
	
	//TFC_SetServo(0, steer * (steer < 0 ? -steer : steer));
	if(TFC_Ticker[1]>=20)
	{
		TFC_SetServo(0, steer);
		TFC_Ticker[1] = 0;
	}
	
	float newSpeed;
	
	if(startCounter)//<60)
	{
		startCounter++;
		newSpeed = 1.0;
	}
	else 
	{
		newSpeed = calculateSpeed_II(speed, steer);	
	}
	
	if (angle > 0)
	{
		TFC_SetMotorPWM(getInnerSpeed(newSpeed, angle), newSpeed);
	} 
	else
	{
		TFC_SetMotorPWM(newSpeed, getInnerSpeed(newSpeed, -angle));
	}
}

void driveWithHillRecognition(float speed)
{
	uint16_t middleIndex = getMiddleIndex();
	//if(middleIndex >50 && middleIndex < 70)
		//calibrateLightSensorAtPoint(middleIndex);
	checkStart(middleIndex);
	float steer = calculateSteering(middleIndex);
	
	//TFC_SetServo(0, steer * (steer < 0 ? -steer : steer));
	if(TFC_Ticker[1]>=20)
	{
		TFC_SetServo(0, steer);
		TFC_Ticker[1] = 0;
	}
	float newSpeed;
	if(isHill() && hillCounter > 4)
	{
		hill=true;
	}
	
	if(startCounter<20)
	{
		startCounter++;
		newSpeed = 1.0;
	}
	else if(hill && hillBrakeCounter < 10)
	{
		newSpeed = -1.0;
		hillBrakeCounter++;
	}
	else
	{
		newSpeed = calculateSpeed_II(speed, steer);
		hillBrakeCounter = 0;
		hill = false;
	}
	
	if (angle > 0)
	{
		TFC_SetMotorPWM(getInnerSpeed(newSpeed, angle), newSpeed);
	} 
	else
	{
		TFC_SetMotorPWM(newSpeed, getInnerSpeed(newSpeed, -angle));
	}
}


float setEndOfRound()
{
	float speed =0;
	if(brakeDelayCounter < 80)
	{
		brakeDelayCounter++;
	}
	else if(brakeDelayCounter >= 80 && endOfRoundCounter < 50)
	{
		speed = -1;
		endOfRoundCounter++;
	}
	else
	{
		speed = 0;
		disableMotor();	
	}
	return speed;
}

int threshold(void)
{
	float Result[128],mean,sum,sd,temp1,k=0.64,r=2000;
	int i;
	uint16_t thr;
	
	for(i=0;i<128;i++)
	{
		Result[i]=LineScanImage0Buffer[1][i];
		if(i>9 && i<120)
		mean+=Result[i]/110;
		//mean+=temp1;		
	}	
	sum=0;	
	for(i=10;i<120;i++)
	{
		temp1=mean-Result[i];
		sum+=temp1*temp1/110;
	}
	sd=sqrt(sum);
	thr=(uint16_t)(mean*(1+k*(sd/r-1)));
	return thr;
}

int main(void)
{
	
	int t = 0;
	TFC_Init();
	disableMotor();

	for(;;)
	{	   
		//TFC_Task must be called in your main loop.
		TFC_Task();

		//This Demo program will look at the middle 2 switch to select one of 4 demo modes.
		//Let's look at the middle 2 switches
		uint_t mode = TFC_GetDIP_Switch()&0x0F;
		
		//set current Poti value as speed, when Button0 is pressed
		if(TFC_PUSH_BUTTON_0_PRESSED)
		{
			speed = 0.6;
			startCounter=0;
			straightCounterMax = (int)(STRAIGHTCOUNTERINIT / speed);
			enableMotor();
			brakeCounter = 0;
			straightCounter = 0;
			cameraImageCounter = 0;
			endOfRoundCounter = 0;
			brakeDelayCounter = 0;
		}
		//set speed 0, when Button1 is pressed
		if(TFC_PUSH_BUTTON_1_PRESSED)
		{
			speed = 0;
			disableMotor();
		}
		checkModeChange(mode);
		switch(mode)
		{
		default:
			disableMotor();
			break;
		case 0 :
			disableMotor();
			if(LineScanImageReady==1)
			{	border=threshold();
				TFC_SetServo(0, 0); 
				TFC_Ticker[0] = 0;
				LineScanImageReady=0;	
				print();
			}
			break;

		case 1:
			if(TFC_Ticker[0] >= 10000)
			{
				enableMotor();
				t++;
				if(t%2 == 1)
				{
					TFC_SetServo(0, -1);
					TFC_SetMotorPWM(0.5, 0);
				}
				else
				{
					TFC_SetServo(0, 1);
					TFC_SetMotorPWM(0, 0.5);
				}
				resetTicker();
			}
			break;

		case 2 :
			disableMotor();
			if(TFC_Ticker[0] > 2000 && LineScanImageReady==1)
			{
				TFC_Ticker[0] = 0;
				LineScanImageReady = 0;
				calibrateLightSensor();
			}
			break;	
			
		case 3:
			disableMotor();
			if(TFC_Ticker[0]>=100 && LineScanImageReady==1)
			{
				TFC_Ticker[0] = 0;
				LineScanImageReady=0;
				print();
			}
			break;
			
		case 4:
			disableMotor();
			TFC_SetServo(0, TFC_ReadPot(0));
			if(TFC_Ticker[0] >= 10)
			{
				TERMINAL_PRINTF("%i \r\n", (int)(TFC_ReadPot(0)*1000));
				resetTicker();
			}
			break;
		case 5:
			if(LineScanImageReady==1)
			{
				uint16 i = 1;
				uint16 mid = getMiddleIndex();
				uint16 count = 1;
				for(i=1;i<=7;i++)
				{
					if(isBlack(mid-i))		//changed from + to -
					{
						count++;
					}
					if(isBlack(mid+i))
					{
						count++;
					}
				}
				switch (count) {
					case 3:
						TFC_BAT_LED0_ON;
						TFC_BAT_LED1_ON;
						TFC_BAT_LED2_OFF;
						TFC_BAT_LED3_OFF;
						break;
					case 4:
						TFC_BAT_LED0_OFF;
						TFC_BAT_LED1_OFF;
						TFC_BAT_LED2_ON;
						TFC_BAT_LED3_OFF;
						break;
					case 5:
						TFC_BAT_LED0_ON;
						TFC_BAT_LED1_OFF;
						TFC_BAT_LED2_ON;
						TFC_BAT_LED3_OFF;
						break;
					case 6:
						TFC_BAT_LED0_OFF;
						TFC_BAT_LED1_ON;
						TFC_BAT_LED2_ON;
						TFC_BAT_LED3_OFF;
						break;
					case 7:
						TFC_BAT_LED0_ON;
						TFC_BAT_LED1_ON;
						TFC_BAT_LED2_ON;
						TFC_BAT_LED3_OFF;
						break;
					case 8:
						TFC_BAT_LED0_OFF;
						TFC_BAT_LED1_OFF;
						TFC_BAT_LED2_OFF;
						TFC_BAT_LED3_ON;
						break;
					case 9:
						TFC_BAT_LED0_ON;
						TFC_BAT_LED1_OFF;
						TFC_BAT_LED2_OFF;
						TFC_BAT_LED3_ON;
						break;
					default:
						TFC_BAT_LED0_OFF;
						TFC_BAT_LED1_OFF;
						TFC_BAT_LED2_OFF;
						TFC_BAT_LED3_OFF;
						break;
				}
			}
			
			break;
		case 8:
			if(LineScanImageReady==1) //TFC_Ticker[0] >= 20 && 
			{
				border=threshold();
				if(overStart)
				{
					speed = setEndOfRound();
					TERMINAL_PRINTF("END_OF_ROUND!!!!!!!!!!!!!!!!\r\n");

						t++;
						if(t>4)
						{
							t=0;
						}			
						TFC_SetBatteryLED_Level(t);
				}
				drive(speed);
//				streamCurrentImageDataInPuttyFormat();
				TFC_Ticker[0] = 0;
				LineScanImageReady=0;
			}
			break;
		case 9:
			if(LineScanImageReady==1) //TFC_Ticker[0] >= 20 && 
			{
				border=threshold();
				drive(speed);
				//print();
				
				//streamCurrentImageDataInPuttyFormat();
				TFC_Ticker[0] = 0;
				LineScanImageReady=0;
			}
			break;
		case 10: // integrated hill detection
			if(LineScanImageReady==1) //TFC_Ticker[0] >= 20 && 
			{
				if(overStart)
				{
					speed = setEndOfRound();
					TERMINAL_PRINTF("END_OF_ROUND!!!!!!!!!!!!!!!!\r\n");

					t++;
					if(t>4)
					{
						t=0;
					}			
					TFC_SetBatteryLED_Level(t);
				}
				driveWithHillRecognition(speed);
//				streamCurrentImageDataInPuttyFormat();
				
				TFC_Ticker[0] = 0;
				LineScanImageReady=0;
			}
			break;
		case 14:
			TFC_SetMotorPWM(speed,-speed);
			TFC_SetServo(0,-1);
		}
	}

	return 0;
}
