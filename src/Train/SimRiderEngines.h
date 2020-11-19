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

#include <QObject>
#include <QString>
#include "Context.h"
#include "Settings.h"
#include "Athlete.h"
#include "ErgFile.h"
#include "BicycleSim.h"
#include "Units.h"
#include "SimRiderStateData.h"
#include <QVector>

#ifndef _GC_SimRiderEngines_h
#define _GC_SimRiderEngines_h

class SimRiderEngines : public QObject {
    Q_OBJECT

public:
    SimRiderEngines(Context* context); // default ctor
    ~SimRiderEngines();
    void initEngines(Context* context);
    void runEngines(Context* context, double displayWorkoutDistance, double curWatts, long total_msecs);
    void updateSimRiderData();
    void initializeIntervalsEngine(double rideDuration, QString warmup, QString cooldown, 
        QString attacks, QString duration, QString increase, QString maxSeparation);
    void initializeIntervalsNextAttack();
    void reCalculateAttackPoints( double currentRouteDistance);
    int getNextAttack();

    void buildAttackPointsList(int numOfAttacks, double rideDuration);

    //Sim Rider Intervals Engine
    double SimRiderIntervalsEngine(double inPower, double currentDistance, double distDiff);
    void initNewAtttackForIntervalsAI(double inPower);
    double setIntervalsAIPacingFields(double distDiff, double inPower);
    void setMaxSeparationLimit(double distDiff);
    double setIntervalsAIAttackFields(double distDiff, double inPower);
    
    double getLocationAtIndex(int index, int limit);
    void setIntAIattackCount(int i);
    int getIntAIattackCount();
    void setEngineType(int i);
    int getEngineType();
    void setEngineInitialized(bool b);
    bool getEngineInitialized();
    void setIsAttacking(bool b);
    bool getIsAttacking();
    void setFileNameERG(QString s);
    QString getFileNameERG();
    void setFileNameGCRide(QString s);
    QString getFileNameGCRide();
    void setAttackStatus(QString s);
    QString getAttackStatus();
    void setWorkoutFinished(bool b);
    bool getWorkoutFinished();
    void setSimRiderRouteDistance(double d);
    double getSimRiderRouteDistance();


    //Set 
    void setLat(double lat) { this->vLat = lat; };
    void setLon(double lon) { this->vLon = lon; };
    void setAlt(double alt) { this->vAlt = alt; };
    void setDist(double dist) { this->vDist = dist; };
    void setWatts(double watts) { this->vWatts = watts; };

    //Get
    double getLat() { return vLat; };
    double getLon() { return vLon; };
    double getAlt() { return vAlt; };
    double getDist() { return vDist; };
    double getWatts() { return vWatts; };


private slots:
    void stop();
    void start();
    void telemetryUpdate(RealtimeData rtd);
    void SimRiderStateUpdate(SimRiderStateData srData);

private:

    Context* context;

    double warmupCooldownPct = 0.05;
    double rideAttackEligable = 0.9;
    double prevRouteDistance = -1.;

    double simRiderRouteDistance; 

    QString attackStatus, fileNameERG, fileNameGCRide;
    int coolDownStarts, warmUpEnds;
    int attackTotal, pacingInterval, maxSeparationLimit, savedMaxSeparationLimit;
    int nextAttackAt, attackPower, currentAttackAt, attackPowerIncrease;
    int attackDuration, engineType;
    QVector<double> attackLocationList;
    bool isAttacking, engineInitialized, isSimRiderEnabled, workoutFinished, isSimRiderWaiting;
    int attackCount;
    ErgFile* pErgFile;
    RideFile* previousGCRideFile;
    SimulatedRider simRider;
    SimRiderStateData srData;
    //ErgFile* curErgFile;
    //double workoutDuration;
    double vLat, vLon, vAlt, vDist, vWatts;
};
#endif
