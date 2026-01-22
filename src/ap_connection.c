#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MAX_AP_CONNECTION 10
#define MAX_AP_CONNECTION_EXPIRE_TIME 600

#define AP_CONNECTION_RULE_NAT_CHAIN "apconnection_nat_chain"
#define AP_CONNECTION_RULE_FILTER_CHAIN "apconnection_filter_chain"

#define AP_CONNECTION_START_PORT_NUM 8081

pthread_mutex_t ap_connection_mutex;

typedef struct ap_connection_info{
    char ip_addr[16];
    unsigned int port;
    unsigned int time; 
}ap_connection_info_t;

struct ap_connection_info ap_connection_info_list[MAX_AP_CONNECTION];

static int create_nat_rule(const char *ipaddress, unsigned int port,  int isTcp)
{
    char rule[256];

    memset(rule, 0, 256);

    sprintf(rule,
            "iptables -t nat -A %s -i nas+ -p %s --dport %d -j DNAT --to %s:8080 -m comment --comment \"AP CONNECTION\"",
            AP_CONNECTION_RULE_NAT_CHAIN, isTcp ? "tcp" : "udp", port, ipaddress);
    system(rule);

    return 0;
}

static int create_filter_rule(const char *ipaddress, int isTcp)
{
    char rule[256];

    memset(rule, 0, 256);

    sprintf(rule,
            "iptables -I %s 1 -i nas+ -p %s -d %s --dport 8080 -j ACCEPT -m comment --comment \"AP CONNECTION\"",
            AP_CONNECTION_RULE_FILTER_CHAIN, isTcp ? "tcp" : "udp", ipaddress);

    system(rule);

    return 0;

}

static int delete_nat_rule(const char *ipaddress, unsigned int port,  int isTcp)
{
    char rule[256];

    memset(rule, 0, 256);

    sprintf(rule,
            "iptables -t nat -D %s -i nas+ -p %s --dport %d -j DNAT --to %s:8080 -m comment --comment \"AP CONNECTION\"",
           AP_CONNECTION_RULE_NAT_CHAIN , isTcp ? "tcp" : "udp", port, ipaddress);
    system(rule);
	printf("%s.%s\n",__func__, rule);

    return 0;
}

static int delete_filter_rule(const char *ipaddress, int isTcp)
{
    char rule[256];

    memset(rule, 0, 256);

    sprintf(rule,
            "iptables -D %s -i nas+ -p %s -d %s --dport 8080 -j ACCEPT -m comment --comment \"AP CONNECTION\"",
            AP_CONNECTION_RULE_FILTER_CHAIN, isTcp ? "tcp" : "udp", ipaddress);

    system(rule);
	printf("%s.%s\n",__func__, rule);

    return 0;

}

int add_ap_connection(const char *ip_addr) 
{
    int i;

    pthread_mutex_lock(&ap_connection_mutex);


    for(i = 0; i < MAX_AP_CONNECTION; i++) 
    {
        if(ap_connection_info_list[i].time != 0 &&
                strcmp(ap_connection_info_list[i].ip_addr, ip_addr) == 0) 
        {
            ap_connection_info_list[i].time = time(NULL);
            pthread_mutex_unlock(&ap_connection_mutex);
            return ap_connection_info_list[i].port;
        }
    }

    for(i = 0; i < MAX_AP_CONNECTION; i++) 
    {
        if(ap_connection_info_list[i].time == 0) 
        {
            strcpy(ap_connection_info_list[i].ip_addr, ip_addr);
            ap_connection_info_list[i].time = time(NULL);

            pthread_mutex_unlock(&ap_connection_mutex);
			/*addiptables*/
            create_nat_rule(ip_addr, ap_connection_info_list[i].port, 0);
            create_nat_rule(ip_addr, ap_connection_info_list[i].port, 1);
            create_filter_rule(ip_addr, 0);
            create_filter_rule(ip_addr, 1);
            return ap_connection_info_list[i].port;
        }
    }
    pthread_mutex_unlock(&ap_connection_mutex);
    return 0; 
}

static void init_iptables(void)
{
    /* remove */
    system("iptables -D FORWARD -j "AP_CONNECTION_RULE_FILTER_CHAIN);
    system("iptables -F "AP_CONNECTION_RULE_FILTER_CHAIN);
    system("iptables -X "AP_CONNECTION_RULE_FILTER_CHAIN);
    system("iptables -t nat -D PREROUTING -j "AP_CONNECTION_RULE_NAT_CHAIN);
    system("iptables -t nat -F "AP_CONNECTION_RULE_NAT_CHAIN);
    system("iptables -t nat -X "AP_CONNECTION_RULE_NAT_CHAIN);

    /* create */
    system("iptables -N "AP_CONNECTION_RULE_FILTER_CHAIN);
    system("iptables -I FORWARD -j "AP_CONNECTION_RULE_FILTER_CHAIN);
    system("iptables -t nat -N "AP_CONNECTION_RULE_NAT_CHAIN);
    system("iptables -t nat -I PREROUTING -j "AP_CONNECTION_RULE_NAT_CHAIN);
}


void init_ap_connection_info_list(void)
{
    int i;

    init_iptables();
    pthread_mutex_lock(&ap_connection_mutex);
    for(i = 0; i < MAX_AP_CONNECTION; i++)
    {
        ap_connection_info_list[i].ip_addr[0] = '\0';
        ap_connection_info_list[i].port = AP_CONNECTION_START_PORT_NUM + i;
        ap_connection_info_list[i].time = 0;
    }
    pthread_mutex_unlock(&ap_connection_mutex);
}

void *monitor_ap_connection(void *arg)
{
    time_t cur;
    int i = 0;

    pthread_mutex_init(&ap_connection_mutex, NULL);

    init_ap_connection_info_list();

    while(1)
    {
        cur = time(NULL);

        pthread_mutex_lock(&ap_connection_mutex);
        for(i = 0; i < MAX_AP_CONNECTION; i++)
        {
            if(ap_connection_info_list[i].time != 0 && difftime(cur, ap_connection_info_list[i].time) > MAX_AP_CONNECTION_EXPIRE_TIME)
            {
				/* delete iptables */
                delete_nat_rule(ap_connection_info_list[i].ip_addr, ap_connection_info_list[i].port, 0);
                delete_nat_rule(ap_connection_info_list[i].ip_addr, ap_connection_info_list[i].port, 1);
                delete_filter_rule(ap_connection_info_list[i].ip_addr, 0);
                delete_filter_rule(ap_connection_info_list[i].ip_addr, 1);

                ap_connection_info_list[i].ip_addr[0] = '\0';
                ap_connection_info_list[i].time = 0;
            }
        }
        pthread_mutex_unlock(&ap_connection_mutex);
        sleep(10);
    }

    pthread_mutex_destroy(&ap_connection_mutex);
    return NULL;
}

int init_ap_connection_thread(void)
{
    pthread_t monitor_ap_connection_tid;

    pthread_create(&monitor_ap_connection_tid, NULL, monitor_ap_connection, NULL);
    pthread_detach(monitor_ap_connection_tid);

    return 0;
}
