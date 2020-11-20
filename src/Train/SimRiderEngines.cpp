/*
 * Copyright (c) 2020 Peter Kanatselis (pkanatselis@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma optimize("", off)
#include "SimRiderEngines.h"
#include <QSound>
#include <QDebug>

SimRiderEngines::SimRiderEngines(Context* context) : simRider(context) {
    engineInitialized = false;

    QString sSimRiderEnabled = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    if (sSimRiderEnabled == "1") {
        isSimRiderEnabled = true;
    }
    else {
        isSimRiderEnabled = false;
    }

    attackStatus = "NOT STARTED";
    workoutFinished = true;
    isAttacking = false;

    nextAttackAt = -1;
    attackCount = -1;
    vLat = -999.;
    vLon = -999.;
    vAlt = -999.;
    vDist = -999.;
    vWatts = -999.;
    engineType = 0;
    savedTimer = 0.;
    savedPower = 0.;

    //savedMaxSeparationLimit = 0; // meters
    isSimRiderWaiting = false;

    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(SimRiderStateUpdate(SimRiderStateData)), this, SLOT(SimRiderStateUpdate(SimRiderStateData)));
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
}

SimRiderEngines::~SimRiderEngines() {

}

void SimRiderEngines::stop() {
    engineInitialized = false;
    setSimRiderRouteDistance(0.);

}

void SimRiderEngines::start() {
    // Force SimRider to read engine settings 
    engineInitialized = false;

 }

void SimRiderEngines::telemetryUpdate(RealtimeData rtd) {

}

void SimRiderEngines::initEngines(Context* context) {

    QString sSimRiderEnabled = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    engineType = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "").toInt();
    QString simRiderTotalAttacks = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, "").toString();


    if (sSimRiderEnabled == "1" && simRiderTotalAttacks.toInt() > 0 && engineType >= 0) {
        isSimRiderEnabled = true;
    }
    else {
        isSimRiderEnabled = false;
    }

    if (context->currentErgFile() && !engineInitialized && isSimRiderEnabled) {
        // Initialize the selected SimRider engine
        switch (engineType) {
        case 0: // Intervals AI engine
            initializeIntervalsEngine(context->currentErgFile()->Duration,
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_WARMUPSTARTSPERCENT, "").toString(),
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_COOLDOWNSTARTSPERCENT, "").toString(),
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, "").toString(),
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKDURATION, "").toString(),
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKPOWERINCREASE, "").toString(),
                appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_MAXSEPARATION, "").toString()
            );
            break;
        case 1: //ERG file selected
            fileNameERG = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ERG_FILENAME, "").toString();
            if (!fileNameERG.isEmpty()) {
                int mode = 0;
                pErgFile = new ErgFile(fileNameERG, mode, context);
                engineInitialized = true;
            }
            break;
        case 2:
            // Previous ride
            fileNameGCRide = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, "").toString();
            if (!fileNameGCRide.isEmpty()) {
                QStringList errors_;
                QFile rideFile(fileNameGCRide);
                previousGCRideFile = RideFileFactory::instance().openRideFile(context, rideFile, errors_);
                if (previousGCRideFile != NULL) {
                    engineInitialized = true;
                }
                else {
                    engineInitialized = false;
                    qDebug() << "File selected for previous ride is invalid. Check chart settings";
                }
            }
            break;
        default: // Invalid selection 
            break;
        }

        isAttacking = false;
        workoutFinished = false;
        attackStatus = "Pacing";

    }
    else { // Engine not initialized, SimRider not enabled, no workout file selected
        qDebug() << "Virtual Partner Engine cannot initialize. Check chart settings";
    }
}

void SimRiderEngines::runEngines(Context* context, double displayWorkoutDistance, double curWatts, long total_msecs) {
    // Run the selected SimRider engine
    if (engineInitialized && isSimRiderEnabled )
    {
        if ((simRider.Distance() * 1000) > context->workout->Duration) {
            workoutFinished = true;
        }
        else {
            workoutFinished = false;
        }
        switch (engineType) {
        case 0: // Intervals AI engine
            if (context->currentErgFile() && (simRider.Distance() * 1000) <= context->workout->Duration) {
                double curDist = displayWorkoutDistance * 1000; // Use route distance to support skipping
                double distDiff = (simRider.Distance() * 1000) - curDist;
                if (!workoutFinished) {
                    simRider.Watts() = SimRiderIntervalsEngine(curWatts, curDist, distDiff, total_msecs);
                }
                else { // if simRider is done, set watts to 0
                    simRider.Watts() = SimRiderIntervalsEngine(0, curDist, distDiff, total_msecs);
                }
                simRider.UpdateSelf(context->currentErgFile());

                vLat = simRider.Latitude();
                vLon = simRider.Longitude();
                vAlt = simRider.Altitude();
                vDist = simRider.Distance();
                vWatts = simRider.Watts();
            }
            else {
                double curDist = displayWorkoutDistance * 1000; // Use route distance to support skipping
                double distDiff = (simRider.Distance() * 1000) - curDist;
                simRider.Watts() = SimRiderIntervalsEngine(0, curDist, distDiff, total_msecs);
                simRider.UpdateSelf(context->currentErgFile());

                vLat = simRider.Latitude();
                vLon = simRider.Longitude();
                vAlt = simRider.Altitude();
                vDist = simRider.Distance();
                vWatts = simRider.Watts();
                attackCount = -1;
            }
            break;
        case 1: // ERG file selected
            if (!fileNameERG.isEmpty()) {
                if (pErgFile->hasWatts()) {
                    int lap = 1;
                    //total_msecs = session_elapsed_msec + session_time.elapsed();
                    ErgFileQueryAdapter efa(pErgFile);
                    if (!workoutFinished) {
                        simRider.Watts() = efa.wattsAt(total_msecs, lap); // we don't care about lap...
                    }
                    else { // if simRider is done, set time to 0
                        simRider.Watts() = efa.wattsAt(0, lap); // we don't care about lap...
                    }
                    simRider.UpdateSelf(context->currentErgFile());

                    vLat = simRider.Latitude();
                    vLon = simRider.Longitude();
                    vAlt = simRider.Altitude();
                    vDist = simRider.Distance();
                    vWatts = simRider.Watts();

                    isAttacking = false;
                    attackStatus = "ERG mode";

                }
                else { // ERG file has no WATTS
                    qDebug() << "ERG file has no WATTS.";
                }
            }
            else {
                qDebug() << "No ERG file found.";
            }
            break;
        case 2:
            // Previous ride
            if (!fileNameGCRide.isEmpty() && fileNameGCRide != NULL) { // && previousGCRideFile->areDataPresent()->watts
                int currentSecond = (int)total_msecs / 1000;
                if (previousGCRideFile->areDataPresent()->watts) { // If we have watts we will run simulation
                    if (!workoutFinished) {
                        simRider.Watts() = previousGCRideFile->dataPoints()[currentSecond]->watts;
                    }
                    else { // if simRider is done, set watts to 0
                        simRider.Watts() = 0;
                    }
                    simRider.UpdateSelf(context->currentErgFile());
                    vWatts = simRider.Watts();
                    vDist = simRider.Distance();
                    vLat = simRider.Latitude();
                    vLon = simRider.Longitude();
                    vAlt = simRider.Altitude();
                }
                else { //update position every second
                    vLat = previousGCRideFile->dataPoints()[currentSecond]->lat;
                    vLon = previousGCRideFile->dataPoints()[currentSecond]->lon;
                    vAlt = previousGCRideFile->dataPoints()[currentSecond]->alt;
                    vDist = previousGCRideFile->dataPoints()[currentSecond]->km;
                    vWatts = 0;
                }

                isAttacking = false;
                attackStatus = "Previous ride mode";
            }
            else { // File is empty or it has no Watts
                qDebug() << "Selected file for previous ride is invalid or empty.";
                //return; 
            }
            break;
        default: // Invalid selection 
            break;
        }

    } else { // Engine not inititalized and/or SimRider not enabled
        qDebug() << "Virtual Partner Engine not inititalized and/or SimRider not enabled";
    }
    updateSimRiderData();
    context->notifySimRiderStateUpdate(srData);
}

void SimRiderEngines::updateSimRiderData() {

    srData.setSimRiderIsEngineInitialized(engineInitialized);         //engineInitialized = false;
    srData.setSimRiderIsEnabled(isSimRiderEnabled);             //isSimRiderEnabled = false;
    srData.setSimRiderAttackStatus(attackStatus);           //attackStatus = "NOT STARTED";
    srData.setSimRiderIsWorkoutFinished(workoutFinished);   //workoutFinished = true;
    srData.setSimRiderIsAttacking(isAttacking);   //isAttacking = false;

    srData.setSimRiderNextAttack(nextAttackAt); //nextAttackAt = -1;
    srData.setSimRiderAttackCount(attackCount); //attackCount = -1;
    srData.setSimRiderLat(vLat); //vLat = -999.;
    srData.setSimRiderLon(vLon); //vLon = -999.;
    srData.setSimRiderAlt(vAlt); //vAlt = -999.;
    srData.setSimRiderDist(vDist); //vDist = -999.;
    srData.setSimRiderWatts(vWatts); //vWatts = -999.;

    srData.setSimRiderEngineType(engineType); 
}


// Simple intervals engine for Simulated Riders.
// Pace for the first x% of the ride.
// Attack (add y watts) to the SimRider from the current user power for distance for z meters.
double SimRiderEngines::SimRiderIntervalsEngine(double inPower, double currentDistance, double distDiff, long total_msecs) {

    if (savedPower == 0.) savedPower = inPower;
    
    if ((total_msecs - savedTimer) < 2000 && !isAttacking) {
        return savedPower;
    }
    else {
        savedTimer = total_msecs;
        savedPower = inPower;
    }
    
    
    double outPower = 0.;

    // if the user skip ahead or back we need to re-calculate our next attack point
    if (abs(currentDistance - prevRouteDistance) > 50 ) {
        reCalculateAttackPoints(currentDistance);
    }
    // Shadow the user during warmup and cooldown
    if (currentDistance < warmUpEnds || currentDistance > coolDownStarts) {
        outPower = inPower;
        attackStatus = "Warmup - Cooldown";
    }
    // Not in warm up or cool down
    else {
        if (attackCount <= attackTotal && attackCount >= 0) {
            if (isAttacking) { //We are currently attacking
                if (currentDistance > (currentAttackAt + attackDuration)) { // If attack is finished reset power.
                    isAttacking = false;
                    outPower = inPower;  //TODO -- decelerate to inPower
                    QSound::play(":audio/lap.wav");
                }
                else { // Maintain attack power or wait for the athlete to catch up
                    outPower = setIntervalsAIAttackFields(distDiff, inPower);
                }
            }
            else {  // Not attacking, pace the user until next attack point or initiate an attack
                if (currentDistance < nextAttackAt) { // Waiting for next attack point
                    outPower = setIntervalsAIPacingFields(distDiff, inPower);
                }
                else { // Intiate the attack
                    initNewAtttackForIntervalsAI(inPower);
                    outPower = setIntervalsAIAttackFields(distDiff, inPower);
                    QSound::play(":audio/lap.wav");
                }
            }
        }
        else { //no more attacks 
            outPower = inPower; 
            attackCount = -1;
            attackStatus = "Pacing";
        }
    }
    prevRouteDistance = currentDistance; // Save state so we'll know if the user jumped
    return outPower;
}

//Initiate new attack for Intervals AI
void SimRiderEngines::initNewAtttackForIntervalsAI(double inPower) {
    currentAttackAt = nextAttackAt;
    initializeIntervalsNextAttack();
    attackPower = inPower + attackPowerIncrease;
    isAttacking = true;
}

// Calculate outPower while pacing based on inPower, and distance between VP and the athlete
double SimRiderEngines::setIntervalsAIPacingFields(double distDiff, double inPower) {
    double outPower = 0.;

    setMaxSeparationLimit(distDiff);
    // Check max separation value before increasing the power of the VP
    if (abs(distDiff) < maxSeparationLimit || maxSeparationLimit == 0) {
        isAttacking = false;
        attackStatus = "Pacing";
        outPower = inPower;
    }
    else {

        if (distDiff > 0) {
            outPower = inPower * 0.8;
            attackStatus = "Slowing down."; // Slow down so the athlete can catch up
        }
        else {
            outPower = inPower * 1.2;
            attackStatus = "Speeding up."; // Speed up to catch up
        }


    }
    return outPower;
}

void SimRiderEngines::setMaxSeparationLimit(double distDiff) {

    // Only reset if the max separation is not disabled by the user
    if (maxSeparationLimit != 0)
    {
        // If  SimRider gets too far ahead, reset the limit to 1 so we can catch up.
        if (abs(distDiff) > maxSeparationLimit) {
            isSimRiderWaiting = true;
        }
        else {
            isSimRiderWaiting = false;
        }


        if (!isSimRiderWaiting) {
            maxSeparationLimit = savedMaxSeparationLimit;
        }
        else {
            maxSeparationLimit = 1;
        }
    }
}

// Calculate outPower during an attack based on inPower, and distance between VP and the athlete
double SimRiderEngines::setIntervalsAIAttackFields(double distDiff, double inPower) {
    double outPower = 0.;

    setMaxSeparationLimit(distDiff);
    // Check max separation value before increasing the power of the VP
    if (abs(distDiff) < maxSeparationLimit || maxSeparationLimit == 0) {
        attackStatus = "Attacking";
        outPower = attackPower;
    }
    else {

        if (distDiff > 0) {
            outPower = inPower * 0.8;
            attackStatus = "Slowing down."; // Slow down so the athlete can catch up
        }
        else {
            outPower = inPower * 1.2;
            attackStatus = "Speeding up."; // Speed up to catch up
        }


    }
    return outPower;
}

// Find position in the route to set current and next attack points
void SimRiderEngines::reCalculateAttackPoints(double currentRouteDistance) {
    int x = 0;
    while (x < attackLocationList.size()) {
        if (attackLocationList.at(x) > currentRouteDistance) {
            
            if (x == 0) {
                nextAttackAt = attackLocationList.at(x);
                currentAttackAt = attackLocationList.at(x);
            }
            else {
                if (x < attackLocationList.size() - 1) {
                    currentAttackAt = attackLocationList.at(x - 1);
                    nextAttackAt = attackLocationList.at(x);
                }
                else {
                    nextAttackAt = attackLocationList.at(attackLocationList.size() - 1);
                    currentAttackAt = attackLocationList.at(attackLocationList.size() - 1);
                }
            }
            break;
        }
        x++;
    }

    //We are at the end of the route and can't jump more
    if (x == attackLocationList.size()) {
        attackCount = -1;
    }
    else {
        attackCount = x;
    }
}

void SimRiderEngines::initializeIntervalsNextAttack() {
    attackCount += 1;
    if (attackCount < attackLocationList.size()) {
        nextAttackAt = attackLocationList.at(attackCount);
    }
}

// Initialize values for a new ride. Some of these values need to be moved to the settings file
void SimRiderEngines::initializeIntervalsEngine(double rideDuration, 
    QString warmup, 
    QString cooldown, 
    QString attacks, 
    QString duration, 
    QString increase, 
    QString maxSeparation) {

    //QString sValue = "10.5";
    //QVariant val = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() / FEET_PER_METER));
    //int i = QString::number(val.toInt());
    //double d = QString::number(val.toDouble());


    attackCount = 0; //Initialize count for attack point calculations
    
    if (warmup == "" || warmup == "0" ) {
        warmUpEnds = 0;
    }
    else {
        warmUpEnds = rideDuration * (warmup.toDouble() / 100);
        if (warmUpEnds < 0) warmUpEnds = 0;
    }

    if (cooldown == "" || cooldown == "0") {
        coolDownStarts = rideDuration;
    }
    else {
        coolDownStarts = rideDuration - (rideDuration * (cooldown.toDouble() /100));
        if (coolDownStarts < 0) warmUpEnds = rideDuration;
    }

    if (attacks == "" || attacks == "0") {
        attackTotal = 0;
    }
    else {
        attackTotal = attacks.toInt();
        if (attackTotal < 0) attackTotal = 0;
    }

    if (attackTotal <= 0) {
        pacingInterval = 0;
    }
    else {
        double pctWarmUp = warmup.toDouble() / 100;
        double pctCoolDown = cooldown.toDouble() / 100;
        pacingInterval = (rideDuration * (1 - (pctWarmUp + pctCoolDown))) / attackTotal;
    }
    
    if (duration == "" || duration == "0") {
        attackDuration = 0;
    }
    else {
        attackDuration = duration.toInt();
        if (attackDuration < 0) attackDuration = 0;
    }

    if (increase == "" || increase == "0") {
        attackPowerIncrease = 0;
    }
    else {
        attackPowerIncrease = increase.toInt();
        // Allow for negative increase. Future functionality, look at the SimRider wBal and add negative multipler to simulate fatigue for VP
    }

    maxSeparationLimit = maxSeparation.toInt();
    savedMaxSeparationLimit = maxSeparationLimit;

    buildAttackPointsList(attackTotal, rideDuration);
    currentAttackAt = attackLocationList.at(0);
    nextAttackAt = currentAttackAt;
    isAttacking = false;
    engineInitialized = true;
    prevRouteDistance = 0; // Save state so we'll know if the user jumped

    if (attackDuration >= (pacingInterval * 0.9)) {
        QString s = "Attack duration is set to " + QString::number(attackDuration) +" but it should be less than " +QString::number((pacingInterval * 0.9)) + ". " +
            "Virtual partner engine will not initialize!";

        engineInitialized = false;
        qDebug() << "Virtual Partner: " + s;
    }
}

int SimRiderEngines::getNextAttack() 
{
    return nextAttackAt;
}

void SimRiderEngines::buildAttackPointsList(int numOfAttacks, double rideDuration)
{
    int ctr = 0;
    attackLocationList.clear();
    while (ctr < numOfAttacks) {
        srand((unsigned int)time(NULL));
        if (ctr == 0) {
            // Random first attack point after warmup ends and during the attack eligable part of the ride
            currentAttackAt = warmUpEnds + (pacingInterval / 2) + rand() % (int)(pacingInterval / numOfAttacks);
            nextAttackAt = currentAttackAt;
            attackLocationList.push_back(nextAttackAt);
        }
        else {
            //srand((unsigned int)time(NULL));
            nextAttackAt = warmUpEnds + (pacingInterval / 2) + (ctr * pacingInterval) + rand() % (int)(pacingInterval / ctr);
            attackLocationList.push_back(nextAttackAt);
        }
        ctr++;
    }
    // Last attack cannot end after the ride duration
    if ( (attackLocationList.at(ctr -1) + attackDuration) >= rideDuration) {
        attackLocationList.pop_back();
        attackLocationList.push_back(rideDuration - attackDuration);
    }
}

void SimRiderEngines::SimRiderStateUpdate(SimRiderStateData srData) {



    //srData.setSimRiderIsEngineInitialized(engineInitialized);         //engineInitialized = false;
    //srData.setSimRiderIsEnabled(isSimRiderEnabled);             //isSimRiderEnabled = false;
    //srData.setSimRiderAttackStatus(attackStatus);           //attackStatus = "NOT STARTED";
    //srData.setSimRiderIsWorkoutFinished(workoutFinished);   //workoutFinished = true;
    //srData.setSimRiderIsAttacking(isAttacking);   //isAttacking = false;

    //srData.setSimRiderNextAttack(nextAttackAt); //nextAttackAt = -1;
    //srData.setSimRiderAttackCount(attackCount); //attackCount = -1;
    //srData.setSimRiderLat(vLat); //vLat = -999.;
    //srData.setSimRiderLon(vLon); //vLon = -999.;
    //srData.setSimRiderAlt(vAlt); //vAlt = -999.;
    //srData.setSimRiderDist(vDist); //vDist = -999.;
    //srData.setSimRiderWatts(vWatts); //vWatts = -999.;

}

double SimRiderEngines::getLocationAtIndex(int index, int limit)
{
    if (index >= 0 && index < limit)
        return attackLocationList.at(index);
    else
        return -1;
}

void SimRiderEngines::setIntAIattackCount(int i)
{
    this->attackCount = i;
}

int SimRiderEngines::getIntAIattackCount()
{
    return attackCount;
}


void SimRiderEngines::setEngineType(int i)
{
    this->engineType = i;
}

int SimRiderEngines::getEngineType()
{
    return engineType;
}

void SimRiderEngines::setEngineInitialized(bool b)
{
    this->engineInitialized = b;
}

bool SimRiderEngines::getEngineInitialized()
{
    return engineInitialized;
}

void SimRiderEngines::setIsAttacking(bool b)
{
    this->isAttacking = b;
}

bool SimRiderEngines::getIsAttacking()
{
    return isAttacking;
}

void SimRiderEngines::setFileNameERG(QString s)
{
    this->fileNameERG = s;
}

QString SimRiderEngines::getFileNameERG()
{
    return fileNameERG;
}

void SimRiderEngines::setFileNameGCRide(QString s)
{
    this->fileNameGCRide = s;
}

QString SimRiderEngines::getFileNameGCRide()
{
    return fileNameGCRide;
}

void SimRiderEngines::setAttackStatus(QString s)
{
    this->attackStatus = s;
}

QString SimRiderEngines::getAttackStatus()
{
    return attackStatus;
}

void SimRiderEngines::setWorkoutFinished(bool b)
{
    this->workoutFinished = b;
}

bool SimRiderEngines::getWorkoutFinished()
{
    return workoutFinished;
}

void SimRiderEngines::setSimRiderRouteDistance(double d)
{
    //this->simRiderRouteDistance = b;
    simRider.Distance() = d;
}

double SimRiderEngines::getSimRiderRouteDistance()
{
    //return simRiderRouteDistance;
    return simRider.Distance();
}