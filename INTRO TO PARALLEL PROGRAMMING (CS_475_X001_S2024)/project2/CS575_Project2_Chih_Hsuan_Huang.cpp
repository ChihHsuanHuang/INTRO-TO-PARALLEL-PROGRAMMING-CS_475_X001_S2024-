//Chih Hsuan Huang
//ID: 934554197
//Email: huanchih@oregonstate.edu
//CS575/475

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

int	NowYear = 2024;		// 2024- 2029
int	NowMonth = 0;		// 0 - 11

float	NowPrecip ;		// inches of rain per month
float	NowTemp ;		// temperature this month
float	NowHeight = 5.;		// grain height in inches
int	NowNumDeer = 2;		// number of deer in the current population
int NowNumPredator = 2;

const float GRAIN_GROWS_PER_MONTH =	       12.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float AVG_Predator_PER_MONTH =		4.0;	// average
const float AMP_Predator_PER_MONTH =		2.0;	// plus or minus
const float RANDOM_Predator =			1.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;



unsigned int seed = 0;

omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

// specify how many threads will be in the barrier:
//	(also init's the Lock)

void
InitBarrier( int n )
{
        NumInThreadTeam = n;
        NumAtBarrier = 0;
	omp_init_lock( &Lock );
}


// have the calling thread wait here until all the other threads catch up:

void
WaitBarrier( )
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}

float
SQR( float x )
{
        return x*x;
}

float Ranf(unsigned int *seedp, float low, float high)
{
    float r = (float)rand_r(seedp); // 0 - RAND_MAX

    return (low + r * (high - low) / (float)RAND_MAX);
}

void Deer()
{
    while (NowYear < 2030)
    {
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)( NowHeight );
        if( nextNumDeer < carryingCapacity )
           nextNumDeer++;
        else
             if( nextNumDeer > carryingCapacity )
                    nextNumDeer--;


        
        WaitBarrier();
        NowNumDeer = nextNumDeer;
        if (NowNumDeer < 0){
            NowNumDeer = 0;
        }

        
        WaitBarrier();
        
        WaitBarrier();
    }
}


void Grain()
{
    while (NowYear < 2030)
    {
        float nextHeight = NowHeight;

        float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );

        float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );

        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if( nextHeight < 0. ) nextHeight = 0.;
        
        WaitBarrier();
        NowHeight = nextHeight;
 
 
        WaitBarrier();
        
        WaitBarrier();
    }
}


void Watcher()
{
    while (NowYear < 2030)
    {
       
        WaitBarrier();

        
        WaitBarrier();

          fprintf(stderr, "%d,  %f,  %d,  %f,  %f,  %d\n", NowMonth + 1, NowHeight * 2.54, NowNumDeer, (NowTemp - 32) * (5. / 9.), NowPrecip, NowNumPredator);


        NowMonth++;

        if (NowMonth > 11)
        {
            NowMonth = 0;
            NowYear++;
        }

       float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	

       float temp = AVG_TEMP - AMP_TEMP * cos( ang );
       NowTemp = temp + Ranf(&seed,  -RANDOM_TEMP, RANDOM_TEMP );

       float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
       NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP );
       if( NowPrecip < 0. )
      	NowPrecip = 0.;

        
        WaitBarrier();
    }
}


void Predator()
{
    while (NowYear < 2030)
    {   
        float nextNumPredator = AVG_Predator_PER_MONTH + Ranf(&seed, -AMP_Predator_PER_MONTH, AMP_Predator_PER_MONTH);
        
     
       if (nextNumPredator > 3)
        {
            NowNumDeer -= 1;
        }
        else
        {
            NowNumDeer += 1;
        }
    

        WaitBarrier();
        NowNumPredator = (int)nextNumPredator; 
        if(NowNumPredator <= 0)
        {
            NowNumPredator = 1; 
        }

        WaitBarrier();

        WaitBarrier();
    }
}


int main(int argc, char *argv[])
{
    fprintf(stderr, "Month,Height,NowNumDeer,Temp,Precip,NowNumPredator\n ");
    omp_set_num_threads(4);   
    InitBarrier(4);           
#pragma omp parallel sections
{
    #pragma omp section
    {
        Deer( );
    }

    #pragma omp section
    {
        Grain( );
    }

    #pragma omp section
    {
        Watcher( );
    }

    #pragma omp section
    {
        Predator( );    
    }
}       // implied barrier -- all functions must return in order
    // to allow any of them to get past here
    return 0;
}
