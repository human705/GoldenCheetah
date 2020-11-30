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

#include "SimRiderStateData.h"
#include <QDebug>

//#include "SimRiderEngines.h"

SimRiderStateData::SimRiderStateData()
{
    // Initialize
    simRiderLat = 0.;
    simRiderLon = 0.; 
    simRiderAlt = 0.;
    simRiderWatts = 0.; 
    simRiderDist = 0.;
    simRiderNextAttackAt = -1;

    simRiderIsAttacking = false;
    simRiderIsEnabled = false;
    simRiderIsWorkoutFinished = true;
    simRiderIsEngineInitialized = false;

    simRiderAttackStatus = "NOT STARTED";
    simRiderAttackCount = -1;
}


//SimRider Data
void SimRiderStateData::setSimRiderLat(double x)
{
    this->simRiderLat = x;
}

void SimRiderStateData::setSimRiderLon(double y)
{
    this->simRiderLon = y;
}

void SimRiderStateData::setSimRiderAlt(double z)
{
    this->simRiderAlt = z;
}

void SimRiderStateData::setSimRiderDist(double d)
{
    this->simRiderDist = d;
}

QString SimRiderStateData::getSimRiderAttackStatus() const
{
    return simRiderAttackStatus;
}

bool SimRiderStateData::getSimRiderIsAttacking() const
{
    return simRiderIsAttacking;
}

double SimRiderStateData::getSimRiderNextAttack() const
{
    return simRiderNextAttackAt;
}

void SimRiderStateData::setSimRiderNextAttack(double d)
{
    this->simRiderNextAttackAt = d;
}

void SimRiderStateData::setSimRiderAttackStatus(QString s)
{
    this->simRiderAttackStatus = s;
}

void SimRiderStateData::setSimRiderIsEnabled(bool b)
{
    this->simRiderIsEnabled = b;
}

void SimRiderStateData::setSimRiderIsAttacking(bool b)
{
    this->simRiderIsAttacking = b;
}

double SimRiderStateData::getSimRiderDist() const
{
    return simRiderDist;
}

bool SimRiderStateData::getSimRiderIsEnabled() const
{
    return simRiderIsEnabled;
}

double SimRiderStateData::getSimRiderLat() const
{
    return simRiderLat;
}

double SimRiderStateData::getSimRiderLon() const
{
    return simRiderLon;
}

double SimRiderStateData::getSimRiderAlt() const
{
    return simRiderAlt;
}

void SimRiderStateData::setSimRiderWatts(double p)
{
    this->simRiderWatts = p;
}

double SimRiderStateData::getSimRiderWatts() const
{
    return simRiderWatts;
}

void SimRiderStateData::setSimRiderAttackCount(int i)
{
    this->simRiderAttackCount = i;
}

int SimRiderStateData::getSimRiderAttackCount() const
{
    return simRiderAttackCount;
}

void SimRiderStateData::setSimRiderIsWorkoutFinished(bool b)
{
    this->simRiderIsWorkoutFinished = b;
}

bool SimRiderStateData::getSimRiderIsWorkoutFinished() const
{
    return simRiderIsWorkoutFinished;
}

void SimRiderStateData::setSimRiderIsEngineInitialized(bool b)
{
    this->simRiderIsEngineInitialized = b;
}

bool SimRiderStateData::getSimRiderIsEngineInitialized() const
{
    return simRiderIsEngineInitialized;
}

void SimRiderStateData::setSimRiderErrorCode(int i)
{
    this->simRiderErrorCode = i;
}

int SimRiderStateData::getSimRiderErrorCode() const
{
    return simRiderErrorCode;
}

void SimRiderStateData::setSimRiderEngineType(int i)
{
    this->simRiderEngineType = i;
}

int SimRiderStateData::getSimRiderEngineType() const
{
    return simRiderEngineType;
}