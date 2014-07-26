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

static struct rte_eth_conf eth_conf; // FIXME: move to DpdkPort?
const quint64 kMaxValue64 = ULLONG_MAX;

int DpdkPort::baseId_ = -1;
QList<DpdkPort*> DpdkPort::allPorts_;
DpdkPort::StatsMonitor *DpdkPort::monitor_;

DpdkPort::DpdkPort(int id, const char *device, struct rte_mempool *mbufPool)
    : AbstractPort(id, device), mbufPool_(mbufPool)
{
    int ret;
    struct rte_eth_dev_info devInfo;

    Q_ASSERT(baseId_ >= 0);

    // FIXME: this derivation of dpdkPortId_ won't work if one of the previous
    // ports wasn't created for some reason
    dpdkPortId_ = id - baseId_;

    transmitLcoreId_ = -1;

    rte_eth_dev_info_get(dpdkPortId_, &devInfo);

    // FIXME: pass by reference?
    initRxQueueConfig(&devInfo.pci_dev->id);
    initTxQueueConfig(&devInfo.pci_dev->id);

    ret = rte_eth_dev_configure(dpdkPortId_, 
                                1, // # of rx queues
                                1, // # of tx queues
                                &eth_conf);
    if (ret < 0) {
        qWarning("Unable to configure dpdk port %d. err = %d", id, ret);
        goto _error_exit;
    }

    ret = rte_eth_tx_queue_setup(dpdkPortId_,
                                 0,  // queue #
                                 32, // # of descriptors in ring
                                 rte_eth_dev_socket_id(dpdkPortId_),
                                 &txConf_);
    if (ret < 0) {
        qWarning("Unable to configure TxQ for port %d. err = %d", id, ret);
        goto _error_exit;
    }

    ret = rte_eth_rx_queue_setup(dpdkPortId_, 
                                 0,  // queue #
                                 32, // # of descriptors in ring
                                 rte_eth_dev_socket_id(dpdkPortId_),
                                 &rxConf_,
                                 mbufPool_);
    if (ret < 0) {
        qWarning("Unable to configure RxQ for port %d. err = %d", id, ret);
        goto _error_exit;
    }

    ret = rte_eth_dev_start(dpdkPortId_);
    if (ret < 0) {
        qWarning("Unable to start port %d. err = %d", id, ret);
        goto _error_exit;
    }

    rte_eth_promiscuous_enable(dpdkPortId_);

    if (!monitor_)
        monitor_ = new StatsMonitor();

    allPorts_.append(this);
    return;

_error_exit:
    isUsable_ = false;
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

void DpdkPort::setTransmitLcoreId(unsigned lcoreId)
{
    transmitLcoreId_ = int(lcoreId);
}

void DpdkPort::initRxQueueConfig(const struct rte_pci_id *pciId)
{
    memset(&rxConf_, 0, sizeof(rxConf_));

    switch (pciId->device_id) {
#if 0
    case FIXME:
        rxConf_.rx_thresh.pthresh = 8;
        rxConf_.rx_thresh.hthresh = 8;
        rxConf_.rx_thresh.wthresh = 4;
        break;
#endif
    default:
        break;
    }

}

void DpdkPort::initTxQueueConfig(const struct rte_pci_id *pciId)
{
    memset(&txConf_, 0, sizeof(rxConf_));

    switch (pciId->device_id) {
#if 0
    case FIXME:
        txConf_.tx_thresh.pthresh = 36;
        txConf_.tx_thresh.hthresh = 0;
        txConf_.tx_thresh.wthresh = 0;
        txConf_.tx_free_thresh = 0;
        txConf_.tx_rs_thresh = 0;
        txConf_.txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS | ETH_TXQ_FLAGS_NOOFFLOADS;
        break;
#endif
    default:
        break;
    }
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
    int ret;

    if (transmitLcoreId_ < 0) {
        qWarning("Port %d.%s doesn't have a lcore to transmit", id(), name());
        return;
    }

    txInfo_.portId = dpdkPortId_;
    txInfo_.stopTx = false;
    txInfo_.pool = mbufPool_;
    ret = rte_eal_remote_launch(DpdkPort::topSpeedTransmit, &txInfo_, 
                transmitLcoreId_);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to launch transmit\n");
}

void DpdkPort::stopTransmit()
{
    txInfo_.stopTx = true;
    rte_eal_wait_lcore(transmitLcoreId_);
}

bool DpdkPort::isTransmitOn()
{
    return !txInfo_.stopTx;
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

    // FIXME: portStats/linkState QLists may have holes in them if some
    // port create/init failed

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

int DpdkPort::topSpeedTransmit(void *arg)
{
    int n;
    TxInfo *txInfo = (TxInfo*)arg;

    while (!txInfo->stopTx) {
        struct rte_mbuf *mbuf = rte_pktmbuf_alloc(txInfo->pool);
        if (mbuf) {
            rte_pktmbuf_append(mbuf, 64);
            rte_eth_tx_burst(txInfo->portId, 0, &mbuf, 1);
        }
    }

    return 0;
}
