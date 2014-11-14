#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdio>
#include <signal.h>

#include "epoll_reactor.h"
#include "log.h"
#include "listener.h"
#include "master_worker.h"
#include "protocol_handler.h"
#include "status_updater.h"
#include "configure_manager.h"
#include "sock_util.h"


struct MasterWorkerFactory : IWorkerFactory
{
    MasterWorkerFactory(StatusUpdater *statusUpdater,
            ConfigureManager *confManager) :
        m_statusUpdater(statusUpdater), m_confManager(confManager)
    {
#ifdef DEBUG
    log_info("MasterWorkerFactory constructor");
#endif
    }

    void OnFdAccepted(epoll_reactor *reactor, int fd)
    {
        sockaddr_in peerName;
        socklen_t len = sizeof(sockaddr_in);
        const char *peer = "UNKNOWN";
        if(getpeername(fd, (sockaddr *)&peerName, &len) == 0)
        {
            peer = inet_ntoa(peerName.sin_addr);
        }

        new ProtocolHandler(fd, reactor,
                new MasterWorker(peer, m_statusUpdater, m_confManager));
    }

    void OnAcceptError(listener *l)
    {
        //nothing
    }

private:
    StatusUpdater *m_statusUpdater;
    ConfigureManager *m_confManager;
};

static const char *listenAt;
static const char *dbHost, *dbUser, *dbPassword, *dbPath;
const char *dbName;
int dbPort = 0;

static
void usage(const char *app)
{
    printf("%s -l listen_at [-h db_host -P db_port] [-u db_user -p db_password] "
            "[-U db_unixpath] -d db_name\n", app);
    exit(1);
}

static
void parse(int argc, char **argv)
{
    int c = -1;
    while((c = getopt(argc, argv, "l:h:P:u:p:U:d:")) != -1)
    {
        switch(c)
        {
        case 'l':
            listenAt = optarg;
            break;
        case 'h':
            dbHost = optarg;
            break;
        case 'P':
            dbPort = atoi(optarg);
            break;
        case 'u':
            dbUser = optarg;
            break;
        case 'p':
            dbPassword = optarg;
            break;
        case 'U':
            dbPath = optarg;
            break;
        case 'd':
            dbName = optarg;
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    if(!listenAt || (!dbPort && !dbPath) || (dbPort && !dbHost))
    {
        usage(argv[0]);
        return;
    }
}

epoll_reactor *g_reactor;

static void stop_reactor(int)
{
    if(g_reactor)
        g_reactor->stop();
}

int main(int argc, char **argv)
{
    parse(argc, argv);

    sockaddr_in addr;
    if(string_to_sock_addr(listenAt, &addr) != 0)
    {
        fprintf(stderr, "invalid listen addr: %s\n", listenAt);
        return 1;
    }

    int fd = listen_or_die(addr, 128);
    set_defer_accept(fd);
    set_nonblocking(fd);

    ConfigureManager confManager;
    confManager.Init(dbHost, dbPort, dbPath, dbUser, dbPassword, dbName);

    StatusUpdater updater(dbHost, dbPort, dbPath, dbUser, dbPassword, dbName);
    if(updater.Connect() != 0)
    {
        fprintf(stderr, "connect to mysql failed.\n");
        return 1;
    }

    _init_log_("agentm", "../log/");
    daemon(1, 0);

    signal(SIGINT, stop_reactor);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, stop_reactor);

    log_info("agentm starting ...");
    epoll_reactor reactor(1024);
    g_reactor = &reactor;

    MasterWorkerFactory workerFactory(&updater, &confManager);

    listener *l = new listener(&reactor, fd, &workerFactory);
    (void)l;    //make compiler happy
    reactor.run();

    log_info("agentm stopped.");
    return 0;
}


