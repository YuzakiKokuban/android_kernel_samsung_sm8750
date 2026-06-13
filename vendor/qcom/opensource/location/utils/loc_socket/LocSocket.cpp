/* Copyright (c) 2019 - 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <unistd.h>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#ifdef USE_QSOCKET
#include <sys/ioctl.h>
#include <poll.h>
#include <qsocket.h>
#include <qsocket_ipcr.h>
#else
#include <endian.h>
#include <linux/qrtr.h>
#endif

#include <log_util.h>
#include <LocIpc.h>

namespace loc_util {

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_Qrtr"

class ServiceInfo {
    const int32_t mServiceId;
    const int32_t mInstanceId;
    const string mName;
public:
    inline ServiceInfo(int32_t service, int32_t instance) :
            mServiceId(service), mInstanceId(instance),
            mName(to_string(service) + ":" + to_string(instance)) {}
    inline const char* getName() const { return mName.data(); }
    inline int32_t getServiceId() const { return mServiceId; }
    inline int32_t getInstanceId() const { return mInstanceId; }
};

#ifdef USE_QSOCKET

class LocIpcQsockSender : public LocIpcSender {
protected:
    shared_ptr<Sock> mSock;
    const ServiceInfo mServiceInfo;
    qsockaddr_ipcr mAddr;
    mutable bool mLookupPending;
    inline virtual bool isOperable() const override {
        return mSock != nullptr && mSock->mSid != -1;
    }
    inline void serviceLookup() const {
        if (mLookupPending && mSock->isValid()) {
            ipcr_name_t ipcrName = {
                .service = (uint32_t)mServiceInfo.getServiceId(),
                .instance = (uint32_t)mServiceInfo.getInstanceId()
            };
            uint32_t numEntries = 1;
            int rc = ipcr_find_name(mSock->mSid, &ipcrName,
                    (struct qsockaddr_ipcr*)&mAddr, NULL, &numEntries, 0);
            LOC_LOGd("serviceLookup, rc = %d, numEntries = %d\n", rc, numEntries);
            if (rc < 0 || 1 != numEntries) {
                mSock->close();
            }
            mLookupPending = false;
        }
    }
    inline virtual ssize_t send(const uint8_t data[], uint32_t length,
                                int32_t /* msgId */) const override {
        serviceLookup();
        return mSock->send(data, length, 0, (struct sockaddr*)&mAddr,
                           sizeof(mAddr));
    }
public:
    inline LocIpcQsockSender(const qsockaddr_ipcr& destAddr) :
            LocIpcSender(),
            mSock(make_shared<Sock>(::socket(AF_IPC_ROUTER, SOCK_DGRAM, 0))),
            mServiceInfo(0, 0), mAddr(destAddr), mLookupPending(false) {
    }

    inline LocIpcQsockSender(int service, int instance) :
            LocIpcSender(),
            mSock(make_shared<Sock>(::socket(AF_IPC_ROUTER, SOCK_DGRAM, 0))),
            mServiceInfo(service, instance), mAddr({}), mLookupPending(true) {
    }

    unique_ptr<LocIpcRecver> getRecver(const shared_ptr<ILocIpcListener>& listener) override {
        return make_unique<SockRecver>(listener, *this, mSock);
    }
};

class LocIpcQsockRecver : public LocIpcQsockSender, public LocIpcRecver {
protected:
    inline virtual ssize_t recv() const override {
        socklen_t size = sizeof(mAddr);
        return mSock->recv(*this, mDataCb, 0, (struct sockaddr*)&mAddr, &size);
    }
public:
    inline LocIpcQsockRecver(const shared_ptr<ILocIpcListener>& listener,
                             int service, int instance) :
            LocIpcQsockSender(service, instance),
            LocIpcRecver(listener, *this) {
        qsockaddr_ipcr addr = {
            AF_IPC_ROUTER,
            {
                IPCR_ADDR_NAME,
                {
                    (uint32_t)service,
                    (uint32_t)instance
                }
            },
            0
        };
        if (mSock->isValid() &&
            ::bind(mSock->mSid, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOC_LOGe("bind socket error. sock fd: %d,reason: %s",
                     mSock->mSid, strerror(errno));
            mSock->close();
        }
    }
    inline virtual unique_ptr<LocIpcSender> getLastSender() const override {
        return make_unique<LocIpcQsockSender>(mAddr);
    }
    inline virtual const char* getName() const override {
        return mServiceInfo.getName();
    }
    inline virtual void abort() const override {
        if (isSendable()) {
            serviceLookup();
            mSock->sendAbort(0, (struct sockaddr*)&mAddr, sizeof(mAddr));
        }
    }
};

shared_ptr<LocIpcSender> createLocIpcQrtrSender(int service, int instance) {
    return make_shared<LocIpcQsockSender>(service, instance);
}
unique_ptr<LocIpcRecver> createLocIpcQrtrRecver(const shared_ptr<ILocIpcListener>& listener,
                                                int service, int instance,
                                                const shared_ptr<LocIpcQrtrWatcher>& qrtrWatcher) {
    return make_unique<LocIpcQsockRecver>(listener, service, instance);
}

#else

#define SOCKET_SENDER_SEND_TIMEOUT_MSEC 100

static inline __le32 cpu_to_le32(uint32_t x) { return htole32(x); }
static inline uint32_t le32_to_cpu(__le32 x) { return le32toh(x); }

class LocIpcQrtrSender : public LocIpcSender {
protected:
    const ServiceInfo mServiceInfo;
    shared_ptr<Sock> mSock;
    mutable sockaddr_qrtr mAddr;
    mutable sockaddr_qrtr mCtrlPntAddr;
    mutable struct qrtr_ctrl_pkt mCtrlPkt;
    mutable bool mLookupPending;
    bool ctrlCmdAndResponse(enum qrtr_pkt_type cmd) const {
        LOC_LOGd("cmd: %d, sock valid %d, sock id %d, service id %d, instance id %d",
                 cmd, mSock->isValid(), mSock->mSid,
                 le32_to_cpu(mCtrlPkt.server.service),
                 le32_to_cpu(mCtrlPkt.server.instance));
        if (mSock->isValid()) {
            int rc = 0;
            mCtrlPkt.cmd = cpu_to_le32(cmd);
            if ((rc = ::sendto(mSock->mSid, &mCtrlPkt, sizeof(mCtrlPkt), 0,
                               (const struct sockaddr *)&mCtrlPntAddr, sizeof(mCtrlPntAddr))) < 0) {
                LOC_LOGe("failed: sendto rc=%d reason=(%s)", rc, strerror(errno));
            } else if (QRTR_TYPE_NEW_LOOKUP == cmd) {
                int len = 0;
                struct qrtr_ctrl_pkt pkt;
                while ((len = ::recv(mSock->mSid, &pkt, sizeof(pkt), 0)) > 0) {
                    if (len >= (decltype(len))sizeof(pkt)) {
                        qrtr_pkt_type pktType = (qrtr_pkt_type) le32_to_cpu(pkt.cmd);
                        LOC_LOGd("qrtr new lookup received pkt type: %d, "
                                 "pkt service id: %d, instance id: %d,"
                                 "node %d, port %d",
                                 pktType, le32_to_cpu(pkt.server.service),
                                 le32_to_cpu(pkt.server.instance),
                                 le32_to_cpu(pkt.server.node),
                                 le32_to_cpu(pkt.server.port));

                        switch (pktType){
                        case QRTR_TYPE_NEW_SERVER:
                            if (le32_to_cpu(pkt.server.service) == 0 &&
                                       le32_to_cpu(pkt.server.instance) == 0) {
                                return false;
                            } else if ((mCtrlPkt.server.service == pkt.server.service) &&
                                (mCtrlPkt.server.instance == pkt.server.instance)) {
                                mAddr.sq_node = le32_to_cpu(pkt.server.node);
                                mAddr.sq_port = le32_to_cpu(pkt.server.port);
                                return true;
                            }
                            break;
                        case QRTR_TYPE_DEL_SERVER:
                            if ((mCtrlPkt.server.service == pkt.server.service) &&
                                (mCtrlPkt.server.instance == pkt.server.instance)) {
                                // service of particular service id, instance id gets deleted
                                return false;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        LOC_LOGd("cmd: %d, return %d", cmd, mSock->isValid());
        return mSock->isValid();
    }

    inline virtual bool isOperable() const override {
        return mSock != nullptr && mSock->isValid() &&
            (mAddr.sq_node != 0 || mAddr.sq_port != 0);
    }

    inline virtual ssize_t send(const uint8_t data[], uint32_t length,
                                int32_t /* msgId */) const override {
        if (mLookupPending) {
            if (ctrlCmdAndResponse(QRTR_TYPE_NEW_LOOKUP) == false) {
                return 0;
            }
            mLookupPending = false;
        }
        int result = mSock->send(data, length, 0, (struct sockaddr*)&mAddr, sizeof(mAddr));
        return result;
    }
public:
    inline LocIpcQrtrSender(const sockaddr_qrtr& destAddr) :
            LocIpcSender(), mServiceInfo(0, 0),
            mSock(make_shared<Sock>(::socket(AF_QIPCRTR, SOCK_DGRAM, 0))),
            mAddr(destAddr), mCtrlPkt({}), mLookupPending(false) {
    }
    inline LocIpcQrtrSender(int service, int instance) : LocIpcSender(),
            mServiceInfo(service, instance),
            mSock(make_shared<Sock>(::socket(AF_QIPCRTR, SOCK_DGRAM, 0))),
            mAddr({AF_QIPCRTR, 0, 0}),
            mCtrlPkt({}),
            mLookupPending(true) {
        // set timeout so if failed to send, call will return after SOCKET_TIMEOUT_MSEC
        // otherwise, call may never return
        timeval timeout;

        timeout.tv_sec = SOCKET_SENDER_SEND_TIMEOUT_MSEC / 1000;
        timeout.tv_usec = SOCKET_SENDER_SEND_TIMEOUT_MSEC % 1000 * 1000;
        setsockopt(mSock->mSid, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        socklen_t sl = sizeof(mAddr);
        int rc = 0;
        if ((rc = getsockname(mSock->mSid, (struct sockaddr*)&mAddr, &sl)) ||
            mAddr.sq_family != AF_QIPCRTR || sl != sizeof(mAddr)) {
            LOC_LOGe("failed: getsockname rc=%d reason=(%s), mAddr.sq_family=%d",
                     rc, strerror(errno), mAddr.sq_family);
            mSock->close();
        } else {
            mCtrlPkt.server.service = cpu_to_le32(service);
            mCtrlPkt.server.instance = cpu_to_le32(instance);
            mCtrlPkt.server.node = cpu_to_le32(mAddr.sq_node);
            mCtrlPkt.server.port = cpu_to_le32(mAddr.sq_port);
            mCtrlPntAddr = mAddr;
            mCtrlPntAddr.sq_port = QRTR_PORT_CTRL;
        }
    }

    unique_ptr<LocIpcRecver> getRecver(const shared_ptr<ILocIpcListener>& listener) override {
        return make_unique<SockRecver>(listener, *this, mSock);
    }

    inline virtual bool updateDestAddr(uint32_t nodeId, uint32_t portId)
    {
        if (mAddr.sq_node != nodeId || mAddr.sq_port != portId) {
            sockaddr_qrtr addr = {AF_QIPCRTR, nodeId, portId};
            mAddr = addr;
        }
        mLookupPending = false;
        return true;
    }
};

class LocIpcQrtrListener : public ILocIpcListener {
    const shared_ptr<ILocIpcListener> mRealListener;
    const shared_ptr<LocIpcQrtrWatcher> mQrtrWatcher;
    inline bool handleQrtrCtrlMsg(const char* data, uint32_t len) {
        const struct qrtr_ctrl_pkt* pkt = reinterpret_cast<const struct qrtr_ctrl_pkt*>(data);
        bool handledAsQrtrCtrlMsg = false;

        if (sizeof(*pkt) == len) {
            const uint32_t cmd = le32_to_cpu(pkt->cmd);
            if (cmd >= QRTR_TYPE_DATA && cmd <= QRTR_TYPE_DEL_LOOKUP) {
                handledAsQrtrCtrlMsg = true;
                int serviceId = le32_to_cpu(pkt->server.service);
                int instanceId = le32_to_cpu(pkt->server.instance);
                uint32_t serverNodeId = le32_to_cpu(pkt->server.node);
                uint32_t serverPort = le32_to_cpu(pkt->server.port);
                int clientNodeId = le32_to_cpu(pkt->client.node);
                int clientPort = le32_to_cpu(pkt->client.port);

                LOC_LOGv("qrtr control msg: cmd %d, server.service:instance-node:port %d:%d-%d:%d,"
                         " client node:port %d:%d", cmd, serviceId, instanceId, serverNodeId,
                         serverPort, clientNodeId, clientPort);

                if (nullptr != mQrtrWatcher) {
                    if ((QRTR_TYPE_NEW_SERVER == cmd || QRTR_TYPE_DEL_SERVER == cmd) &&
                        mQrtrWatcher->isServiceInWatch(serviceId)) {
                        LocIpcQrtrWatcher::ServiceStatus status = (QRTR_TYPE_NEW_SERVER == cmd) ?
                                LocIpcQrtrWatcher::ServiceStatus::UP :
                                LocIpcQrtrWatcher::ServiceStatus::DOWN;
                        mQrtrWatcher->onServiceStatusChange(serviceId, instanceId, status,
                                                            serverNodeId, serverPort);
                    } else if ((QRTR_TYPE_DEL_CLIENT == cmd || QRTR_TYPE_BYE == cmd) &&
                               mQrtrWatcher->isClientInWatch(clientNodeId)) {
                        mQrtrWatcher->onClientGone(clientNodeId, clientPort);
                    }
                }
            }
        }
        return handledAsQrtrCtrlMsg;
    }
public:
    inline LocIpcQrtrListener(const shared_ptr<ILocIpcListener>& listener,
                              const shared_ptr<LocIpcQrtrWatcher>& qrtrWatcher) :
            mRealListener(listener), mQrtrWatcher(qrtrWatcher) {}
    inline virtual void onListenerReady() override {
        if (nullptr != mRealListener) mRealListener->onListenerReady();
    }
    inline virtual void onReceive(const char* data, uint32_t len,
                                  const LocIpcRecver* recver) override {
        if ((!handleQrtrCtrlMsg(data, len)) && (nullptr != mRealListener)) {
            mRealListener->onReceive(data, len, recver);
        }
    }
};

class LocIpcQrtrRecver : public LocIpcQrtrSender, public LocIpcRecver {
protected:
    inline virtual ssize_t recv() const override {
        socklen_t size = sizeof(mAddr);
        return mSock->recv(*this, mDataCb, 0, (struct sockaddr*)&mAddr, &size);
    }
public:
    inline LocIpcQrtrRecver(const shared_ptr<ILocIpcListener>& listener,
                            int service, int instance,
                            const shared_ptr<LocIpcQrtrWatcher>& qrtrWatcher) :
            LocIpcQrtrSender(service, instance),
            LocIpcRecver(make_shared<LocIpcQrtrListener>(listener, qrtrWatcher), *this) {

        ctrlCmdAndResponse(QRTR_TYPE_NEW_SERVER);

        if (nullptr != qrtrWatcher) {
            struct qrtr_ctrl_pkt pkt = {};
            pkt.cmd = cpu_to_le32(QRTR_TYPE_NEW_LOOKUP);
            int rc = 0;
            for (int serviceId : qrtrWatcher->getServicesToWatch()) {
                pkt.server.service = cpu_to_le32(serviceId);
                LOC_LOGd("watch for service: %d", pkt.server.service);
                if ((rc = ::sendto(mSock->mSid, &pkt, sizeof(pkt), 0,
                                   (const struct sockaddr *)&mCtrlPntAddr,
                                   sizeof(mCtrlPntAddr))) < 0) {
                    LOC_LOGe("failed: sendto rc=%d reason=(%s)\n", rc, strerror(errno));
                }
            }
        }
    }

    inline ~LocIpcQrtrRecver() { ctrlCmdAndResponse(QRTR_TYPE_DEL_SERVER); }
    inline virtual unique_ptr<LocIpcSender> getLastSender() const override {
        return make_unique<LocIpcQrtrSender>(mAddr);
    }
    inline virtual const char* getName() const override {
        return mServiceInfo.getName();
    }
    inline virtual void abort() const override {
        if (isSendable()) {
            mSock->sendAbort(0, (struct sockaddr*)&mAddr, sizeof(mAddr));
        }
    }
};

shared_ptr<LocIpcSender> createLocIpcQrtrSender(int service, int instance) {
    return make_shared<LocIpcQrtrSender>(service, instance);
}
unique_ptr<LocIpcRecver> createLocIpcQrtrRecver(const shared_ptr<ILocIpcListener>& listener,
                                                int service, int instance,
                                                const shared_ptr<LocIpcQrtrWatcher>& qrtrWatcher) {
    return make_unique<LocIpcQrtrRecver>(listener, service, instance, qrtrWatcher);
}

#endif

}
