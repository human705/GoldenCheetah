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
#include "SimulatedRidersNestStats.h"
#include "BicycleSim.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "SmallPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "Library.h"
#include "ErgFile.h"

// overlay helper
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>
#include <QFileDialog>


// declared in main, we only want to use it to get QStyle
extern QApplication *application;

SimulatedRidersNestStats::SimulatedRidersNestStats(Context *context) : GcChartWindow(context), context(context), riderNest(context) //sr1(context), firstShow(true), 
{
    riderNest.resize(2); // Set intitial riders
    riderDeltas.resize(2);
    attackStatus = "";
    prevRouteDistance = -1.0;
    prevVP1Distance = -1.0;

    // Create chart settings widget
    //QWidget *settingsWidget = new QWidget(this);
    settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0, 0, 0, 0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    // Create the grid layout for all the settings on the page
    settingsGridLayout = new QGridLayout(settingsWidget);
    
    // CheckBox to globaly enable Virtual Partner and add it to the grid
    enableVPChkBox = new QCheckBox("Enable Virtual Partner");
    QString sValue = "";
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    if (sValue.isEmpty() || sValue == "0") {
        enableVPChkBox->setCheckState(Qt::Unchecked);
    }
    else {
        enableVPChkBox->setCheckState(Qt::Checked);
    }
    settingsGridLayout->addWidget(enableVPChkBox, 0, 0, 1, 2, Qt::AlignLeft);

    // Create a group box for all the engines and radio buttons for each engine
    VPenginesGrpBox = new QGroupBox(tr("Simulated Rider Engines:"));
    radioIntervalsAI = new QRadioButton(tr("&Intervals AI"));
    radioERGworkout = new QRadioButton(tr("&ERG workout"));
    radioPreviousRide = new QRadioButton(tr("&Previous Ride"));

    // Group all radio buttons in the VBox
    QVBoxLayout* VPEnginesVBox = new QVBoxLayout(settingsWidget);
    VPEnginesVBox->addWidget(radioIntervalsAI);
    VPEnginesVBox->addWidget(radioERGworkout);
    VPEnginesVBox->addWidget(radioPreviousRide);
    //Add VBox to GroupBox
    VPenginesGrpBox->setLayout(VPEnginesVBox);
    //Add group box to the settings grid
    settingsGridLayout->addWidget(VPenginesGrpBox, 1, 0, 1, 2, Qt::AlignLeft);

    // Connect radio buton click signals
    connect(radioIntervalsAI, SIGNAL(clicked(bool)), this, SLOT(intervalsAIClicked()));
    connect(radioERGworkout, SIGNAL(clicked(bool)), this, SLOT(eRGWorkoutClicked()));
    connect(radioPreviousRide, SIGNAL(clicked(bool)), this, SLOT(previousRideClicked()));

    createERGsection();

    createPrevRidesection();

    createIntervalsAIsection();

    // Add Save settings button at the bottom of the page
    rButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), "Save settings", this);
    settingsGridLayout->addWidget(rButton, 10, 0, 1, 3,  Qt::AlignHCenter);
    connect(rButton, SIGNAL(clicked(bool)), this, SLOT(saveEnginesOptions()));

    // Read SimRider engine type from file and set the form values
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "").toString();
    if (sValue == "" || sValue == "0") {
        radioIntervalsAI->setChecked(true);
        intervalsAIClicked();
    }
    else {
        if (sValue == "1") {
            radioERGworkout->setChecked(true);
            eRGWorkoutClicked();
        }
        else {
            if (sValue == "2") {
                radioPreviousRide->setChecked(true);
                previousRideClicked();
            }
        }
    }

    //Set widget as configuration settings
    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);

    //Create a VBox layout to show both widgets (settings and stats) when adding the chart
    initialVBoxlayout = new QVBoxLayout();
    initialVBoxlayout->setSpacing(0);
    initialVBoxlayout->setContentsMargins(2, 0, 2, 2);
    setChartLayout(initialVBoxlayout);

    // Conent signal to recieve updates build the chart.
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(pause()), this, SLOT(pause()));
    connect(context, SIGNAL(SimRiderStateUpdate(SimRiderStateData)), this, SLOT(SimRiderStateUpdate(SimRiderStateData)));

    //New widget to show rider statistics
    QWidget* nestStatsWidget = new QWidget(this);
    nestStatsWidget->setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));  //Set chart menu text color
    nestStatsWidget->setWindowTitle("Simulated Riders Nest Stats");

    header = new QLabel("Virtual Partner Simulation");
    header->setStyleSheet("QLabel { color : white; }");
    header->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    position0Name = new QLabel("Athlete Name");
    position0Name->setStyleSheet("QLabel { color : yellow; }");
    position0Name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    position0Status = new QLabel("AI status");
    position0Status->setStyleSheet("QLabel { color : yellow; }");
    position0Status->setAlignment(Qt::AlignCenter);

    position0Position = new QLabel("Distance");
    position0Position->setStyleSheet("QLabel { color : yellow; }");
    position0Position->setAlignment(Qt::AlignCenter);

    position0Watts = new QLabel("Power (Watts)");
    position0Watts->setStyleSheet("QLabel { color : yellow; }");
    position0Watts->setAlignment(Qt::AlignCenter);
    
    position1Name = new QLabel(context->athlete->cyclist);
    position1Name->setStyleSheet("QLabel { color : rgb(43, 130, 203) }");
    position1Name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    position1Status = new QLabel("");
    position1Status->setStyleSheet("QLabel { color : white; }");
    position1Status->setAlignment(Qt::AlignCenter);

    position1Position = new QLabel("");
    position1Position->setStyleSheet("QLabel { color : white; }");
    position1Position->setAlignment(Qt::AlignCenter);

    position1Watts = new QLabel("");
    position1Watts->setStyleSheet("QLabel { color : white; }");
    position1Watts->setAlignment(Qt::AlignCenter);

    position2Name = new QLabel("Virtual Partner");
    position2Name->setStyleSheet("QLabel { color : red; }");
    position2Name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    position2Status = new QLabel("Not started");
    position2Status->setStyleSheet("QLabel { color : white; }");
    position2Status->setAlignment(Qt::AlignCenter);

    position2Position = new QLabel("0.0");
    position2Position->setStyleSheet("QLabel { color : white; }");
    position2Position->setAlignment(Qt::AlignCenter);

    position2Watts = new QLabel("0");
    position2Watts->setStyleSheet("QLabel { color : white; }");
    position2Watts->setAlignment(Qt::AlignCenter);

    // Create Grid Layout to show statistics
    QGridLayout* nestGridLayout = new QGridLayout(nestStatsWidget);
    nestGridLayout->addWidget(header, 0, 0, 1, 4, Qt::AlignHCenter);

    nestGridLayout->addWidget(position0Name, 1, 0);
    nestGridLayout->addWidget(position0Position, 1, 1);
    nestGridLayout->addWidget(position0Status, 1, 2);
    nestGridLayout->addWidget(position0Watts, 1, 3);

    nestGridLayout->addWidget(position1Name, 2, 0);
    nestGridLayout->addWidget(position1Position, 2, 1);
    nestGridLayout->addWidget(position1Status, 2, 2);
    nestGridLayout->addWidget(position1Watts, 2, 3);

    nestGridLayout->addWidget(position2Name, 3, 0);
    nestGridLayout->addWidget(position2Position, 3, 1);
    nestGridLayout->addWidget(position2Status, 3, 2, Qt::AlignCenter);
    nestGridLayout->addWidget(position2Watts, 3, 3);

    // Add stats widget to initial VBox Layout
    initialVBoxlayout->addWidget(nestStatsWidget);
    configChanged(CONFIG_APPEARANCE);

    setChartHeader();
}

void SimulatedRidersNestStats::createIntervalsAIsection() {

    // Create a GroupBox for Intervals AI options
    intervalsAIGrpBox = new QGroupBox(tr("Simulated Rider Intervals AI Engine options:"));
    // Create a vbox layout for Intervals AI Engine options
    QVBoxLayout* intervalsAIOptionsVBox = new QVBoxLayout(settingsWidget);
    // Create grid layout for the engine options inside the settings grid
    QGridLayout* IntervalAIOptionsGridLayout = new QGridLayout(settingsWidget);
    //Add labels and values to the engine options grid
    QString sValue = "";
    IntAITotalAttacksLabel = new QLabel(tr("Total attacks during the ride by the AI: <br><font color=red><i>Zero disables Virtual Partner</i></font>"));
    qleIntAITotalAttacks = new QLineEdit(this);
    qleIntAITotalAttacks->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, "").toString();
    if (sValue.isEmpty()) qleIntAITotalAttacks->setText("8");
    else qleIntAITotalAttacks->setText(sValue);
    //Add label 1 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAITotalAttacksLabel, 0, 0, Qt::AlignLeft);
    //Add value 1 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAITotalAttacks, 0, 1, Qt::AlignLeft);

    sValue = "";
    WarmUpDurationLabel = new QLabel(tr("Warm Up %:"));
    qleIntAIWarmUpDuration = new QLineEdit(this);
    qleIntAIWarmUpDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_WARMUPSTARTSPERCENT, "").toString();
    if (sValue.isEmpty()) qleIntAIWarmUpDuration->setText("5"); //meters, need to convert between meters and feet
    else qleIntAIWarmUpDuration->setText(sValue);
    //Add label 2 to the options grid
    IntervalAIOptionsGridLayout->addWidget(WarmUpDurationLabel, 1, 0, Qt::AlignLeft);
    //Add value 2 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIWarmUpDuration, 1, 1, Qt::AlignLeft);

    sValue = "";
    CoolDownDurationLabel = new QLabel(tr("Cool down %:"));
    qleIntAICoolDownDuration = new QLineEdit(this);
    qleIntAICoolDownDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_COOLDOWNSTARTSPERCENT, "").toString();
    if (sValue.isEmpty()) qleIntAICoolDownDuration->setText("5"); //meters, need to convert between meters and feet
    else qleIntAICoolDownDuration->setText(sValue);
    //Add label 3 to the options grid
    IntervalAIOptionsGridLayout->addWidget(CoolDownDurationLabel, 2, 0, Qt::AlignLeft);
    //Add value 3 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAICoolDownDuration, 2, 1, Qt::AlignLeft);

    sValue = "";
    AttackPowerIncreaseLabel = new QLabel(tr("AI power increase during attack (WATTS):"));
    qleIntAIAttackPowerIncrease = new QLineEdit(this);
    qleIntAIAttackPowerIncrease->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKPOWERINCREASE, "").toString();
    if (sValue.isEmpty()) qleIntAIAttackPowerIncrease->setText("55"); //meters, need to convert between meters and feet
    else qleIntAIAttackPowerIncrease->setText(sValue);
    //Add label 4 to the options grid
    IntervalAIOptionsGridLayout->addWidget(AttackPowerIncreaseLabel, 3, 0, Qt::AlignLeft);
    //Add value 4 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIAttackPowerIncrease, 3, 1, Qt::AlignLeft);

    QString feetMetersLabel = ((GlobalContext::context()->useMetricUnits) ? "AI attack duration (meters):" : "AI attack duration (feet):");
    sValue = "";
    IntAIAttackDurationLabel = new QLabel(feetMetersLabel);
    qleIntAIAttackDuration = new QLineEdit(this);
    qleIntAIAttackDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKDURATION, "").toString();
    //We always write meters so we need to convert between meters and feet
    QVariant feetMeterValue = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() * FEET_PER_METER));
    if (sValue.isEmpty()) qleIntAIAttackDuration->setText("100");
    else qleIntAIAttackDuration->setText(QString::number(feetMeterValue.toInt()));
    //Add label 5 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAIAttackDurationLabel, 4, 0, Qt::AlignLeft);
    //Add value 5 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIAttackDuration, 4, 1, Qt::AlignLeft);

    QString feetMetersMaxSepLabel = ((GlobalContext::context()->useMetricUnits) ? "Maximum separation from Virtual Partner (meters):" : "Maximum separation from Virtual Partner (feet):");
    feetMetersMaxSepLabel += " <br><font color=red><i>If set to zero, Virtual Partner will not wait.</i></font>";
    sValue = "";
    IntAIMaxSeparationLabel = new QLabel(feetMetersMaxSepLabel);
    qleIntAIMaxSeparation = new QLineEdit(this);
    qleIntAIMaxSeparation->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_MAXSEPARATION, "").toString();
    //We always write meters so we need to convert between meters and feet
    feetMeterValue = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() * FEET_PER_METER));
    if (sValue.isEmpty()) qleIntAIMaxSeparation->setText("50");
    else qleIntAIMaxSeparation->setText(QString::number(feetMeterValue.toInt()));
    //Add label 5 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAIMaxSeparationLabel, 5, 0, Qt::AlignLeft);
    //Add value 5 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIMaxSeparation, 5, 1, Qt::AlignLeft);

    //Add Interval AI options groupbox  to the VBox
    intervalsAIOptionsVBox->addWidget(intervalsAIGrpBox);
    //Add Interval AI Options grid  layout to the group box
    intervalsAIGrpBox->setLayout(IntervalAIOptionsGridLayout);
    // Add Intervals AI options groupbox to the settings grid
    settingsGridLayout->addWidget(intervalsAIGrpBox, 2, 0, 1, 3, Qt::AlignLeft);
}

void SimulatedRidersNestStats::createPrevRidesection() {
    // Add select Ride file button 
    selectRideFileButton = new QPushButton(application->style()->standardIcon(QStyle::SP_DirIcon), "Select file", this);
    //settingsGridLayout->addWidget(selectRideFileButton, 1, 2, 1, 2, Qt::AlignHCenter);
    selectRideFileButton->hide();
    connect(selectRideFileButton, SIGNAL(clicked(bool)), this, SLOT(selectRideFileToOpen()));

    // Add file name label for previous ride file 
    selectRideFileLabel = new QLabel(tr(" Need JSON file"));
    //fileNameLabel->setWordWrap(true);
    selectRideFileLabel->setWordWrap(true);
    //fileNameLabel->setVisible(false);
    selectRideFileLabel->setVisible(false);
    QString sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, "").toString();
    if (sValue.isEmpty()) selectRideFileLabel->setText(" No file selected! ");
    else selectRideFileLabel->setText(sValue);

    // Create a group box for the previous ride options
    selectPrevRideGrpBox = new QGroupBox(tr("Select previous ride"));

    // Create grid layout 
    QGridLayout* prevRideOptionsGridLayout = new QGridLayout(settingsWidget);

    //Add labels and values to the grid
    prevRideOptionsGridLayout->addWidget(selectRideFileButton, 0, 0, 1, 2, Qt::AlignCenter);
    prevRideOptionsGridLayout->addWidget(selectRideFileLabel, 1, 0, 1, 2, Qt::AlignCenter);

    // Previous ride stats labels
    QLabel* l1 = new QLabel(tr("Route Name = "));
    //l1->setStyleSheet("QLabel{border: 2px solid black;}");
    prevRideOptionsGridLayout->addWidget(l1, 2, 0, 1, 1, Qt::AlignLeft);
    routeNameLbl = new QLabel(tr(""));
    prevRideOptionsGridLayout->addWidget(routeNameLbl, 2, 1, 1, 1, Qt::AlignLeft);

    QLabel* l2 = new QLabel(tr("Distance = "));
    //l2->setStyleSheet("QLabel{border: 2px solid black;}");
    prevRideOptionsGridLayout->addWidget(l2, 3, 0, 1, 1, Qt::AlignLeft);
    routeDistanceLbl = new QLabel(tr(" "));
    prevRideOptionsGridLayout->addWidget(routeDistanceLbl, 3, 1, 1, 1, Qt::AlignLeft);

    QLabel* l3 = new QLabel(tr("AVG Speed = "));
    //l3->setStyleSheet("QLabel{border: 2px solid black;}");
    prevRideOptionsGridLayout->addWidget(l3, 4, 0, 1, 1, Qt::AlignLeft);
    routeAvgSpeedLbl = new QLabel(tr(" "));
    prevRideOptionsGridLayout->addWidget(routeAvgSpeedLbl, 4, 1, 1, 1, Qt::AlignLeft);

    // Add grid to the GroupBox
    selectPrevRideGrpBox->setLayout(prevRideOptionsGridLayout);

    //Add group box to the settings grid
    settingsGridLayout->addWidget(selectPrevRideGrpBox, 1, 2, 1, 3, Qt::AlignLeft);

    // Show previous ride stats 
    if (!appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, "").toString().isEmpty()) {
        gcRideFileName = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, "").toString();
        displayRouteStats();
    }

}

void SimulatedRidersNestStats::createERGsection() {

    // Connect radio buton click signals
    connect(radioIntervalsAI, SIGNAL(clicked(bool)), this, SLOT(intervalsAIClicked()));
    connect(radioERGworkout, SIGNAL(clicked(bool)), this, SLOT(eRGWorkoutClicked()));
    connect(radioPreviousRide, SIGNAL(clicked(bool)), this, SLOT(previousRideClicked()));

    // Add select ERG file button 
    selectERGFileButton = new QPushButton(application->style()->standardIcon(QStyle::SP_DirIcon), "Select file", this);
    //settingsGridLayout->addWidget(selectERGFileButton, 1, 2, 1, 2, Qt::AlignHCenter);
    selectERGFileButton->hide();
    connect(selectERGFileButton, SIGNAL(clicked(bool)), this, SLOT(selectERGFileToOpen()));

    // Add file name label for ERG file 
    selectERGFileLabel = new QLabel(tr(" Need ERG file"));
    //fileNameLabel->setWordWrap(true);
    selectERGFileLabel->setWordWrap(true);
    //fileNameLabel->setVisible(false);
    selectERGFileLabel->setVisible(false);
    QString sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ERG_FILENAME, "").toString();
    if (sValue.isEmpty()) selectERGFileLabel->setText(" No file selected! ");
    else selectERGFileLabel->setText(sValue);
    settingsGridLayout->addWidget(selectERGFileButton, 1, 2, Qt::AlignCenter);
    settingsGridLayout->addWidget(selectERGFileLabel, 1, 3, Qt::AlignLeft);
}

SimulatedRidersNestStats::~SimulatedRidersNestStats()
{
}

void SimulatedRidersNestStats::intervalsAIClicked() {
    intervalsAIGrpBox->show();
    selectERGFileLabel->hide();
    selectERGFileButton->hide();
    selectRideFileButton->hide();
    selectRideFileLabel->hide();
    selectPrevRideGrpBox->hide();
}

void SimulatedRidersNestStats::eRGWorkoutClicked() {
    intervalsAIGrpBox->hide();
    selectERGFileLabel->show();
    selectERGFileButton->show();
    selectRideFileButton->hide();
    selectRideFileLabel->hide();
    selectPrevRideGrpBox->hide();
}

void SimulatedRidersNestStats::previousRideClicked() {
    intervalsAIGrpBox->hide();
    selectERGFileLabel->hide();
    selectERGFileButton->hide();
    selectRideFileButton->show();
    selectRideFileLabel->show();
    selectPrevRideGrpBox->show();

}

void SimulatedRidersNestStats::selectERGFileToOpen() {
    QString savedFileName = selectERGFileLabel->text();
    QString path = context->athlete->home->workouts().canonicalPath() + "/";
    workoutERGFileName = QFileDialog::getOpenFileName(this, "Select ERG file", path, tr("workout (*.erg)"));
    //QMessageBox::information(0, QString(".."),
    //    QString("Selected file: " + workoutERGFileName),
    //    QMessageBox::Ok);
    if (workoutERGFileName != NULL) {
        selectERGFileLabel->setText(workoutERGFileName);
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ERG_FILENAME, workoutERGFileName);
    }
    else {
        selectERGFileLabel->setText(savedFileName);
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ERG_FILENAME, savedFileName);
    }

}

void SimulatedRidersNestStats::selectRideFileToOpen() {
    QString savedFileName = selectRideFileLabel->text();
    QString path = context->athlete->home->activities().canonicalPath() + "/";
    gcRideFileName = QFileDialog::getOpenFileName(this, "Select JSON file", path, tr("GC Ride (*.json)"));
    //QMessageBox::information(0, QString(".."),
    //    QString("Selected file: " + gcRideFileName),
    //    QMessageBox::Ok);
    if (gcRideFileName != NULL) {
        selectRideFileLabel->setText(gcRideFileName);
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, gcRideFileName);
        displayRouteStats();
    }
    else {
        selectRideFileLabel->setText(savedFileName);
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, savedFileName);
    }
}

void SimulatedRidersNestStats::displayRouteStats() {
    // Display file information for user validation
    QStringList errors_;
    RideFile* checkRideFile;
    QFile rideFile(gcRideFileName);
    checkRideFile = RideFileFactory::instance().openRideFile(context, rideFile, errors_);

    if (checkRideFile != NULL)
    {
        QString n = checkRideFile->getTag("Route", "");
        routeNameLbl->setText(n);

        int iCount = checkRideFile->dataPoints().count();
        double routeDistance = 0;
        double startLat = 0, startLon = 0, startAlt = 0, startSecs = -1;
        double endLat = 0, endLon = 0, endAlt = 0, endSecs = 0;
        if (iCount > 0) {
            const RideFilePoint* pStartPoint = checkRideFile->dataPoints()[0];
            startLat = pStartPoint->lat;
            startLon = pStartPoint->lon;
            startAlt = pStartPoint->alt;
            startSecs = pStartPoint->secs;

            const RideFilePoint* pEndPoint = checkRideFile->dataPoints()[iCount - 1];
            routeDistance = pEndPoint->km;
            endLat = pEndPoint->lat;
            endLon = pEndPoint->lon;
            endAlt = pEndPoint->alt;
            endSecs = pEndPoint->secs;
        }

        // Validate route begin, end and distance against the selected workout
        if (context->workout)
        {
            geolocation routeStartGeoloc(startLat, startLon, 0);
            geolocation workoutStartGeoloc(context->workout->Points[0].lat, context->workout->Points[0].lon, 0);

            double d = routeStartGeoloc.DistanceFrom(workoutStartGeoloc);

            geolocation routeEndGeoloc(endLat, endLon, 0);
            geolocation workoutEndGeoloc(context->workout->Points[context->workout->Points.count() - 1].lat, context->workout->Points[context->workout->Points.count() - 1].lon, 0);

            double startDist = abs(routeStartGeoloc.DistanceFrom(workoutStartGeoloc));
            double endDist = abs(routeEndGeoloc.DistanceFrom(workoutEndGeoloc));
            double diffDist = abs(context->workout->Duration - (routeDistance * 1000)); //meters

            //Alert the user if any point is more than 20 meters difference
            //or if the distance diffenrence is more than 100 meters 
            if (abs(startDist) > 20 || abs(endDist) > 20 || abs(diffDist) > 100 ) {
                    QMessageBox::warning(0, QString("Route vs. Workout check!"),
                    QString("Warning!\n\nSelected route does not appear to match the workout\n" 
                            "starting point and/or end point and/or distance.\n\n"
                            "Starting point diff: " + QString::number(startDist, 'f', 2) + "\n"
                            "Ending point diff: " + QString::number(endDist, 'f', 2) + "\n"
                            "Distance diff: " + QString::number(diffDist, 'f', 2)
                    ),
                    QMessageBox::Ok);
            }
        }

        if (!GlobalContext::context()->useMetricUnits) routeDistance *= MILES_PER_KM;
        routeDistanceLbl->setText(QString::number(routeDistance, 'f', 1) + ((GlobalContext::context()->useMetricUnits) ? tr(" KM") : tr(" MI")));
        double routeAvgSpeed = 0.;
        if (routeDistance > 0 && endSecs > 0) {
            routeAvgSpeed = routeDistance / (endSecs / 3600); //KPH 
        }

        routeAvgSpeedLbl->setText(QString::number(routeAvgSpeed, 'f', 1) + ((GlobalContext::context()->useMetricUnits) ? tr(" KPH") : tr(" MPH")));
    }
    else {
        // Show error for NULL RIDE
    }
}

bool SimulatedRidersNestStats::routeMatchesWorkout() {
    return false;
}


void SimulatedRidersNestStats::start(){
    if (!this->context->workout) {
        qDebug() << "Error: Route not selected.";
        return;
    }   
    attackStatus = "";
}

void SimulatedRidersNestStats::setChartHeader() { 
    
    QString sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "").toString();
    int engType = sValue.toInt();
    switch(engType) {
        case 0:
            header->setText("Intervals AI Engine!");
            break;
        case 1:
            header->setText("ERG mode!");
            break;
        case 2:
            header->setText("Follow previous ride!");
            break;
        default :
            header->setText("Virtual Partner Simulation!");
            break;

    }
}

void SimulatedRidersNestStats::saveEnginesOptions() {
    QString isVPEnabled = "0";
    if (enableVPChkBox->checkState() == Qt::Checked) {
        isVPEnabled = "1";
    }
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, isVPEnabled);

    if (radioIntervalsAI->isChecked()) {
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "0");
    }
    else {
        if (radioERGworkout->isChecked()) {
            appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "1");
        }
        else {
            if (radioPreviousRide->isChecked()) {
                appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "2");
            }
        }
    }

    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, qleIntAITotalAttacks->text());
    //Always save in meters
    QString sValue = qleIntAIAttackDuration->text();
    QVariant val = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() / FEET_PER_METER));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKDURATION, QString::number(val.toInt()));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_WARMUPSTARTSPERCENT, qleIntAIWarmUpDuration->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_COOLDOWNSTARTSPERCENT, qleIntAICoolDownDuration->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKPOWERINCREASE, qleIntAIAttackPowerIncrease->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ERG_FILENAME, selectERGFileLabel->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_PREVIOUSRIDE_FILENAME, selectRideFileLabel->text());
    //Always save in meters
    sValue = qleIntAIMaxSeparation->text();
    val = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() / FEET_PER_METER));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_MAXSEPARATION, val.toInt());

    QMessageBox::information(0, QString("Settings Saved"),
        QString("Settings saved succesfully!\n"), 
        QMessageBox::Ok);

    setChartHeader();
}

void SimulatedRidersNestStats::stop()
{
    position1Position->setText("");
    position1Status->setText("");
    position1Watts->setText("");
    position2Watts->setText("");
    position2Position->setText("");
    position2Status->setText("");
    distDiff = 0.;
    setChartHeader();
}

void SimulatedRidersNestStats::pause()
{
}

void SimulatedRidersNestStats::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
}

void SimulatedRidersNestStats::telemetryUpdate(RealtimeData rtd)
{
    if (context->isRunning && !context->isPaused && localSrData.getSimRiderIsEnabled())
    {
        if (localSrData.getSimRiderEngineType() >= 0) {
            setDisplayData(rtd, localSrData.getSimRiderEngineType());
        }
        else {
            header->setText("Unknown Engine type!");
        }
    }

    if (context->isRunning && context->isPaused && localSrData.getSimRiderIsEnabled()) {
        header->setText("Virtual Partner feature is paused!");
    }

    if (context->isRunning && !context->isPaused && !localSrData.getSimRiderIsEnabled()) {
        header->setText("Virtual Partner feature is disabled!");
    }

}

void SimulatedRidersNestStats::SimRiderStateUpdate(SimRiderStateData srData) {
    localSrData = srData;
}

void SimulatedRidersNestStats::setDisplayData(RealtimeData rtd, int engineType) {
    
    if (engineType == 0)
    {
        int displayAttackCount = localSrData.getSimRiderAttackCount() + 1;

        if (localSrData.getSimRiderIsEngineInitialized()) {
            // Display next attack distance
            if (!localSrData.getSimRiderIsAttacking() && displayAttackCount > 0)
            {
                double attackDistance = localSrData.getSimRiderNextAttack() - (rtd.getRouteDistance() * 1000); //Using route distance to support skipping
                header->setText("Next attack # " + QString::number(displayAttackCount, 'f', 0) + " in " + QString::number(attackDistance, 'f', 1) + " " +
                    ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" f")));
                attackStatus = localSrData.getSimRiderAttackStatus();
            }
            else { // SimRider is attacking
                header->setText("Attack # " + QString::number(displayAttackCount - 1, 'f', 0) + " in progress");
                attackStatus = localSrData.getSimRiderAttackStatus();
            }
            if (displayAttackCount == 0) {
                header->setText("No more attacks!");
                attackStatus = localSrData.getSimRiderAttackStatus();
            }
        }
        else {
            header->setText("Engine NOT initialized!");
            attackStatus = " --- ";
        }
    }
    else {
        if (engineType == 1) {
            header->setText("Virtual partner in ERG mode");
            attackStatus = "Following Workout";
        }
        else {
            if (engineType == 2) {
                header->setText("Following previous ride");
                attackStatus = " ";
            }
        }
    }

    if (!localSrData.getSimRiderIsWorkoutFinished()) {
        distDiff = (localSrData.getSimRiderDist() - rtd.getRouteDistance()) * 1000;
    }
    else {
        distDiff = context->workout->Duration - (rtd.getRouteDistance() * 1000);
    }
    setGridData(localSrData.getSimRiderWatts());
}

void SimulatedRidersNestStats::setGridData(double vp1DisplayWatts) {

    //Virtual rider is behind
    if (distDiff < 0.1) {
        position1Name->setText(context->athlete->cyclist);
        position1Name->setStyleSheet("QLabel { color : rgb(43, 130, 203) }");
        //position1Name->setStyleSheet("QLabel{border: 1px solid black; background: red; color: #4A0C46;}");
        position1Position->setText("");
        position1Status->setText("");
        position1Watts->setText("");
        distDiff = ((GlobalContext::context()->useMetricUnits) ? distDiff : (distDiff * FEET_PER_METER));
        position2Position->setText(QString::number(distDiff, 'f', 1) + ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" f")));
        position2Status->setText(attackStatus);
        position2Name->setText("Virtual Partner");
        position2Name->setStyleSheet("QLabel { color : red; }");
        position2Watts->setText(QString::number(vp1DisplayWatts, 'f', 0));
        if (attackStatus == "Attacking") {
            position2Status->setStyleSheet("QLabel { color : red;}");
            position2Watts->setStyleSheet("QLabel { color : red;}");
        }
        else {
            position2Status->setStyleSheet("QLabel { color : white; }");
            position2Watts->setStyleSheet("QLabel { color : white; }");
        }

    }
    else { // Virtual rider is ahead
        position1Name->setText("Virtual Partner");
        position1Name->setStyleSheet("QLabel { color : red; }");
        distDiff = ((GlobalContext::context()->useMetricUnits) ? distDiff : (distDiff * FEET_PER_METER));
        position1Position->setText(QString::number(distDiff, 'f', 1) + ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" f")));
        position1Status->setText(attackStatus);
        position1Watts->setText(QString::number(vp1DisplayWatts, 'f', 0));

        if (attackStatus == "Attacking") {
            position1Status->setStyleSheet("QLabel { color : red;}");
            position1Watts->setStyleSheet("QLabel { color : red;}");
        }
        else {
            position1Status->setStyleSheet("QLabel { color : white; }");
            position1Watts->setStyleSheet("QLabel { color : white; }");
        }
        position2Name->setText(context->athlete->cyclist);
        position2Name->setStyleSheet("QLabel { color : rgb(43, 130, 203); }");
        position2Position->setText("");
        position2Status->setText("");
        position2Watts->setText("");
    }
}
