
check_include_file(alloca.h     	HAVE_ALLOCA_H)
check_include_file(arpa/inet.h  	HAVE_ARPA_INET_H)
check_include_file(arpa/nameser.h	HAVE_ARPA_NAMESER_H)
check_include_file(grp.h        	HAVE_GRP_H)
check_include_file(ifaddrs.h    	HAVE_IFADDRS_H)
check_include_file(inttypes.h   	HAVE_INTTYPES_H)
check_include_file(malloc.h     	HAVE_MALLOC_H)
check_include_file(netdb.h      	HAVE_NETDB_H)
check_include_file(pwd.h        	HAVE_PWD_H)
check_include_file(syslog.h     	HAVE_SYSLOG_H)
check_include_file(sys/epoll.h  	HAVE_EPOLL)
check_include_file(sys/file.h   	HAVE_SYS_FILE_H)
check_include_file(sys/filio.h  	HAVE_SYS_FILIO_H)
check_include_file(sys/ioctl.h  	HAVE_SYS_IOCTL_H)
check_include_file(sys/kstat.h  	HAVE_SYS_KSTAT_H)
check_include_file(sys/loadavg.h	HAVE_SYS_LOADAVG_H)
check_include_file(sys/malloc.h 	HAVE_SYS_MALLOC_H)
check_include_file(sys/param.h  	HAVE_SYS_PARAM_H)
check_include_file(sys/poll.h   	HAVE_POLL)
check_include_file(sys/prctl.h  	HAVE_SYS_PRCTL_H)
check_include_file(sys/select.h 	HAVE_SYS_SELECT_H)
check_include_file(sys/signal.h 	HAVE_SYS_SIGNAL_H)
check_include_file(sys/sockio.h 	HAVE_SYS_SOCKIO_H)
check_include_file(sys/stat.h   	HAVE_SYS_STAT_H)
check_include_file(sys/sysinfo.h	HAVE_SYS_SYSINFO_H)
check_include_file(sys/time.h   	HAVE_SYS_TIME_H)
check_include_file(sys/types.h  	HAVE_SYS_TYPES_H)
check_include_file(sys/uio.h    	HAVE_SYS_UIO_H)
check_include_file(sys/wait.h   	HAVE_SYS_WAIT_H)
check_include_file(unistd.h     	HAVE_UNISTD_H)

check_include_file(io.h         	HAVE_IO_H)
check_include_file(sched.h      	HAVE_SCHED_H)
check_include_file(signal.h     	HAVE_SIGNAL_H)
check_include_file(process.h    	HAVE_PROCESS_H)
check_include_file(io.h         	HAVE_IO_H)
check_include_file(dlfcn.h      	HAVE_DLFCN_H)
check_include_file(net/if_dl.h  	HAVE_NET_DL_H)
check_include_file(kvm.h        	HAVE_KVM_H)
check_include_file(kstat.h      	HAVE_KSTAT_H)
check_include_file(fcntl.h      	HAVE_FCNTL_H)
check_include_file(utime.h      	HAVE_UTIME_H)
check_include_file(limits.h     	HAVE_LIMITS_H)
check_include_file(dmalloc.h    	HAVE_DMALLOC_H)


check_include_file(sys/socket.h 	HAVE_SYS_SOCKET_H)
# OpenBSD fails with this test
IF (NOT HAVE_SYS_SOCKET_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\nint main(void) { };" HAVE_SYS_SOCKET_H_TMP)
    IF ( HAVE_SYS_SOCKET_H_TMP )
	SET(HAVE_SYS_SOCKET_H 1)
    ENDIF ( HAVE_SYS_SOCKET_H_TMP )
ENDIF (NOT HAVE_SYS_SOCKET_H)

check_include_file(netinet/in.h		HAVE_NETINET_IN_H)
# OpenBSD fails with this test
IF (NOT HAVE_NETINET_IN_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/mman.h>\nint main(void) { };" HAVE_NETINET_IN_H_TMP)
    IF ( HAVE_NETINET_IN_H_TMP )
	SET(HAVE_NETINET_IN_H 1)
    ENDIF ( HAVE_NETINET_IN_H_TMP )
ENDIF (NOT HAVE_NETINET_IN_H)


check_include_file(netinet/if.h		HAVE_NETINET_IF_H)
check_include_file(netinet/ip.h		HAVE_NETINET_IP_H)
check_include_file(netinet/in6.h	HAVE_NETINET_IN6_H)
check_include_file(netinet/ip6.h	HAVE_NETINET_IP6_H)
check_include_file(netinet/tcp.h	HAVE_NETINET_TCP_H)


check_include_file(sys/mman.h   	HAVE_SYS_MMAN_H)
# OpenBSD fails with this test
IF (NOT HAVE_SYS_MMAN_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/mman.h>\nint main(void) { };" HAVE_SYS_MMAN_H_TMP)
    IF ( HAVE_SYS_MMAN_H_TMP )
	SET(HAVE_SYS_MMAN_H 1)
    ENDIF ( HAVE_SYS_MMAN_H_TMP )
ENDIF (NOT HAVE_SYS_MMAN_H)


check_include_file(sys/sysctl.h  	HAVE_SYS_SYSCTL_H)
# Freebsd fails with this test
IF (NOT HAVE_SYS_SYSCTL_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/sysctl.h>\nint main(void) { };" HAVE_SYS_SYSCTL_H_TMP)
    IF ( HAVE_SYS_SYSCTL_H_TMP )
	SET(HAVE_SYS_SYSCTL_H 1)
    ENDIF ( HAVE_SYS_SYSCTL_H_TMP )
ENDIF (NOT HAVE_SYS_SYSCTL_H)

IF (NOT HAVE_SYS_SYSCTL_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/param.h>\n#include <sys/types.h>\n#include <sys/sysctl.h>\nint main(void) { };" HAVE_SYS_SYSCTL_H_TMP2)
    IF ( HAVE_SYS_SYSCTL_H_TMP2 )
	SET(HAVE_SYS_SYSCTL_H 1)
    ENDIF ( HAVE_SYS_SYSCTL_H_TMP2 )
ENDIF (NOT HAVE_SYS_SYSCTL_H)




check_include_file(sys/resource.h 	HAVE_SYS_RESOURCE_H)
# OpenBSD fails with sys/resource.h
IF ( NOT HAVE_SYS_RESOURCE_H )
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/time.h>\n#include <sys/resource.h>\nint main(void) { return 0; };" HAVE_SYS_RESOURCE_H_TMP)
    IF ( HAVE_SYS_RESOURCE_H_TMP )
	SET(HAVE_SYS_RESOURCE_H 1)
    ENDIF ( HAVE_SYS_RESOURCE_H_TMP )
ENDIF ( NOT HAVE_SYS_RESOURCE_H )




check_include_file(net/if.h  		HAVE_NET_IF_H)
check_include_file(net/if_mib.h  	HAVE_NET_IF_MIB_H)
check_include_file(net/if_arp.h 	HAVE_NET_IF_ARP_H)

IF (NOT HAVE_NET_IF_H)
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\n#include <net/if.h>\nint main(void) { return 0; };" HAVE_NET_IF_H_TMP)
    IF ( HAVE_NET_IF_H_TMP )
	SET(HAVE_NET_IF_H 1)
    ENDIF ( HAVE_NET_IF_H_TMP )
ENDIF (NOT HAVE_NET_IF_H)

IF ( NOT HAVE_NET_IF_MIB_H )
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\n#include <net/if.h>\n#include <net/if_mib.h>\nint main(void) { return 0; };" HAVE_NET_IF_MIB_H_TMP)
    IF ( HAVE_NET_IF_MIB_H_TMP )
	SET(HAVE_NET_IF_MIB_H 1)
    ENDIF ( HAVE_NET_IF_MIB_H_TMP )
ENDIF ( NOT HAVE_NET_IF_MIB_H )

IF ( NOT HAVE_NET_IF_ARP_H )
    CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\n#include <net/if.h>\n#include <net/if_arp.h>\nint main(void) { return 0; };" HAVE_NET_IF_ARP_H_TMP)
    IF ( HAVE_NET_IF_ARP_H_TMP )
	SET(HAVE_NET_IF_ARP_H 1)
    ENDIF ( HAVE_NET_IF_ARP_H_TMP )
ENDIF ( NOT HAVE_NET_IF_ARP_H )


CHECK_C_SOURCE_COMPILES("#include <time.h>\nint main(void) { struct tm t; localtime_r(0, &t); return 0; };" HAVE_LOCALTIME_R)
CHECK_C_SOURCE_COMPILES("#include <time.h>\nint main(void) { struct tm t; gmtime_r(0, &t); return 0; };" HAVE_GMTIME_R)

CHECK_C_SOURCE_COMPILES("#include <time.h>\nint main(void) { struct tm t; t.tm_gmtoff = 0; return 0; };" HAVE_STRUCT_TM_TM_GMTOFF)
CHECK_C_SOURCE_COMPILES("#include <time.h>\nint main(void) { struct tm t; t.__tm_gmtoff = 0; return 0; };" HAVE_STRUCT_TM___TM_GMTOFF)
CHECK_C_SOURCE_COMPILES("#include <time.h>\nint main(void) { struct tm t; t.tm_zone = \"\"; return 0; };" HAVE_STRUCT_TM_TM_ZONE)

CHECK_C_SOURCE_COMPILES("#include <sys/socket.h>\nint main(void) { int foo = AF_INET6; return 0; };" HAVE_AF_INET6)
CHECK_C_SOURCE_COMPILES("#include <sys/socket.h>\n#include <netinet/in.h>\nint main(void) { int foo = PF_INET6; return 0; };" HAVE_PF_INET6)

CHECK_C_SOURCE_COMPILES("#include <sys/socket.h>\n#include <netinet/in.h>\nint main(void) { struct sockaddr_in6 foo; return 0; };" HAVE_STRUCT_SOCKADDR_IN6)

CHECK_C_SOURCE_COMPILES("#include <sys/uio.h>\nint main(void) { struct iovec foo; return 0; };" HAVE_STRUCT_IOVEC)

CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\n#include <netdb.h>\nint main(void) { struct addrinfo foo; return 0; };" HAVE_STRUCT_ADDRINFO)

CHECK_C_SOURCE_COMPILES("#include <sys/types.h>\n#include <sys/socket.h>\nint main(void) { long b=0; int sockfd=2; setsockopt(sockfd, SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b)) return 0; };" HAVE_SETSOCKOPT_SO_NONBLOCK)

CHECK_C_SOURCE_COMPILES("#include <unistd.h>\n#include <fcntl.h>\nint main(void) { int sockfd; fcntl(sockfd, F_SETFL, O_NONBLOCK); return 0; };" HAVE_FCNTL_O_NONBLOCK)
CHECK_C_SOURCE_COMPILES("#include <sys/signal.h>\nint main(void) { sig_atomic_t foo; return 0; };" HAVE_SIG_ATOMIC_T)

