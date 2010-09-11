/*
 * DEBUG: section 93    ICAP (RFC 3507) Client
 */

#include "squid.h"
#include "comm.h"
#include "HttpReply.h"

#include "adaptation/icap/OptXact.h"
#include "adaptation/icap/Options.h"
#include "adaptation/icap/Config.h"
#include "base/TextException.h"
#include "SquidTime.h"
#include "HttpRequest.h"

CBDATA_NAMESPACED_CLASS_INIT(Adaptation::Icap, OptXact);
CBDATA_NAMESPACED_CLASS_INIT(Adaptation::Icap, OptXactLauncher);


Adaptation::Icap::OptXact::OptXact(Adaptation::Icap::ServiceRep::Pointer &aService):
        AsyncJob("Adaptation::Icap::OptXact"),
        Adaptation::Icap::Xaction("Adaptation::Icap::OptXact", aService)
{
}

void Adaptation::Icap::OptXact::start()
{
    Adaptation::Icap::Xaction::start();

    openConnection();
}

void Adaptation::Icap::OptXact::handleCommConnected()
{
    scheduleRead();

    MemBuf requestBuf;
    requestBuf.init();
    makeRequest(requestBuf);
    debugs(93, 9, HERE << "request " << status() << ":\n" <<
           (requestBuf.terminate(), requestBuf.content()));
    icap_tio_start = current_time;
    scheduleWrite(requestBuf);
}

void Adaptation::Icap::OptXact::makeRequest(MemBuf &buf)
{
    const Adaptation::Service &s = service();
    const String uri = s.cfg().uri;
    buf.Printf("OPTIONS " SQUIDSTRINGPH " ICAP/1.0\r\n", SQUIDSTRINGPRINT(uri));
    const String host = s.cfg().host;
    buf.Printf("Host: " SQUIDSTRINGPH ":%d\r\n", SQUIDSTRINGPRINT(host), s.cfg().port);
    if (TheConfig.allow206_enable)
        buf.Printf("Allow: 206\r\n");
    buf.append(ICAP::crlf, 2);

    // XXX: HttpRequest cannot fully parse ICAP Request-Line
    http_status reqStatus;
    Must(icapRequest->parse(&buf, true, &reqStatus) > 0);
}

void Adaptation::Icap::OptXact::handleCommWrote(size_t size)
{
    debugs(93, 9, HERE << "finished writing " << size <<
           "-byte request " << status());
}

// comm module read a portion of the ICAP response for us
void Adaptation::Icap::OptXact::handleCommRead(size_t)
{
    if (parseResponse()) {
        icap_tio_finish = current_time;
        setOutcome(xoOpt);
        sendAnswer(icapReply);
        Must(done()); // there should be nothing else to do
        return;
    }

    scheduleRead();
}

bool Adaptation::Icap::OptXact::parseResponse()
{
    debugs(93, 5, HERE << "have " << readBuf.contentSize() << " bytes to parse" <<
           status());
    debugs(93, 5, HERE << "\n" << readBuf.content());

    HttpReply::Pointer r(new HttpReply);
    r->protoPrefix = "ICAP/"; // TODO: make an IcapReply class?

    if (!parseHttpMsg(r)) // throws on errors
        return false;

    if (httpHeaderHasConnDir(&r->header, "close"))
        reuseConnection = false;

    icapReply = r;
    return true;
}

void Adaptation::Icap::OptXact::swanSong()
{
    Adaptation::Icap::Xaction::swanSong();
}

void Adaptation::Icap::OptXact::finalizeLogInfo()
{
    //    al.cache.caddr = 0;
    al.icap.reqMethod = Adaptation::methodOptions;

    if (icapReply && al.icap.bytesRead > icapReply->hdr_sz)
        al.icap.bodyBytesRead = al.icap.bytesRead - icapReply->hdr_sz;

    Adaptation::Icap::Xaction::finalizeLogInfo();
}

/* Adaptation::Icap::OptXactLauncher */

Adaptation::Icap::OptXactLauncher::OptXactLauncher(Adaptation::ServicePointer aService):
        AsyncJob("Adaptation::Icap::OptXactLauncher"),
        Adaptation::Icap::Launcher("Adaptation::Icap::OptXactLauncher", aService)
{
}

Adaptation::Icap::Xaction *Adaptation::Icap::OptXactLauncher::createXaction()
{
    Adaptation::Icap::ServiceRep::Pointer s =
        dynamic_cast<Adaptation::Icap::ServiceRep*>(theService.getRaw());
    Must(s != NULL);
    return new Adaptation::Icap::OptXact(s);
}
