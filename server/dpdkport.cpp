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

DpdkPort::DpdkPort(int id, const char *device)
    : AbstractPort(id, device)
{
    int ret;

    ret = rte_eth_dev_configure(id, 1, 1, &eth_conf);

    if (ret < 0)
        qWarning("Unable to configure dpdk port %d. err = %d", id, ret);
}

DpdkPort::~DpdkPort()
{
}


void DpdkPort::init()
{
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


