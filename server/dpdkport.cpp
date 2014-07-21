/*
Copyright (C) 2014 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "dpdkport.h"

#include <rte_ethdev.h>

static struct rte_eth_conf eth_conf;
const quint64 kMaxValue64 = ULLONG_MAX;

int DpdkPort::baseId_ = -1;
QList<DpdkPort*> DpdkPort::allPorts_;
DpdkPort::StatsMonitor *DpdkPort::monitor_;

DpdkPort::DpdkPort(int id, const char *device)
    : AbstractPort(id, device)
{
    int ret;

    Q_ASSERT(baseId_ >= 0);

#if 0
    ret = rte_eth_dev_configure(id - baseId_, 1, 1, &eth_conf);
    if (ret < 0)
        qWarning("Unable to configure dpdk port %d. err = %d", id, ret);
#endif

    if (!monitor_)
        monitor_ = new StatsMonitor();

    allPorts_.append(this);
}

DpdkPort::~DpdkPort()
{
    qDebug("In %s", __FUNCTION__);

    if (monitor_->isRunning())
    {
        monitor_->stop();
        monitor_->wait();
    }
}


void DpdkPort::init()
{
    if (!monitor_->isRunning())
        monitor_->start();
}

int DpdkPort::setBaseId(int baseId)
{
    baseId_ = baseId;
    return 0;
}


bool DpdkPort::hasExclusiveControl()
{
    return false;
}

bool DpdkPort::setExclusiveControl(bool exclusive)
{
    return false;
}


void DpdkPort::clearPacketList()
{
}

void DpdkPort::loopNextPacketSet(qint64 size, qint64 repeats,
                               long repeatDelaySec, long repeatDelayNsec)
{
}

bool DpdkPort::appendToPacketList(long sec, long nsec, const uchar *packet, 
                                int length)
{
    return false;
}

void DpdkPort::setPacketListLoopMode(bool loop, 
                                   quint64 secDelay, quint64 nsecDelay)
{
}

void DpdkPort::startTransmit()
{
}

void DpdkPort::stopTransmit()
{
}

bool DpdkPort::isTransmitOn()
{
    return false;
}


void DpdkPort::startCapture()
{
}

void DpdkPort::stopCapture()
{
}

bool DpdkPort::isCaptureOn()
{
    return false;
}

QIODevice* DpdkPort::captureData()
{
    return NULL;
}

DpdkPort::StatsMonitor::StatsMonitor() 
{
    portCount_ = rte_eth_dev_count();
    stop_ = false;
}

DpdkPort::StatsMonitor::~StatsMonitor() 
{
}


void DpdkPort::StatsMonitor::stop()
{
    stop_ = true;
}

void DpdkPort::StatsMonitor::run() 
{
    QList<PortStats*> portStats;
    QList<OstProto::LinkState*> linkState;
    int i;

    //
    // Populate the port stats/link lists
    //
    for (i = 0; i < portCount_; i++) {
        portStats.insert(i, &(allPorts_.at(i)->stats_));
        linkState.insert(i, &(allPorts_.at(i)->linkState_));
    }

    //
    // We are all set - Let's start polling for stats!
    //
    while (!stop_)
    {
        for (i = 0; i < portCount_; i++) {
            struct rte_eth_stats rteStats;
            struct rte_eth_link rteLink;
            AbstractPort::PortStats *stats = portStats[i];
            OstProto::LinkState *state = linkState[i];

            rte_eth_stats_get(i, &rteStats);
            if (!stats)
                continue;

            stats->rxPps = 
                ((rteStats.ipackets >= stats->rxPkts) ?
                 rteStats.ipackets - stats->rxPkts :
                 rteStats.ipackets + (kMaxValue64 
                     - stats->rxPkts))
                / kRefreshFreq_;
            stats->rxBps = 
                ((rteStats.ibytes >= stats->rxBytes) ?
                 rteStats.ibytes - stats->rxBytes :
                 rteStats.ibytes + (kMaxValue64 
                     - stats->rxBytes))
                / kRefreshFreq_;
            stats->rxPkts  = rteStats.ipackets;
            stats->rxBytes = rteStats.ibytes;

            stats->txPps = 
                ((rteStats.opackets >= stats->txPkts) ?
                 rteStats.opackets - stats->txPkts :
                 rteStats.opackets + (kMaxValue64 
                     - stats->txPkts))
                / kRefreshFreq_;
            stats->txBps = 
                ((rteStats.obytes >= stats->txBytes) ?
                 rteStats.obytes - stats->txBytes :
                 rteStats.obytes + (kMaxValue64 
                     - stats->txBytes))
                / kRefreshFreq_;
            stats->txPkts  = rteStats.opackets;
            stats->txBytes = rteStats.obytes;

            // TODO: export detailed error stats
            stats->rxDrops = rteStats.rx_nombuf; 
            stats->rxErrors = rteStats.ierrors;

            // TODO: export rteStats.oerrors

            Q_ASSERT(state);

            // TODO: investigate if _nowait() is costly
            rte_eth_link_get_nowait(i, &rteLink);
            *state = rteLink.link_status ?
                OstProto::LinkStateUp : OstProto::LinkStateDown;

            QThread::sleep(kRefreshFreq_);
        }
    }

    portStats.clear();
    linkState.clear();
}


