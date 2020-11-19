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

#include <QString>

#ifndef _GC_SimRiderStateData_h
#define _GC_SimRiderStateData_h

class SimRiderStateData {

private:

    //SimRider Data
    double simRiderLat, simRiderLon, simRiderAlt, simRiderWatts, simRiderDist, simRiderNextAttackAt;
    bool simRiderIsAttacking, simRiderIsEnabled, simRiderIsWorkoutFinished, simRiderIsEngineInitialized;
    QString simRiderAttackStatus, simRiderErrorMsg;
    int simRiderAttackCount, simRiderErrorCode, simRiderEngineType;

public:
    SimRiderStateData();

    void setSimRiderLat(double x);
    double getSimRiderLat() const;

    void setSimRiderLon(double y);
    double getSimRiderLon() const;

    void setSimRiderAlt(double z);
    double getSimRiderAlt() const;

    void setSimRiderDist(double d);
    double getSimRiderDist() const;

    void setSimRiderWatts(double p);
    double getSimRiderWatts() const;

    void setSimRiderIsEnabled(bool b);
    bool getSimRiderIsEnabled() const;

    void setSimRiderAttackStatus(QString s);
    QString getSimRiderAttackStatus() const;

    void setSimRiderIsAttacking(bool b);
    bool getSimRiderIsAttacking() const;

    void setSimRiderNextAttack(double d);
    double getSimRiderNextAttack() const;
    
    void setSimRiderAttackCount(int i);
    int getSimRiderAttackCount() const;

    void setSimRiderIsWorkoutFinished(bool b);
    bool getSimRiderIsWorkoutFinished() const;

    void setSimRiderIsEngineInitialized(bool b);
    bool getSimRiderIsEngineInitialized() const;

    void setSimRiderErrorCode(int i);
    int getSimRiderErrorCode() const;

    void setSimRiderEngineType(int i);
    int getSimRiderEngineType() const;

};
#endif
