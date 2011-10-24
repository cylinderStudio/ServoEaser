//
// ServoEaser.cpp -- Arduino library for Servo easing
//
// Uses concepts from:
// -- http://portfolio.tobiastoft.dk/331886/Easing-library-for-Arduino
// -- http://jesusgollonet.com/processing/pennerEasing/
// -- http://robertpenner.com/easing/
// -- http://robertpenner.com/easing/easing_demo.html
//
// 2011, TeamPneumo, Tod E. Kurt, http://todbot.com/blog/
// 
//

#include "ServoEaser.h"
#include <math.h>

#define DEBUG 0

#if (DEBUG > 0 )
#define debug_update() {\
        Serial.print("update: currPos:"); Serial.print(currPos);        \
        Serial.print(" startPos:"); Serial.print(startPos);             \
        Serial.print(" changePos:"); Serial.print(changePos);           \
        Serial.print(" tick:"); Serial.print(tick);                     \
        Serial.print(" arrived:");Serial.println(((arrived)?"true":"false"));}
#define debug_getnextpos() {                                            \
        Serial.print("getNextPos: changePos:"); Serial.print(changePos);     \
        Serial.print(" moves[i].pos:"); Serial.print(moves[movesIndex].pos); \
        Serial.print(" durMillis:"); Serial.println(durMillis);}
#define debug_reset() {                                     \
        Serial.print("reset: currPos:"); Serial.print(currPos); \
        Serial.print(" changePos:"); Serial.println(changePos); }
#else
#define debug_update() {}
#define debug_getnextpos() {}
#define debug_reset() {}
#endif

// default easing function
// this is from Easing::easeInOutCubic()
// t: current time, b: beginning value, c: change in value, d: duration
// t and d can be in frames or seconds/milliseconds
float ServoEaser_easeInOutCubic(float t, float b, float c, float d)
{
    if ((t/=d/2) < 1) return c/2*t*t*t + b;
	return c/2*((t-=2)*t*t + 2) + b;
}

// set up an easer with a servo and a moves list
void ServoEaser::begin(Servo s, int frameTime, 
                       ServoMove* mlist, int mcount)
{
    begin( s, frameTime, servo.read() );
    play( mlist, mcount );
}

// set up an easer with just a servo and a starting position
void ServoEaser::begin(Servo s, int frameTime, int pos )
{
    servo = s;
    frameMillis = frameTime;
    startPos = pos;

    easingFunc = ServoEaser_easeInOutCubic;
    arrivedFunc = NULL;

    useMicros = false;

    reset();
}

// reset easer to initial conditions, does not nuke easingFunc or arrivedFunc
void ServoEaser::reset()
{
    currPos = servo.read();

    startPos = currPos;  // get everyone in sync
    changePos = 0; 

    movesIndex = 0;
    if( movesCount > 0 ) {
        changePos = moves[ movesIndex ].pos - startPos ;
        durMillis = moves[ movesIndex ].dur;
    }
    
    tickCount = durMillis / frameMillis;
    tick = 0;
    
    debug_reset();
}

//
void ServoEaser::play( ServoMove* mlist, int mcount)
{
    play( mlist, mcount, 0);
}
//
void ServoEaser::play( ServoMove* mlist, int mcount, int mreps )
{
    moves = mlist;
    movesCount = mcount;
    movesReps = mreps;

    running = true;

    reset();
}

// manual, non-moves list, control of easer position
void ServoEaser::easeTo( int pos, int dur )
{
    movesCount = 0;  // no longer doing moves list
    startPos = currPos;
    changePos = pos - startPos;
    durMillis = dur;
    tickCount = durMillis / frameMillis;
    tick = 0;
    arrived = false;
    running = true;
}

// used internally to select next servo position
void ServoEaser::getNextPos()
{
    arrived = true;
    if( arrivedFunc != NULL ) { arrivedFunc( currPos, movesIndex ); }

    if( movesCount==0 ) { // not in a moves list
        return;
    }

    movesIndex++;
    if( movesIndex == movesCount ) {
        movesIndex = 0;  // loop
        movesReps--;
        if( movesReps == 0 ) { // we are done
            stop();
        } else if( movesReps == -1 ) { // we are infinite
            movesReps = 0;
        }
    }
    startPos  = currPos; // current position becomes new start position

    changePos = moves[ movesIndex ].pos - startPos ;
    durMillis = moves[ movesIndex ].dur;

    debug_getnextpos();

    tickCount = durMillis / frameMillis;
    tick = 0;
    arrived = false;
}

// call this regularly in loop()
void ServoEaser::update()
{
    if( ((millis() - lastMillis) < frameMillis) || !running ) {  // time yet?
        return;
    }
    lastMillis = millis();

    currPos = easingFunc( tick, startPos, changePos, tickCount );
    
    debug_update();

    if( !arrived ) tick++;
    if( tick == tickCount ) { // time for new position
        getNextPos(); 
    }
    if( useMicros ) {
        servo.writeMicroseconds( angleToMicros(currPos) );
    } else {
        servo.write( currPos );
    }
}

//
void ServoEaser::setMinMaxMicroseconds(int mi, int ma)
{
    min = mi;
    max = ma;
}

// from Servo.cpp
#define SERVO_MIN() (MIN_PULSE_WIDTH - this->min * 4)  // min value in uS 
#define SERVO_MAX() (MAX_PULSE_WIDTH - this->max * 4)  // max value in uS

#define in_min 0
#define in_max 180
#define out_min SERVO_MIN()
#define out_max SERVO_MAX()
int ServoEaser::angleToMicros(float angle)
{
    //                 x   in_min,in_max, out_min, out_max
    //int value = map(value, 0, 180, SERVO_MIN(),  SERVO_MAX());
    return (angle-in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//
void ServoEaser::useMicroSeconds(boolean t)
{
    useMicros = t;
}
//
boolean ServoEaser::usingMicroSeconds()
{
    return useMicros;
}
//
void ServoEaser::start()
{
    running = true;
}
//
void ServoEaser::stop()
{
    running = false;
}
//
boolean ServoEaser::isRunning()
{ 
    return running;
}
//
boolean ServoEaser::hasArrived()
{ 
    return arrived;
}

// 
void ServoEaser::setEasingFunc( EasingFunc func )
{
    easingFunc = func;
}
//
void ServoEaser::setArrivedFunc( ArrivedFunc func ) 
{
    arrivedFunc = func;
}

//
/*
void ServoEaser::setMovesList( ServoMove* mlist, int mcount )
{
    moves = mlist;
    movesCount = mcount;

    reset();
}
*/

 

