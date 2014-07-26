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
#include <rte_pci.h>

#include <QList>
#include <net/if.h>

// TODO: Move everything here into a singleton class - DpdkPortManager ?

static struct rte_mempool *mbufPool_ = NULL;
static int lcoreCount_;
static quint64 lcoreFreeMask_;
static int rxLcoreId_;
static bool stopRxPoll_;

int getFreeLcore() 
{
    for (int i = 0; i < lcoreCount_; i++) {
        if (lcoreFreeMask_ & (1 << i)) {
            lcoreFreeMask_ &= ~(1 << i);
            return i;
        }
    }

    return -1;
}

int pkts = 0;

int pollRxRings(void *arg)
{
    int i, count = rte_eth_dev_count();
    struct rte_mbuf* rxPkts[32];

    while (!stopRxPoll_) {
        for (i = 0; i < count; i++) {
            int n = rte_eth_rx_burst(i,
                                 0, // Queue#
                                 rxPkts,
                                 sizeof(rxPkts)/sizeof(rxPkts[0]));
            pkts += n;
            for (int j = 0; j < n; j++) 
                rte_pktmbuf_free(rxPkts[j]);
        }
    }
    qDebug("DPDK Rx polling stopped");

    return 0;
}

int dpdkStopPolling()
{
    stopRxPoll_ = true;
    rte_eal_wait_lcore(rxLcoreId_);
    return 0; 
}

int initDpdk(char* progname) 
{
    int ret;
    static char *eal_args[] = {progname, "-c0xf", "-n1", "-m128", "--file-prefix=drone"};

    // TODO: read env var DRONE_RTE_EAL_ARGS to override defaults

    ret = rte_eal_init(sizeof(eal_args)/sizeof(char*), eal_args);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");

    mbufPool_ = rte_mempool_create("DpktPktMbuf",
                                      16*1024, // # of mbufs
                                      2048, // sz of mbuf
                                      32,   // per-lcore cache sz
                                      sizeof(struct rte_pktmbuf_pool_private),
                                      rte_pktmbuf_pool_init, // pool ctor
                                      NULL, // pool ctor arg
                                      rte_pktmbuf_init, // mbuf ctor
                                      NULL, // mbuf ctor arg
                                      SOCKET_ID_ANY,
                                      0     // flags
                                      );

    if (!mbufPool_)
        rte_exit(EXIT_FAILURE, "cannot init mbuf pool\n");

    if (rte_pmd_init_all() < 0)
        rte_exit(EXIT_FAILURE, "cannot init pmd\n");

    if (rte_eal_pci_probe() < 0)
        rte_exit(EXIT_FAILURE, "cannot probe PCI\n");

    // init lcore information
    lcoreCount_ = rte_lcore_count();
    lcoreFreeMask_ = 0;
    for (int i = 0; i < lcoreCount_; i++) {
        if (rte_lcore_is_enabled(i) && (unsigned(i) != rte_get_master_lcore()))
            lcoreFreeMask_ |= (1 << i);
    }
    qDebug("lcore_count = %d, lcore_free_mask = 0x%llx", 
            lcoreCount_, lcoreFreeMask_);

    // assign a lcore for Rx polling
    rxLcoreId_ = getFreeLcore();
    if (rxLcoreId_ < 0)
        rte_exit(EXIT_FAILURE, "not enough cores for Rx polling");

    stopRxPoll_ = false;

    return 0;
}

QList<AbstractPort*> createDpdkPorts(int baseId) 
{
    QList<AbstractPort*> portList;
    int ret, count = rte_eth_dev_count();

    DpdkPort::setBaseId(baseId);

    for (int i = 0; i < count ; i++) {
        struct rte_eth_dev_info info;
        char if_name[IF_NAMESIZE];
        DpdkPort *port;
        int lcore_id;

        rte_eth_dev_info_get(i, &info);

        // Use Predictable Interface Naming Convention
        // <http://www.freedesktop.org/wiki/Software/systemd/PredictableNetworkInterfaceNames/>
        if (info.pci_dev->addr.domain)
            snprintf(if_name, sizeof(if_name), "enP%up%us%u", 
                    info.pci_dev->addr.domain, 
                    info.pci_dev->addr.bus, 
                    info.pci_dev->addr.devid);
        else
            snprintf(if_name, sizeof(if_name), "enp%us%u", 
                    info.pci_dev->addr.bus, 
                    info.pci_dev->addr.devid);

        qDebug("%d. %s", baseId, if_name);
        qDebug("dpdk %d: %u "
                "min_rx_buf = %u, max_rx_pktlen = %u, "
                "maxq rx/tx = %u/%u", 
                i, info.if_index,
                info.min_rx_bufsize, info.max_rx_pktlen,
                info.max_rx_queues, info.max_tx_queues);
        port = new DpdkPort(baseId++, if_name, mbufPool_);
        if (!port->isUsable())
        {
            qDebug("%s: unable to open %s. Skipping!", __FUNCTION__,
                    if_name);
            delete port;
            baseId--;
            continue;
        }

        lcore_id = getFreeLcore();
        if (lcore_id >=  0) {
            port->setTransmitLcoreId(lcore_id);
        }
        else {
            qWarning("Not enough cores - port %d.%s cannot transmit", 
                    baseId, if_name);
        }

        portList.append(port);
    }

    ret = rte_eal_remote_launch(pollRxRings, NULL, rxLcoreId_);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot launch poll-rx-rings\n");

    return portList;
}

