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

#ifndef _GC_SimulatedRidersNestStats_h
#define _GC_SimulatedRidersNestStats_h
#include "GoldenCheetah.h"

#include <QWidget>
#include <QDialog>

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include "RideFile.h"
#include "IntervalItem.h"
#include "Context.h"
#include "TrainSidebar.h"
#include "BicycleSim.h"
#include <QDialog>
#include <QSslSocket>
#include <QGroupBox>

class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class LSimulatedRidersNestStats;
class IntervalSummaryWindow;
class SmallPlot;

class SimulatedRidersNestStats : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        SimulatedRidersNestStats(Context *);
        void createIntervalsAIsection();
        void createPrevRidesection();
        void createERGsection();
        ~SimulatedRidersNestStats();
        bool markerIsVisible;
        double plotLon = 0;
        double plotLat = 0;
        QString currentPage;

        //////RiderNest<4> riderNest; // nest of 4 riders
        RiderNest riderNest;
        std::vector<double> riderDeltas;
        SimulatedRidersNestStats();  // default ctor

    public slots:
        void configChanged(qint32);

    private:
        Context *context;
        QVBoxLayout *initialVBoxlayout;
        QString attackStatus;

        // setting dialog
        QCheckBox *enableVPChkBox;
        QLineEdit *qleIntAITotalAttacks, *qleIntAIAttackDuration, *qleIntAIWarmUpDuration, *qleIntAICoolDownDuration, *qleIntAIAttackPowerIncrease, *qleIntAIMaxSeparation;
        QLabel *IntAITotalAttacksLabel, *IntAIAttackDurationLabel, *WarmUpDurationLabel, *CoolDownDurationLabel, *AttackPowerIncreaseLabel, *IntAIMaxSeparationLabel;

        // reveal controls
        QPushButton *rButton;
        QPushButton *applyButton, *selectERGFileButton, *selectRideFileButton;

        QString workoutERGFileName, gcRideFileName;
        QLabel *header;
        QLabel* position2Name, * position2Status, * position2Position, *position2Watts;
        QLabel *position1Name, *position1Status, *position1Position, *position1Watts;
        QLabel *position0Name, *position0Status, *position0Position, *position0Watts; // Current Athlete
        QRadioButton* radioIntervalsAI, * radioERGworkout, *radioPreviousRide;
        QLabel* fileNameLabel, *selectERGFileLabel, *selectRideFileLabel;
        QGroupBox *VPenginesGrpBox, *intervalsAIGrpBox, * selectPrevRideGrpBox;
        QGridLayout *settingsGridLayout;
        double distDiff, prevRouteDistance, prevVP1Distance;
        QLabel* routeNameLbl, *routeDistanceLbl, *routeAvgSpeedLbl;
        QWidget *settingsWidget;

        SimRiderStateData localSrData;
        //bool firstShow;

    private slots:
        void telemetryUpdate(RealtimeData rtd);
        void start();
        void setChartHeader();
        void saveEnginesOptions();
        void stop();
        void pause();
        void setDisplayData(RealtimeData rtd, int engineType);
        void setGridData(double vp1DisplayWatts);
        void intervalsAIClicked();
        void eRGWorkoutClicked();
        void previousRideClicked();
        void selectERGFileToOpen();
        void selectRideFileToOpen();
        void displayRouteStats();
        bool routeMatchesWorkout();
        void SimRiderStateUpdate(SimRiderStateData srData);

    protected:
 
};

// Attack Statuses
//#define ATTACKING           0x1        // Attacking
//#define PACING              0x2        // Pacing user
//#define WARMUP              0x3        // Warm up
//#define COOLDOWN            0x4        // Cool down

#endif
