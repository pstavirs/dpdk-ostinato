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

int initDpdk(char* progname) 
{
    int ret;
    static char *eal_args[] = {progname, "-c0xf", "-n1", "-m128", "--file-prefix=drone"};

    ret = rte_eal_init(sizeof(eal_args)/sizeof(char*), eal_args);
    if (ret < 0)
        rte_panic("Cannot init EAL\n");

    if (rte_pmd_init_all() < 0)
        rte_exit(EXIT_FAILURE, "cannot init pmd\n");

    if (rte_eal_pci_probe() < 0)
        rte_exit(EXIT_FAILURE, "cannot probe PCI\n");

    return 0;
}

QList<AbstractPort*> createDpdkPorts(int baseId) 
{
    QList<AbstractPort*> portList;
    int i, count = rte_eth_dev_count();

    DpdkPort::setBaseId(baseId);

    for (i = 0; i < count ; i++) {
        struct rte_eth_dev_info info;
        char if_name[IF_NAMESIZE];
        AbstractPort *port;

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
        port = new DpdkPort(baseId++, if_name);
        if (!port->isUsable())
        {
            qDebug("%s: unable to open %s. Skipping!", __FUNCTION__,
                    if_name);
            delete port;
            baseId--;
            continue;
        }

        portList.append(port);
    }

    return portList;
}
