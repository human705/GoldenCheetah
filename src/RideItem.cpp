/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "RideItem.h"
#include "RideMetric.h"
#include "RideFile.h"
#include "Zones.h"
#include <assert.h>
#include <math.h>

RideItem::RideItem(int type,
                   QString path, QString fileName, const QDateTime &dateTime,
                   const Zones *zones, QString notesFileName) :
    QTreeWidgetItem(type), path(path), fileName(fileName),
    dateTime(dateTime), ride(NULL), zones(zones), notesFileName(notesFileName)
{
    setText(0, dateTime.toString("ddd"));
    setText(1, dateTime.toString("MMM d, yyyy"));
    setText(2, dateTime.toString("h:mm AP"));
    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);
}

RideItem::~RideItem()
{
    MetricIter i(metrics);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
}

int RideItem::zoneRange()
{
    return zones->whichRange(dateTime.date());
}

int RideItem::numZones()
{
    int zone_range = zoneRange();
    return (zone_range >= 0) ? zones->numZones(zone_range) : 0;
}

double RideItem::timeInZone(int zone)
{
    computeMetrics();
    if (!ride)
        return 0.0;
    assert(zone < numZones());
    return time_in_zone[zone];
}

void
RideItem::freeMemory()
{
    if (ride) {
        delete ride;
        ride = NULL;
    }
}

void
RideItem::computeMetrics()
{
    const QDateTime nilTime;
    if ((computeMetricsTime != nilTime) &&
        (computeMetricsTime >= zones->modificationTime)) {
        return;
    }

    if (!ride) {
        QFile file(path + "/" + fileName);
        QStringList errors;
        ride = RideFileFactory::instance().openRideFile(file, errors);
        if (!ride)
            return;
    }

    computeMetricsTime = QDateTime::currentDateTime();

    int zone_range = zoneRange();
    int num_zones = numZones();
    time_in_zone.clear();
    if (zone_range >= 0) {
        num_zones = zones->numZones(zone_range);
        time_in_zone.resize(num_zones);
    }

    double secs_delta = ride->recIntSecs();
    foreach (const RideFilePoint *point, ride->dataPoints()) {
        if (point->watts >= 0.0) {
            if (num_zones > 0) {
                int zone = zones->whichZone(zone_range, point->watts);
                if (zone >= 0)
                    time_in_zone[zone] += secs_delta;
            }
        }
    }

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QSet<QString> todo;

    // hack djconnel: do the metrics TWICE, to catch dependencies
    // on displayed variables.  Presently if a variable depends on zones,
    // for example, and zones change, the value may be considered still
    // value even though it will change.  This is presently happening
    // where bikescore depends on relative intensity.
    // note metrics are only calculated if zones are defined
    for (int metriciteration = 0; metriciteration < 2; metriciteration ++) {
        for (int i = 0; i < factory.metricCount(); ++i) {
            todo.insert(factory.metricName(i));

            while (!todo.empty()) {
                QMutableSetIterator<QString> i(todo);
            later:
                while (i.hasNext()) {
                    const QString &name = i.next();
                    const QVector<QString> &deps = factory.dependencies(name);
                    for (int j = 0; j < deps.size(); ++j)
                        if (!metrics.contains(deps[j]))
                            goto later;
                    RideMetric *metric = factory.newMetric(name);
                    metric->compute(ride, zones, zone_range, metrics);
                    metrics.insert(name, metric);
                    i.remove();
                }
            }
        }
    }
}


