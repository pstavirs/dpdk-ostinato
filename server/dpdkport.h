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

#ifndef _SERVER_DPDK_PORT_H
#define _SERVER_DPDK_PORT_H

#include "abstractport.h"

#include <QList>
#include <QThread>
#include <rte_ethdev.h>

class DpdkPort: public AbstractPort
{
public:
    DpdkPort(int id, const char *device, struct rte_mempool *mbufPool);
    virtual ~DpdkPort();

    virtual void init();

    static int setBaseId(int baseId);

    virtual bool hasExclusiveControl();
    virtual bool setExclusiveControl(bool exclusive);

    virtual void clearPacketList();
    virtual void loopNextPacketSet(qint64 size, qint64 repeats,
                                   long repeatDelaySec, long repeatDelayNsec);
    virtual bool appendToPacketList(long sec, long nsec, const uchar *packet, 
                                    int length);
    virtual void setPacketListLoopMode(bool loop, 
                                       quint64 secDelay, quint64 nsecDelay);
    virtual void startTransmit();
    virtual void stopTransmit();
    virtual bool isTransmitOn();

    virtual void startCapture();
    virtual void stopCapture();
    virtual bool isCaptureOn();
    virtual QIODevice* captureData();

    // DpdkPort-specific
    void setTransmitLcoreId(unsigned lcoreId);
    void initRxQueueConfig(const struct rte_pci_id *pciId);
    void initTxQueueConfig(const struct rte_pci_id *pciId);

    static int topSpeedTransmit(void *arg);

private:
    class StatsMonitor: public QThread
    {
    public:
        StatsMonitor();
        ~StatsMonitor();
        void run();
        void stop();
    private:
        static const int kRefreshFreq_ = 1; // in seconds
        int portCount_;
        bool stop_;
    };

    typedef struct TxInfo_ {
        int portId;
        bool stopTx;
        struct rte_mempool *pool;
    } TxInfo;


    int dpdkPortId_;
    struct rte_mempool *mbufPool_;
    struct rte_eth_rxconf rxConf_;
    struct rte_eth_txconf txConf_;

    int transmitLcoreId_;
    TxInfo txInfo_;

    static int baseId_;
    static QList<DpdkPort*> allPorts_;
    static StatsMonitor *monitor_; // rx/tx stast for ALL ports
};

#endif
