/*
 * Copyright (C) 1996-2015 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 33    Client-side Routines */

#ifndef SQUID_SERVERS_SERVER_H
#define SQUID_SERVERS_SERVER_H

#include "anyp/forward.h"
#include "anyp/ProtocolVersion.h"
#include "base/AsyncJob.h"
#include "BodyPipe.h"
#include "comm/forward.h"
#include "CommCalls.h"
#include "SBuf.h"

/**
 * Common base for all Server classes used
 * to manage connections from clients.
 */
class Server : virtual public AsyncJob, public BodyProducer
{
public:
    Server(const MasterXaction::Pointer &xact);
    virtual ~Server() {}

    /* AsyncJob API */
    virtual void start();
    virtual bool doneAll() const;
    virtual void swanSong();

    /// tell all active contexts on a connection about an error
    virtual void notifyAllContexts(const int xerrno) = 0;

    /// ??
    virtual bool connFinishedWithConn(int size) = 0;

    /// processing to be done after a Comm::Read()
    virtual void afterClientRead() = 0;

    /// maybe grow the inBuf and schedule Comm::Read()
    void readSomeData();

    /**
     * called when new request data has been read from the socket
     *
     * \retval false called comm_close or setReplyToError (the caller should bail)
     * \retval true  we did not call comm_close or setReplyToError
     */
    virtual bool handleReadData() = 0;

    /// whether Comm::Read() is scheduled
    bool reading() const {return reader != NULL;}

    /// cancels Comm::Read() if it is scheduled
    void stopReading();

    /// Update flags and timeout after the first byte received
    virtual void receivedFirstByte() = 0;

    /// maybe schedule another Comm::Write() and perform any
    /// processing to be done after previous Comm::Write() completes
    virtual void writeSomeData() {}

    /// whether Comm::Write() is scheduled
    bool writing() const {return writer != NULL;}

// XXX: should be 'protected:' for child access only,
//      but all sorts of code likes to play directly
//      with the I/O buffers and socket.
public:

    /// grows the available read buffer space (if possible)
    bool maybeMakeSpaceAvailable();

    // Client TCP connection details from comm layer.
    Comm::ConnectionPointer clientConnection;

    /**
     * The transfer protocol currently being spoken on this connection.
     * HTTP/1.x CONNECT, HTTP/1.1 Upgrade and HTTP/2 SETTINGS offer the
     * ability to change protocols on the fly.
     */
    AnyP::ProtocolVersion transferProtocol;

    /// Squid listening port details where this connection arrived.
    AnyP::PortCfgPointer port;

    /// read I/O buffer for the client connection
    SBuf inBuf;

    bool receivedFirstByte_; ///< true if at least one byte received on this connection

protected:
    void doClientRead(const CommIoCbParams &io);
    void clientWriteDone(const CommIoCbParams &io);

    AsyncCall::Pointer reader; ///< set when we are reading
    AsyncCall::Pointer writer; ///< set when we are writing
};

#endif /* SQUID_SERVERS_SERVER_H */