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
            void setPacketListSize(quint64 size);
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
    static int syncTransmit(void *arg);

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

    typedef struct DpdkPacket {
        struct rte_mbuf *mbuf;
        struct rte_mbuf *mbufClone; // clone of mbuf to prevent free by Tx
        long tsSec;
        long tsNsec;
    } DpdkPacket;

    typedef struct DpdkPacketSet {
        quint64 startOfs;
        quint64 endOfs;
        quint64 loopCount;
        quint64 repeatDelayUsec;   // valid only if loopCount > 0

        DpdkPacketSet()
        {
            startOfs = endOfs = 0;
            loopCount = 1;
            repeatDelayUsec = 0;
        }
    } DpdkPacketSet;

    typedef struct DpdkPacketList {
        DpdkPacket *packets;
        quint64 size; // current count of elements in packets[]
        quint64 maxSize; // max number of elements in packets[]

        bool loop;
        quint64 loopDelaySec;   // valid only if loop == true
        quint64 loopDelayNsec;  // valid only if loop == true

        DpdkPacketSet *packetSet;
        quint64 setSize; // current count of elements in packetSet[]

        bool topSpeedTransmit;

        DpdkPacketList()
        {
            reset();
        }
        void reset()
        {
            packets = NULL;
            size = 0;
            maxSize = 0;
            loop = false;
            loopDelaySec = loopDelayNsec = 0;
            packetSet = NULL;
            setSize = 0;
            topSpeedTransmit = true;
        }
    } DpdkPacketList;

    typedef struct TxInfo {
        int portId;
        bool stopTx;
        struct rte_mempool *pool;
        DpdkPacketList *list;

        TxInfo() 
        {
            portId = -1;
            stopTx = true;
            pool = NULL;
            list = NULL;
        }
    } TxInfo;

    int dpdkPortId_;
    struct rte_mempool *mbufPool_;
    struct rte_eth_rxconf rxConf_;
    struct rte_eth_txconf txConf_;

    int transmitLcoreId_;
    TxInfo txInfo_;
    DpdkPacketList packetList_;

    static int baseId_;
    static QList<DpdkPort*> allPorts_;
    static StatsMonitor *monitor_; // rx/tx stats for ALL ports
    static struct rte_mempool *cloneMbufPool_; // only used for clone mbufs
};

#endif
