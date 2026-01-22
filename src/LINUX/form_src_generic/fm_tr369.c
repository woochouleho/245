/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "options.h"

typedef struct
{
	/* Common */
	char tr369_target[32];
	int tr369_action;

	/* LocalAgent MTP */
	int localagent_mtp_instnum;
	int localagent_mtp_enable;
	char localagent_mtp_protocol[16];
	char localagent_mtp_stomp_reference[256];
	char localagent_mtp_stomp_destination[256];
	int localagent_mtp_coap_encryption;
	char localagent_mtp_coap_path[64];
	int localagent_mtp_coap_port;
	char localagent_mtp_mqtt_reference[256];
	char localagent_mtp_mqtt_response_topic[256];
	int localagent_mtp_mqtt_publishqos;

	/* LocalAgent Controller */
	int localagent_controller_instnum;
	int localagent_controller_enable;
	char localagent_controller_endpointid[64];
	int localagent_controller_periodicnotifinterval;
	int localagent_controller_mtp_instnum;
	int localagent_controller_mtp_enable;
	char localagent_controller_mtp_protocol[16];
	char localagent_controller_mtp_stomp_reference[256];
	char localagent_controller_mtp_stomp_destination[256];
	int localagent_controller_mtp_coap_encryption;
	char localagent_controller_mtp_coap_host[256];
	int localagent_controller_mtp_coap_port;
	char localagent_controller_mtp_coap_path[64];
	char localagent_controller_mtp_websocket_host[256];
	int localagent_controller_mtp_websocket_port;
	char localagent_controller_mtp_websocket_path[64];
	int localagent_controller_mtp_websocket_encryption;
	int localagent_controller_mtp_websocket_keepaliveinterval;
	char localagent_controller_mtp_mqtt_reference[256];
	char localagent_controller_mtp_mqtt_topic[256];

	/* STOMP Connection */
	int stomp_connection_instnum;
	int stomp_connection_enable;
	char stomp_connection_host[256];
	int stomp_connection_port;
	char stomp_connection_username[256];
	char stomp_connection_password[256];
	int stomp_connection_heartbeat;
	int stomp_connection_outgoingheartbeat;
	int stomp_connection_incomingheartbeat;
	int stomp_connection_encryption;

	/* MQTT Client */
	int mqtt_client_instnum;
	int mqtt_client_enable;
	char mqtt_client_brokeraddress[256];
	int mqtt_client_brokerport;
	char mqtt_client_username[256];
	char mqtt_client_password[256];
	char mqtt_client_clientid[256];
	int mqtt_client_keepalivetime;
	int mqtt_client_protocol_version;
	int mqtt_client_transport_protocol;

	/* MQTT Client Subscription */
	int mqtt_client_subscription_instnum;
	int mqtt_client_subscription_enable;
	char mqtt_client_subscription_topic[256];
	int mqtt_client_subscription_qos;
} WEB_TR369_PRMT_T;

int formTR369_prmt_get(request *wp, WEB_TR369_PRMT_T *prmt)
{
	char *str = NULL;

	if (wp == NULL || prmt == NULL)
		return -1;

	/* Common */
	str = boaGetVar(wp, "tr369_target", "");
	if (str && str[0]) snprintf(prmt->tr369_target, sizeof(prmt->tr369_target), "%s", str);
	fprintf(stderr, "tr369_target = %s\n", prmt->tr369_target);

	str = boaGetVar(wp, "tr369_action", "");
	if (str && str[0]) prmt->tr369_action = atoi(str);
	fprintf(stderr, "tr369_action = %d\n", prmt->tr369_action);

	/* LocalAgent MTP */
	str = boaGetVar(wp, "localagent_mtp_instnum", "");
	if (str && str[0]) prmt->localagent_mtp_instnum = atoi(str);
	fprintf(stderr, "localagent_mtp_instnum = %d\n", prmt->localagent_mtp_instnum);

	str = boaGetVar(wp, "localagent_mtp_enable", "");
	if (str && str[0]) prmt->localagent_mtp_enable = atoi(str);
	fprintf(stderr, "localagent_mtp_enable = %d\n",  prmt->localagent_mtp_enable);

	str = boaGetVar(wp, "localagent_mtp_protocol", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_protocol, sizeof(prmt->localagent_mtp_protocol), "%s", str);
	fprintf(stderr, "localagent_mtp_protocol = %s\n", prmt->localagent_mtp_protocol);

	str = boaGetVar(wp, "localagent_mtp_stomp_reference", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_stomp_reference, sizeof(prmt->localagent_mtp_stomp_reference), "%s", str);
	fprintf(stderr, "localagent_mtp_stomp_reference = %s\n", prmt->localagent_mtp_stomp_reference);

	str = boaGetVar(wp, "localagent_mtp_stomp_destination", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_stomp_destination, sizeof(prmt->localagent_mtp_stomp_destination), "%s", str);
	fprintf(stderr, "localagent_mtp_stomp_destination = %s\n", prmt->localagent_mtp_stomp_destination);

	str = boaGetVar(wp, "localagent_mtp_coap_encryption", "");
	if (str && str[0]) prmt->localagent_mtp_coap_encryption = atoi(str);
	fprintf(stderr, "localagent_mtp_coap_encryption = %d\n", prmt->localagent_mtp_coap_encryption);

	str = boaGetVar(wp, "localagent_mtp_coap_path", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_coap_path, sizeof(prmt->localagent_mtp_coap_path), "%s", str);
	fprintf(stderr, "localagent_mtp_coap_path = %s\n", prmt->localagent_mtp_coap_path);

	str = boaGetVar(wp, "localagent_mtp_coap_port", "");
	if (str && str[0]) prmt->localagent_mtp_coap_port = atoi(str);
	fprintf(stderr, "localagent_mtp_coap_port = %d\n", prmt->localagent_mtp_coap_port);

	str = boaGetVar(wp, "localagent_mtp_mqtt_reference", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_mqtt_reference, sizeof(prmt->localagent_mtp_mqtt_reference), "%s", str);
	fprintf(stderr, "localagent_mtp_mqtt_reference = %s\n", prmt->localagent_mtp_mqtt_reference);

	str = boaGetVar(wp, "localagent_mtp_mqtt_response_topic", "");
	if (str && str[0]) snprintf(prmt->localagent_mtp_mqtt_response_topic, sizeof(prmt->localagent_mtp_mqtt_response_topic), "%s", str);
	fprintf(stderr, "localagent_mtp_mqtt_response_topic = %s\n", prmt->localagent_mtp_mqtt_response_topic);

	str = boaGetVar(wp, "localagent_mtp_mqtt_publishqos", "");
	if (str && str[0]) prmt->localagent_mtp_mqtt_publishqos = atoi(str);
	fprintf(stderr, "localagent_mtp_mqtt_publishqos = %d\n", prmt->localagent_mtp_mqtt_publishqos);

	/* LocalAgent Controller */
	str = boaGetVar(wp, "localagent_controller_instnum", "");
	if (str && str[0]) prmt->localagent_controller_instnum = atoi(str);
	fprintf(stderr, "localagent_controller_instnum = %d\n", prmt->localagent_controller_instnum);

	str = boaGetVar(wp, "localagent_controller_enable", "");
	if (str && str[0]) prmt->localagent_controller_enable = atoi(str);
	fprintf(stderr, "localagent_controller_enable = %d\n", prmt->localagent_controller_enable);

	str = boaGetVar(wp, "localagent_controller_endpointid", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_endpointid, sizeof(prmt->localagent_controller_endpointid), "%s", str);
	fprintf(stderr, "localagent_controller_endpointid = %s\n", prmt->localagent_controller_endpointid);

	str = boaGetVar(wp, "localagent_controller_periodicnotifinterval", "");
	if (str && str[0]) prmt->localagent_controller_periodicnotifinterval = atoi(str);
	fprintf(stderr, "localagent_controller_periodicnotifinterval = %d\n", prmt->localagent_controller_periodicnotifinterval);

	str = boaGetVar(wp, "localagent_controller_mtp_instnum", "");
	if (str && str[0]) prmt->localagent_controller_mtp_instnum = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_instnum = %d\n", prmt->localagent_controller_mtp_instnum);

	str = boaGetVar(wp, "localagent_controller_mtp_enable", "");
	if (str && str[0]) prmt->localagent_controller_mtp_enable = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_enable = %d\n", prmt->localagent_controller_mtp_enable);

	str = boaGetVar(wp, "localagent_controller_mtp_protocol", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_protocol, sizeof(prmt->localagent_controller_mtp_protocol), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_protocol = %s\n", prmt->localagent_controller_mtp_protocol);

	str = boaGetVar(wp, "localagent_controller_mtp_stomp_reference", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_stomp_reference, sizeof(prmt->localagent_controller_mtp_stomp_reference), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_stomp_reference = %s\n", prmt->localagent_controller_mtp_stomp_reference);

	str = boaGetVar(wp, "localagent_controller_mtp_stomp_destination", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_stomp_destination, sizeof(prmt->localagent_controller_mtp_stomp_destination), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_stomp_destination = %s\n", prmt->localagent_controller_mtp_stomp_destination);

	str = boaGetVar(wp, "localagent_controller_mtp_coap_encryption", "");
	if (str && str[0]) prmt->localagent_controller_mtp_coap_encryption = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_coap_encryption = %d\n", prmt->localagent_controller_mtp_coap_encryption);

	str = boaGetVar(wp, "localagent_controller_mtp_coap_host", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_coap_host, sizeof(prmt->localagent_controller_mtp_coap_host), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_coap_host = %s\n", prmt->localagent_controller_mtp_coap_host);

	str = boaGetVar(wp, "localagent_controller_mtp_coap_port", "");
	if (str && str[0]) prmt->localagent_controller_mtp_coap_port = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_coap_port = %d\n", prmt->localagent_controller_mtp_coap_port);

	str = boaGetVar(wp, "localagent_controller_mtp_coap_path", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_coap_path, sizeof(prmt->localagent_controller_mtp_coap_path), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_coap_path = %s\n", prmt->localagent_controller_mtp_coap_path);

	str = boaGetVar(wp, "localagent_controller_mtp_websocket_host", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_websocket_host, sizeof(prmt->localagent_controller_mtp_websocket_host), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_websocket_host = %s\n", prmt->localagent_controller_mtp_websocket_host);

	str = boaGetVar(wp, "localagent_controller_mtp_websocket_port", "");
	if (str && str[0]) prmt->localagent_controller_mtp_websocket_port = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_websocket_port = %d\n", prmt->localagent_controller_mtp_websocket_port);

	str = boaGetVar(wp, "localagent_controller_mtp_websocket_path", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_websocket_path, sizeof(prmt->localagent_controller_mtp_websocket_path), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_websocket_path = %s\n", prmt->localagent_controller_mtp_websocket_path);

	str = boaGetVar(wp, "localagent_controller_mtp_websocket_encryption", "");
	if (str && str[0]) prmt->localagent_controller_mtp_websocket_encryption = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_websocket_encryption = %d\n", prmt->localagent_controller_mtp_websocket_encryption);

	str = boaGetVar(wp, "localagent_controller_mtp_websocket_keepaliveinterval", "");
	if (str && str[0]) prmt->localagent_controller_mtp_websocket_keepaliveinterval = atoi(str);
	fprintf(stderr, "localagent_controller_mtp_websocket_keepaliveinterval = %d\n", prmt->localagent_controller_mtp_websocket_keepaliveinterval);

	str = boaGetVar(wp, "localagent_controller_mtp_mqtt_reference", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_mqtt_reference, sizeof(prmt->localagent_controller_mtp_mqtt_reference), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_mqtt_reference = %s\n", prmt->localagent_controller_mtp_mqtt_reference);

	str = boaGetVar(wp, "localagent_controller_mtp_mqtt_topic", "");
	if (str && str[0]) snprintf(prmt->localagent_controller_mtp_mqtt_topic, sizeof(prmt->localagent_controller_mtp_mqtt_topic), "%s", str);
	fprintf(stderr, "localagent_controller_mtp_mqtt_topic = %s\n", prmt->localagent_controller_mtp_mqtt_topic);

	/* STOMP Connection */
	str = boaGetVar(wp, "stomp_connection_instnum", "");
	if (str && str[0]) prmt->stomp_connection_instnum = atoi(str);
	fprintf(stderr, "stomp_connection_instnum = %d\n", prmt->stomp_connection_instnum);

	str = boaGetVar(wp, "stomp_connection_enable", "");
	if (str && str[0]) prmt->stomp_connection_enable = atoi(str);
	fprintf(stderr, "stomp_connection_enable = %d\n", prmt->stomp_connection_enable);

	str = boaGetVar(wp, "stomp_connection_host", "");
	if (str && str[0]) snprintf(prmt->stomp_connection_host, sizeof(prmt->stomp_connection_host), "%s", str);
	fprintf(stderr, "stomp_connection_host = %s\n", prmt->stomp_connection_host);

	str = boaGetVar(wp, "stomp_connection_port", "");
	if (str && str[0]) prmt->stomp_connection_port = atoi(str);
	fprintf(stderr, "stomp_connection_port = %d\n", prmt->stomp_connection_port);

	str = boaGetVar(wp, "stomp_connection_username", "");
	if (str && str[0]) snprintf(prmt->stomp_connection_username, sizeof(prmt->stomp_connection_username), "%s", str);
	fprintf(stderr, "stomp_connection_username = %s\n", prmt->stomp_connection_username);

	str = boaGetVar(wp, "stomp_connection_password", "");
	if (str && str[0]) snprintf(prmt->stomp_connection_password, sizeof(prmt->stomp_connection_password), "%s", str);
	fprintf(stderr, "stomp_connection_password = %s\n", prmt->stomp_connection_password);

	str = boaGetVar(wp, "stomp_connection_heartbeat", "");
	if (str && str[0]) prmt->stomp_connection_heartbeat = atoi(str);
	fprintf(stderr, "stomp_connection_heartbeat = %d\n", prmt->stomp_connection_heartbeat);

	str = boaGetVar(wp, "stomp_connection_outgoingheartbeat", "");
	if (str && str[0]) prmt->stomp_connection_outgoingheartbeat = atoi(str);
	fprintf(stderr, "stomp_connection_outgoingheartbeat = %d\n", prmt->stomp_connection_outgoingheartbeat);

	str = boaGetVar(wp, "stomp_connection_incomingheartbeat", "");
	if (str && str[0]) prmt->stomp_connection_incomingheartbeat = atoi(str);
	fprintf(stderr, "stomp_connection_incomingheartbeat = %d\n", prmt->stomp_connection_incomingheartbeat);

	str = boaGetVar(wp, "stomp_connection_encryption", "");
	if (str && str[0]) prmt->stomp_connection_encryption = atoi(str);
	fprintf(stderr, "stomp_connection_encryption = %d\n", prmt->stomp_connection_encryption);

	/* MQTT Client */
	str = boaGetVar(wp, "mqtt_client_instnum", "");
	if (str && str[0]) prmt->mqtt_client_instnum = atoi(str);
	fprintf(stderr, "mqtt_client_instnum = %d\n", prmt->mqtt_client_instnum);

	str = boaGetVar(wp, "mqtt_client_enable", "");
	if (str && str[0]) prmt->mqtt_client_enable = atoi(str);
	fprintf(stderr, "mqtt_client_enable = %d\n", prmt->mqtt_client_enable);

	str = boaGetVar(wp, "mqtt_client_brokeraddress", "");
	if (str && str[0]) snprintf(prmt->mqtt_client_brokeraddress, sizeof(prmt->mqtt_client_brokeraddress), "%s", str);
	fprintf(stderr, "mqtt_client_brokeraddress = %s\n", prmt->mqtt_client_brokeraddress);

	str = boaGetVar(wp, "mqtt_client_brokerport", "");
	if (str && str[0]) prmt->mqtt_client_brokerport = atoi(str);
	fprintf(stderr, "mqtt_client_brokerport = %d\n", prmt->mqtt_client_brokerport);

	str = boaGetVar(wp, "mqtt_client_username", "");
	if (str && str[0]) snprintf(prmt->mqtt_client_username, sizeof(prmt->mqtt_client_username), "%s", str);
	fprintf(stderr, "mqtt_client_username = %s\n", prmt->mqtt_client_username);

	str = boaGetVar(wp, "mqtt_client_password", "");
	if (str && str[0]) snprintf(prmt->mqtt_client_password, sizeof(prmt->mqtt_client_password), "%s", str);
	fprintf(stderr, "mqtt_client_password = %s\n", prmt->mqtt_client_password);

	str = boaGetVar(wp, "mqtt_client_clientid", "");
	if (str && str[0]) snprintf(prmt->mqtt_client_clientid, sizeof(prmt->mqtt_client_clientid), "%s", str);
	fprintf(stderr, "mqtt_client_clientid = %s\n", prmt->mqtt_client_clientid);

	str = boaGetVar(wp, "mqtt_client_keepalivetime", "");
	if (str && str[0]) prmt->mqtt_client_keepalivetime = atoi(str);
	fprintf(stderr, "mqtt_client_keepalivetime = %d\n", prmt->mqtt_client_keepalivetime);

	str = boaGetVar(wp, "mqtt_client_protocol_version", "");
	if (str && str[0]) prmt->mqtt_client_protocol_version = atoi(str);
	fprintf(stderr, "mqtt_client_protocol_version = %d\n", prmt->mqtt_client_protocol_version);

	str = boaGetVar(wp, "mqtt_client_transport_protocol", "");
	if (str && str[0]) prmt->mqtt_client_transport_protocol = atoi(str);
	fprintf(stderr, "mqtt_client_transport_protocol = %d\n", prmt->mqtt_client_transport_protocol);

	/* MQTT Client Subscription */
	str = boaGetVar(wp, "mqtt_client_subscription_instnum", "");
	if (str && str[0]) prmt->mqtt_client_subscription_instnum = atoi(str);
	fprintf(stderr, "mqtt_client_subscription_instnum = %d\n", prmt->mqtt_client_subscription_instnum);

	str = boaGetVar(wp, "mqtt_client_subscription_enable", "");
	if (str && str[0]) prmt->mqtt_client_subscription_enable = atoi(str);
	fprintf(stderr, "mqtt_client_subscription_enable = %d\n", prmt->mqtt_client_subscription_enable);

	str = boaGetVar(wp, "mqtt_client_subscription_topic", "");
	if (str && str[0]) snprintf(prmt->mqtt_client_subscription_topic, sizeof(prmt->mqtt_client_subscription_topic), "%s", str);
	fprintf(stderr, "mqtt_client_subscription_topic = %s\n", prmt->mqtt_client_subscription_topic);

	str = boaGetVar(wp, "mqtt_client_subscription_qos", "");
	if (str && str[0]) prmt->mqtt_client_subscription_qos = atoi(str);
	fprintf(stderr, "mqtt_client_subscription_qos = %d\n", prmt->mqtt_client_subscription_qos);

	return 0;
}

int formTR369_prmt_set(int mib_id, void *p, WEB_TR369_PRMT_T *prmt)
{
	if (p == NULL || prmt == NULL)
		return -1;

	if (mib_id == MIB_TR369_LOCALAGENT_MTP_TBL)
	{
		MIB_TR369_LOCALAGENT_MTP_T *p_entry = (MIB_TR369_LOCALAGENT_MTP_T *)p;

		p_entry->Enable = (prmt->localagent_mtp_enable == 0) ? 0:1;
		snprintf(p_entry->Protocol, sizeof(p_entry->Protocol), "%s", prmt->localagent_mtp_protocol);
		if (strcmp(prmt->localagent_mtp_protocol, "STOMP") == 0)
		{
			snprintf(p_entry->STOMP_Reference, sizeof(p_entry->STOMP_Reference), "%s", prmt->localagent_mtp_stomp_reference);
			snprintf(p_entry->STOMP_Destination, sizeof(p_entry->STOMP_Destination), "%s", prmt->localagent_mtp_stomp_destination);
		}
		else if (strcmp(prmt->localagent_mtp_protocol, "CoAP") == 0)
		{
			p_entry->CoAP_Port = prmt->localagent_mtp_coap_port;
			snprintf(p_entry->CoAP_Path, sizeof(p_entry->CoAP_Path), "%s", prmt->localagent_mtp_coap_path);
			p_entry->CoAP_EnableEncryption = (prmt->localagent_mtp_coap_encryption == 0) ? 0:1;
		}
		else if (strcmp(prmt->localagent_mtp_protocol, "MQTT") == 0)
		{
			snprintf(p_entry->MQTT_Reference, sizeof(p_entry->MQTT_Reference), "%s", prmt->localagent_mtp_mqtt_reference);
			snprintf(p_entry->MQTT_ResponseTopicConfigured, sizeof(p_entry->MQTT_ResponseTopicConfigured), "%s", prmt->localagent_mtp_mqtt_response_topic);
			p_entry->MQTT_PublishQoS = prmt->localagent_mtp_mqtt_publishqos;
		}
	}
	else if (mib_id == MIB_TR369_LOCALAGENT_CONTROLLER_TBL)
	{
		MIB_TR369_LOCALAGENT_CONTROLLER_T *p_entry = (MIB_TR369_LOCALAGENT_CONTROLLER_T *)p;

		p_entry->Enable = (prmt->localagent_controller_enable == 0) ? 0:1;
		snprintf(p_entry->EndpointID, sizeof(p_entry->EndpointID), "%s", prmt->localagent_controller_endpointid);
		p_entry->PeriodicNotifInterval = prmt->localagent_controller_periodicnotifinterval;
	}
	else if (mib_id == MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL)
	{
		MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T *p_entry = (MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T *)p;

		p_entry->Enable = (prmt->localagent_controller_mtp_enable == 0) ? 0:1;
		snprintf(p_entry->Protocol, sizeof(p_entry->Protocol), "%s", prmt->localagent_controller_mtp_protocol);

		if (strcmp(prmt->localagent_controller_mtp_protocol, "STOMP") == 0)
		{
			snprintf(p_entry->STOMP_Reference, sizeof(p_entry->STOMP_Reference), "%s", prmt->localagent_controller_mtp_stomp_reference);
			snprintf(p_entry->STOMP_Destination, sizeof(p_entry->STOMP_Destination), "%s", prmt->localagent_controller_mtp_stomp_destination);
		}
		else if (strcmp(prmt->localagent_controller_mtp_protocol, "CoAP") == 0)
		{
			snprintf(p_entry->CoAP_Host, sizeof(p_entry->CoAP_Host), "%s", prmt->localagent_controller_mtp_coap_host);
			p_entry->CoAP_Port = prmt->localagent_controller_mtp_coap_port;
			snprintf(p_entry->CoAP_Path, sizeof(p_entry->CoAP_Path), "%s", prmt->localagent_controller_mtp_coap_path);
			p_entry->CoAP_EnableEncryption = (prmt->localagent_controller_mtp_coap_encryption == 0) ? 0:1;
		}
		else if (strcmp(prmt->localagent_controller_mtp_protocol, "WebSocket") == 0)
		{
			snprintf(p_entry->WebSocket_Host, sizeof(p_entry->WebSocket_Host), "%s", prmt->localagent_controller_mtp_websocket_host);
			p_entry->WebSocket_Port = prmt->localagent_controller_mtp_websocket_port;
			snprintf(p_entry->WebSocket_Path, sizeof(p_entry->WebSocket_Path), "%s", prmt->localagent_controller_mtp_websocket_path);
			p_entry->WebSocket_EnableEncryption = (prmt->localagent_controller_mtp_websocket_encryption == 0) ? 0:1;
			p_entry->WebSocket_KeepAliveInterval = prmt->localagent_controller_mtp_websocket_keepaliveinterval;
		}
		else if (strcmp(prmt->localagent_controller_mtp_protocol, "MQTT") == 0)
		{
			snprintf(p_entry->MQTT_Reference, sizeof(p_entry->MQTT_Reference), "%s", prmt->localagent_controller_mtp_mqtt_reference);
			snprintf(p_entry->MQTT_Topic, sizeof(p_entry->MQTT_Topic), "%s", prmt->localagent_controller_mtp_mqtt_topic);
		}
	}
	else if (mib_id == MIB_TR369_STOMP_CONNECTION_TBL)
	{
		MIB_TR369_STOMP_CONNECTION_T *p_entry = (MIB_TR369_STOMP_CONNECTION_T *)p;

		p_entry->Enable = (prmt->stomp_connection_enable == 0) ? 0:1;
		snprintf(p_entry->Host, sizeof(p_entry->Host), "%s", prmt->stomp_connection_host);
		p_entry->Port = prmt->stomp_connection_port;
		snprintf(p_entry->Username, sizeof(p_entry->Username), "%s", prmt->stomp_connection_username);
		snprintf(p_entry->Password, sizeof(p_entry->Password), "%s", prmt->stomp_connection_password);
		p_entry->EnableHeartbeats = (prmt->stomp_connection_heartbeat == 0) ? 0:1;
		p_entry->OutgoingHeartbeat = prmt->stomp_connection_outgoingheartbeat;
		p_entry->IncomingHeartbeat = prmt->stomp_connection_incomingheartbeat;
		p_entry->EnableEncryption = (prmt->stomp_connection_encryption == 0) ? 0:1;
	}
	else if (mib_id == MIB_TR369_MQTT_CLIENT_TBL)
	{
		MIB_TR369_MQTT_CLIENT_T *p_entry = (MIB_TR369_MQTT_CLIENT_T *)p;

		p_entry->Enable = (prmt->mqtt_client_enable == 0) ? 0:1;
		snprintf(p_entry->BrokerAddress, sizeof(p_entry->BrokerAddress), "%s", prmt->mqtt_client_brokeraddress);
		p_entry->BrokerPort = prmt->mqtt_client_brokerport;
		snprintf(p_entry->Username, sizeof(p_entry->Username), "%s", prmt->mqtt_client_username);
		snprintf(p_entry->Password, sizeof(p_entry->Password), "%s", prmt->mqtt_client_password);
		snprintf(p_entry->ClientID, sizeof(p_entry->ClientID), "%s", prmt->mqtt_client_clientid);
		p_entry->KeepAliveTime = prmt->mqtt_client_keepalivetime;
		p_entry->ProtocolVersion = prmt->mqtt_client_protocol_version;
		p_entry->TransportProtocol = prmt->mqtt_client_transport_protocol;
	}
	else if (mib_id == MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL)
	{
		MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T *p_entry = (MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T *)p;

		p_entry->Enable = (prmt->mqtt_client_subscription_enable == 0) ? 0:1;
		snprintf(p_entry->Topic, sizeof(p_entry->Topic), "%s", prmt->mqtt_client_subscription_topic);
		p_entry->QoS = prmt->mqtt_client_subscription_qos;
	}

	return 0;
}

void formTR369(request * wp, char *path, char *query)
{
	char *submitUrl;
	char error_msg[128] = {0};
	int i = 0, total = 0, need_update = 0;
	unsigned int chainid = 0, chainid2 = 0;
	WEB_TR369_PRMT_T prmt;
	MIB_TR369_LOCALAGENT_MTP_T mtp_entry;
	MIB_TR369_LOCALAGENT_CONTROLLER_T controller_entry;
	MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T controller_mtp_entry;
	MIB_TR369_STOMP_CONNECTION_T stomp_entry;
	MIB_TR369_MQTT_CLIENT_T mqtt_client_entry;
	MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T mqtt_client_sub_entry;

	fprintf(stderr, "[%s:%s]\n", __FILE__, __FUNCTION__);

	memset(&prmt, 0, sizeof(WEB_TR369_PRMT_T));
	formTR369_prmt_get(wp, &prmt);

	if (strcmp(prmt.tr369_target, "localagent_mtp") == 0)
	{
		if (prmt.tr369_action == 0) /* delete */
		{
			if (rtk_tr369_mtp_get_entry_by_instnum(&mtp_entry, prmt.localagent_mtp_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}
			mib_chain_delete(MIB_TR369_LOCALAGENT_MTP_TBL, chainid);
		}
		else
		{
			if (prmt.localagent_mtp_instnum == -1) /* add */
			{
				memset(&mtp_entry, 0, sizeof(MIB_TR369_LOCALAGENT_MTP_T));
				mtp_entry.instnum = rtk_tr369_mtp_get_new_instnum();
				formTR369_prmt_set(MIB_TR369_LOCALAGENT_MTP_TBL, (void *)&mtp_entry, &prmt);
				mib_chain_add(MIB_TR369_LOCALAGENT_MTP_TBL, (void *)&mtp_entry);
			}
			else /* edit */
			{
				if (rtk_tr369_mtp_get_entry_by_instnum(&mtp_entry, prmt.localagent_mtp_instnum, &chainid) < 0)
				{
					strcpy(error_msg, strGetChainerror);
					goto CONFIG_ERROR;
				}
				formTR369_prmt_set(MIB_TR369_LOCALAGENT_MTP_TBL, (void *)&mtp_entry, &prmt);
				mib_chain_update(MIB_TR369_LOCALAGENT_MTP_TBL, (void *)&mtp_entry, chainid);
			}
		}
	}
	else if (strcmp(prmt.tr369_target, "localagent_controller") == 0)
	{
		if (prmt.tr369_action == 0) /* delete controller */
		{
			if (rtk_tr369_controller_get_entry_by_instnum(&controller_entry, prmt.localagent_controller_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}

			total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (!mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, i, (void *)&controller_mtp_entry))
					continue;

				if (controller_mtp_entry.controller_instnum == controller_entry.instnum)
				{
					mib_chain_delete(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, i);
				}
			}

			mib_chain_delete(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, chainid);
		}
	}
	else if (strcmp(prmt.tr369_target, "localagent_controller_mtp") == 0)
	{
		if (prmt.tr369_action == 0) /* delete controller_mtp */
		{
			if (rtk_tr369_controller_mtp_get_entry_by_instnum(&controller_mtp_entry, prmt.localagent_controller_instnum, prmt.localagent_controller_mtp_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}
			mib_chain_delete(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, chainid);
		}
		else
		{
			if (prmt.localagent_controller_instnum == -1 && prmt.localagent_controller_mtp_instnum == -1) /* add controller and controller_mtp */
			{
				memset(&controller_entry, 0, sizeof(MIB_TR369_LOCALAGENT_CONTROLLER_T));
				memset(&controller_mtp_entry, 0, sizeof(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T));
				controller_entry.instnum = rtk_tr369_controller_get_new_instnum();
				controller_mtp_entry.controller_instnum = controller_entry.instnum;
				controller_mtp_entry.instnum = 1;

				formTR369_prmt_set(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, (void *)&controller_entry, &prmt);
				mib_chain_add(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, (void *)&controller_entry);

				formTR369_prmt_set(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry, &prmt);
				mib_chain_add(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry);
			}
			else /* edit controller */
			{
				if (rtk_tr369_controller_get_entry_by_instnum(&controller_entry, prmt.localagent_controller_instnum, &chainid) < 0)
				{
					strcpy(error_msg, strGetChainerror);
					goto CONFIG_ERROR;
				}
				formTR369_prmt_set(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, (void *)&controller_entry, &prmt);
				mib_chain_update(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, (void *)&controller_entry, chainid);

				if (prmt.localagent_controller_mtp_instnum == -1) /* add controller_mtp */
				{
					memset(&controller_mtp_entry, 0, sizeof(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T));
					controller_mtp_entry.controller_instnum = controller_entry.instnum;
					controller_mtp_entry.instnum = rtk_tr369_controller_mtp_get_new_instnum(controller_entry.instnum);
					formTR369_prmt_set(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry, &prmt);
					mib_chain_add(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry);
				}
				else /* edit controller_mtp */
				{
					if (rtk_tr369_controller_mtp_get_entry_by_instnum(&controller_mtp_entry, prmt.localagent_controller_instnum, prmt.localagent_controller_mtp_instnum, &chainid2) < 0)
					{
						strcpy(error_msg, strGetChainerror);
						goto CONFIG_ERROR;
					}
					formTR369_prmt_set(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry, &prmt);
					mib_chain_update(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, (void *)&controller_mtp_entry, chainid2);
				}
			}
		}
	}
	else if (strcmp(prmt.tr369_target, "stomp_connection") == 0)
	{
		if (prmt.tr369_action == 0) /* delete */
		{
			if (rtk_tr369_stomp_get_entry_by_instnum(&stomp_entry, prmt.stomp_connection_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}
			mib_chain_delete(MIB_TR369_STOMP_CONNECTION_TBL, chainid);
		}
		else
		{
			if (prmt.stomp_connection_instnum == -1) /* add */
			{
				memset(&stomp_entry, 0, sizeof(MIB_TR369_STOMP_CONNECTION_T));
				stomp_entry.instnum = rtk_tr369_stomp_get_new_instnum();
				formTR369_prmt_set(MIB_TR369_STOMP_CONNECTION_TBL, (void *)&stomp_entry, &prmt);
				mib_chain_add(MIB_TR369_STOMP_CONNECTION_TBL, (void *)&stomp_entry);
			}
			else /* edit */
			{
				if (rtk_tr369_stomp_get_entry_by_instnum(&stomp_entry, prmt.stomp_connection_instnum, &chainid) < 0)
				{
					strcpy(error_msg, strGetChainerror);
					goto CONFIG_ERROR;
				}
				formTR369_prmt_set(MIB_TR369_STOMP_CONNECTION_TBL, (void *)&stomp_entry, &prmt);
				mib_chain_update(MIB_TR369_STOMP_CONNECTION_TBL, (void *)&stomp_entry, chainid);
			}
		}
	}
	else if (strcmp(prmt.tr369_target, "mqtt_client") == 0)
	{
		if (prmt.tr369_action == 0) /* delete */
		{
			if (rtk_tr369_mqtt_client_get_entry_by_instnum(&mqtt_client_entry, prmt.mqtt_client_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}

			total = mib_chain_total(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL);
			for (i = (total - 1); i >= 0; i--)
			{
				if (mib_chain_get(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, i, (char *)&mqtt_client_sub_entry) == 0)
					continue;

				if (mqtt_client_sub_entry.client_instnum == mqtt_client_entry.instnum)
				{
					mib_chain_delete(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, i);
				}
			}

			mib_chain_delete(MIB_TR369_MQTT_CLIENT_TBL, chainid);
		}
		else
		{
			if (prmt.mqtt_client_instnum == -1) /* add */
			{
				memset(&mqtt_client_entry, 0, sizeof(MIB_TR369_MQTT_CLIENT_T));
				mqtt_client_entry.instnum = rtk_tr369_mqtt_client_get_new_instnum();
				formTR369_prmt_set(MIB_TR369_MQTT_CLIENT_TBL, (void *)&mqtt_client_entry, &prmt);
				mib_chain_add(MIB_TR369_MQTT_CLIENT_TBL, (void *)&mqtt_client_entry);

				if (prmt.mqtt_client_subscription_instnum == -1) /* add */
				{
					memset(&mqtt_client_sub_entry, 0, sizeof(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T));
					mqtt_client_sub_entry.client_instnum = mqtt_client_entry.instnum;
					mqtt_client_sub_entry.instnum = 1;
					formTR369_prmt_set(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry, &prmt);
					mib_chain_add(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry);
				}
			}
			else /* edit */
			{
				if (rtk_tr369_mqtt_client_get_entry_by_instnum(&mqtt_client_entry, prmt.mqtt_client_instnum, &chainid) < 0)
				{
					strcpy(error_msg, strGetChainerror);
					goto CONFIG_ERROR;
				}
				formTR369_prmt_set(MIB_TR369_MQTT_CLIENT_TBL, (void *)&mqtt_client_entry, &prmt);
				mib_chain_update(MIB_TR369_MQTT_CLIENT_TBL, (void *)&mqtt_client_entry, chainid);

				if (prmt.mqtt_client_subscription_instnum == -2) /* ignore */
				{
				}
				else if (prmt.mqtt_client_subscription_instnum == -1) /* add */
				{
					memset(&mqtt_client_sub_entry, 0, sizeof(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T));
					mqtt_client_sub_entry.client_instnum = mqtt_client_entry.instnum;
					mqtt_client_sub_entry.instnum = rtk_tr369_mqtt_client_subscription_get_new_instnum(mqtt_client_entry.instnum);
					formTR369_prmt_set(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry, &prmt);
					mib_chain_add(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry);
				}
				else /* edit */
				{
					if (rtk_tr369_mqtt_client_subscription_get_entry_by_instnum(&mqtt_client_sub_entry, prmt.mqtt_client_instnum, prmt.mqtt_client_subscription_instnum, &chainid2) < 0)
					{
						strcpy(error_msg, strGetChainerror);
						goto CONFIG_ERROR;
					}
					formTR369_prmt_set(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry, &prmt);
					mib_chain_update(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, (void *)&mqtt_client_sub_entry, chainid2);
				}
			}
		}
	}
	else if (strcmp(prmt.tr369_target, "mqtt_client_subscription") == 0)
	{
		if (prmt.tr369_action == 0) /* delete */
		{
			if (rtk_tr369_mqtt_client_subscription_get_entry_by_instnum(&mqtt_client_sub_entry, prmt.mqtt_client_instnum, prmt.mqtt_client_subscription_instnum, &chainid) < 0)
			{
				strcpy(error_msg, strGetChainerror);
				goto CONFIG_ERROR;
			}
			mib_chain_delete(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, chainid);
		}
	}
	else
	{
		strcpy(error_msg, strGetChainerror);
		goto CONFIG_ERROR;
	}

#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

	rtk_tr369_restart_obuspa();

	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

CONFIG_ERROR:
	ERR_MSG(error_msg);
	return;
}

int showTR369List(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
	char *name = NULL;

	if (boaArgs(argc, argv, "%s", &name) < 1)
	{
		boaError(wp, 400, (char *)strArgerror);
		return -1;
	}

	if (!strcmp(name, "tr369_localagent_mtp_table"))
	{
		int i = 0, total = 0;
		MIB_TR369_LOCALAGENT_MTP_T mtp_entry;

		nBytesSent += boaWrite(wp, "var LocalAgentMTP_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_Protocol = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentMTP_CoAP_Port = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_CoAP_Path = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_CoAP_EnableEncryption = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentMTP_STOMP_Reference = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_STOMP_Destination = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentMTP_WebSocket_Port = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_WebSocket_Path = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_WebSocket_EnableEncryption = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentMTP_MQTT_Reference = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_MQTT_ResponseTopicConfigured = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentMTP_MQTT_PublishQoS = new Array();\n");

		total = mib_chain_total(MIB_TR369_LOCALAGENT_MTP_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_TR369_LOCALAGENT_MTP_TBL, i, (void *)&mtp_entry))
				continue;

			nBytesSent += boaWrite(wp, "LocalAgentMTP_Index[LocalAgentMTP_Index.length] = \"%u\";\n", mtp_entry.instnum);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_Enable[LocalAgentMTP_Enable.length] = \"%u\";\n", mtp_entry.Enable);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_Protocol[LocalAgentMTP_Protocol.length] = \"%s\";\n", mtp_entry.Protocol);

#if defined(USP_TR369_ENABLE_COAP)
			nBytesSent += boaWrite(wp, "LocalAgentMTP_CoAP_Port[LocalAgentMTP_CoAP_Port.length] = \"%u\";\n", mtp_entry.CoAP_Port);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_CoAP_Path[LocalAgentMTP_CoAP_Path.length] = \"%s\";\n", mtp_entry.CoAP_Path);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_CoAP_EnableEncryption[LocalAgentMTP_CoAP_EnableEncryption.length] = \"%u\";\n", mtp_entry.CoAP_EnableEncryption);
#endif

#if defined(USP_TR369_ENABLE_STOMP)
			nBytesSent += boaWrite(wp, "LocalAgentMTP_STOMP_Reference[LocalAgentMTP_STOMP_Reference.length] = \"%s\";\n", mtp_entry.STOMP_Reference);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_STOMP_Destination[LocalAgentMTP_STOMP_Destination.length] = \"%s\";\n", mtp_entry.STOMP_Destination);
#endif

#if defined(USP_TR369_ENABLE_WEBSOCKET)
			nBytesSent += boaWrite(wp, "LocalAgentMTP_WebSocket_Port[LocalAgentMTP_WebSocket_Port.length] = \"%u\";\n", mtp_entry.WebSocket_Port);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_WebSocket_Path[LocalAgentMTP_WebSocket_Path.length] = \"%s\";\n", mtp_entry.WebSocket_Path);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_WebSocket_EnableEncryption[LocalAgentMTP_WebSocket_EnableEncryption.length] = \"%u\";\n", mtp_entry.WebSocket_EnableEncryption);
#endif

#if defined(USP_TR369_ENABLE_MQTT)
			nBytesSent += boaWrite(wp, "LocalAgentMTP_MQTT_Reference[LocalAgentMTP_MQTT_Reference.length] = \"%s\";\n", mtp_entry.MQTT_Reference);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_MQTT_ResponseTopicConfigured[LocalAgentMTP_MQTT_ResponseTopicConfigured.length] = \"%s\";\n", mtp_entry.MQTT_ResponseTopicConfigured);
			nBytesSent += boaWrite(wp, "LocalAgentMTP_MQTT_PublishQoS[LocalAgentMTP_MQTT_PublishQoS.length] = \"%u\";\n", mtp_entry.MQTT_PublishQoS);
#endif
		}
	}
	else if (!strcmp(name, "tr369_localagent_controller_table"))
	{
		int i = 0, total = 0;
		MIB_TR369_LOCALAGENT_CONTROLLER_T controller_entry;

		nBytesSent += boaWrite(wp, "var LocalAgentController_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_EndpointID = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_PeriodicNotifInterval = new Array();\n");

		total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_TBL);
		for (i = 0; i < total; i++)
		{
			if (mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_TBL, i, (char *)&controller_entry) == 0)
				continue;

			nBytesSent += boaWrite(wp, "LocalAgentController_Index[LocalAgentController_Index.length] = \"%u\";\n", controller_entry.instnum);
			nBytesSent += boaWrite(wp, "LocalAgentController_Enable[LocalAgentController_Enable.length] = \"%u\";\n", controller_entry.Enable);
			nBytesSent += boaWrite(wp, "LocalAgentController_EndpointID[LocalAgentController_EndpointID.length] = \"%s\";\n", controller_entry.EndpointID);
			nBytesSent += boaWrite(wp, "LocalAgentController_PeriodicNotifInterval[LocalAgentController_PeriodicNotifInterval.length] = \"%u\";\n", controller_entry.PeriodicNotifInterval);
		}
	}
	else if (!strcmp(name, "tr369_localagent_controller_mtp_table"))
	{
		int i = 0, total = 0;
		MIB_TR369_LOCALAGENT_CONTROLLER_MTP_T controller_mtp_entry;

		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_Controller_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_Protocol = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_CoAP_Host = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_CoAP_Port = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_CoAP_Path = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_CoAP_EnableEncryption = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_STOMP_Reference = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_STOMP_Destination = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_WebSocket_Host = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_WebSocket_Port = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_WebSocket_Path = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_WebSocket_EnableEncryption = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_WebSocket_KeepAliveInterval = new Array();\n");

		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_MQTT_Reference = new Array();\n");
		nBytesSent += boaWrite(wp, "var LocalAgentController_MTP_MQTT_Topic = new Array();\n");

		total = mib_chain_total(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL);
		for (i = 0; i < total; i++)
		{
			if (mib_chain_get(MIB_TR369_LOCALAGENT_CONTROLLER_MTP_TBL, i, (char *)&controller_mtp_entry) == 0)
				continue;

			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_Index[LocalAgentController_MTP_Index.length] = \"%u\";\n", controller_mtp_entry.instnum);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_Controller_Index[LocalAgentController_MTP_Controller_Index.length] = \"%u\";\n", controller_mtp_entry.controller_instnum);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_Enable[LocalAgentController_MTP_Enable.length] = \"%u\";\n", controller_mtp_entry.Enable);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_Protocol[LocalAgentController_MTP_Protocol.length] = \"%s\";\n", controller_mtp_entry.Protocol);

#if defined(USP_TR369_ENABLE_COAP)
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_CoAP_Host[LocalAgentController_MTP_CoAP_Host.length] = \"%s\";\n", controller_mtp_entry.CoAP_Host);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_CoAP_Port[LocalAgentController_MTP_CoAP_Port.length] = \"%u\";\n", controller_mtp_entry.CoAP_Port);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_CoAP_Path[LocalAgentController_MTP_CoAP_Path.length] = \"%s\";\n", controller_mtp_entry.CoAP_Path);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_CoAP_EnableEncryption[LocalAgentController_MTP_CoAP_EnableEncryption.length] = \"%u\";\n", controller_mtp_entry.CoAP_EnableEncryption);
#endif

#if defined(USP_TR369_ENABLE_STOMP)
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_STOMP_Reference[LocalAgentController_MTP_STOMP_Reference.length] = \"%s\";\n", controller_mtp_entry.STOMP_Reference);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_STOMP_Destination[LocalAgentController_MTP_STOMP_Destination.length] = \"%s\";\n", controller_mtp_entry.STOMP_Destination);
#endif

#if defined(USP_TR369_ENABLE_WEBSOCKET)
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_WebSocket_Host[LocalAgentController_MTP_WebSocket_Host.length] = \"%s\";\n", controller_mtp_entry.WebSocket_Host);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_WebSocket_Port[LocalAgentController_MTP_WebSocket_Port.length] = \"%u\";\n", controller_mtp_entry.WebSocket_Port);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_WebSocket_Path[LocalAgentController_MTP_WebSocket_Path.length] = \"%s\";\n", controller_mtp_entry.WebSocket_Path);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_WebSocket_EnableEncryption[LocalAgentController_MTP_WebSocket_EnableEncryption.length] = \"%u\";\n", controller_mtp_entry.WebSocket_EnableEncryption);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_WebSocket_KeepAliveInterval[LocalAgentController_MTP_WebSocket_KeepAliveInterval.length] = \"%u\";\n", controller_mtp_entry.WebSocket_KeepAliveInterval);
#endif

#if defined(USP_TR369_ENABLE_MQTT)
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_MQTT_Reference[LocalAgentController_MTP_MQTT_Reference.length] = \"%s\";\n", controller_mtp_entry.MQTT_Reference);
			nBytesSent += boaWrite(wp, "LocalAgentController_MTP_MQTT_Topic[LocalAgentController_MTP_MQTT_Topic.length] = \"%s\";\n", controller_mtp_entry.MQTT_Topic);
#endif
		}
	}
	else if (!strcmp(name, "tr369_mtp_protocol_table"))
	{
		nBytesSent += boaWrite(wp, "var MTP_Protocol = new Array();\n");
#if defined(USP_TR369_ENABLE_COAP)
		nBytesSent += boaWrite(wp, "MTP_Protocol[MTP_Protocol.length] = \"%s\";\n", "CoAP");
		nBytesSent += boaWrite(wp, "var ENABLE_CoAP = 1;\n");
#else
		nBytesSent += boaWrite(wp, "var ENABLE_CoAP = 0;\n");
#endif
#if defined(USP_TR369_ENABLE_STOMP)
		nBytesSent += boaWrite(wp, "MTP_Protocol[MTP_Protocol.length] = \"%s\";\n", "STOMP");
		nBytesSent += boaWrite(wp, "var ENABLE_STOMP = 1;\n");
#else
		nBytesSent += boaWrite(wp, "var ENABLE_STOMP = 0;\n");
#endif
#if defined(USP_TR369_ENABLE_WEBSOCKET)
		nBytesSent += boaWrite(wp, "MTP_Protocol[MTP_Protocol.length] = \"%s\";\n", "WebSocket");
		nBytesSent += boaWrite(wp, "var ENABLE_WebSocket = 1;\n");
#else
		nBytesSent += boaWrite(wp, "var ENABLE_WebSocket = 0;\n");
#endif
#if defined(USP_TR369_ENABLE_MQTT)
		nBytesSent += boaWrite(wp, "MTP_Protocol[MTP_Protocol.length] = \"%s\";\n", "MQTT");
		nBytesSent += boaWrite(wp, "var ENABLE_MQTT = 1;\n");
#else
		nBytesSent += boaWrite(wp, "var ENABLE_MQTT = 0;\n");
#endif
	}
	else if (!strcmp(name, "tr369_stomp_connection_table"))
	{
		int i = 0, total = 0;
		MIB_TR369_STOMP_CONNECTION_T stomp_entry;

		nBytesSent += boaWrite(wp, "var STOMP_Connection_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_Host = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_Port = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_Username = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_Password = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_EnableHeartbeats = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_OutgoingHeartbeat = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_IncomingHeartbeat = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_ServerRetryInitialInterval = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_ServerRetryIntervalMultiplier = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_ServerRetryMaxInterval = new Array();\n");
		nBytesSent += boaWrite(wp, "var STOMP_Connection_EnableEncryption = new Array();\n");

#if defined(USP_TR369_ENABLE_STOMP)
		total = mib_chain_total(MIB_TR369_STOMP_CONNECTION_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_TR369_STOMP_CONNECTION_TBL, i, (void *)&stomp_entry))
				continue;

			nBytesSent += boaWrite(wp, "STOMP_Connection_Index[STOMP_Connection_Index.length] = \"%u\";\n", stomp_entry.instnum);
			nBytesSent += boaWrite(wp, "STOMP_Connection_Enable[STOMP_Connection_Enable.length] = \"%u\";\n", stomp_entry.Enable);
			nBytesSent += boaWrite(wp, "STOMP_Connection_Host[STOMP_Connection_Host.length] = \"%s\";\n", stomp_entry.Host);
			nBytesSent += boaWrite(wp, "STOMP_Connection_Port[STOMP_Connection_Port.length] = \"%u\";\n", stomp_entry.Port);
			nBytesSent += boaWrite(wp, "STOMP_Connection_Username[STOMP_Connection_Username.length] = \"%s\";\n", stomp_entry.Username);
			nBytesSent += boaWrite(wp, "STOMP_Connection_Password[STOMP_Connection_Password.length] = \"%s\";\n", stomp_entry.Password);
			nBytesSent += boaWrite(wp, "STOMP_Connection_EnableHeartbeats[STOMP_Connection_EnableHeartbeats.length] = \"%u\";\n", stomp_entry.EnableHeartbeats);
			nBytesSent += boaWrite(wp, "STOMP_Connection_OutgoingHeartbeat[STOMP_Connection_OutgoingHeartbeat.length] = \"%u\";\n", stomp_entry.OutgoingHeartbeat);
			nBytesSent += boaWrite(wp, "STOMP_Connection_IncomingHeartbeat[STOMP_Connection_IncomingHeartbeat.length] = \"%u\";\n", stomp_entry.IncomingHeartbeat);
			nBytesSent += boaWrite(wp, "STOMP_Connection_ServerRetryInitialInterval[STOMP_Connection_ServerRetryInitialInterval.length] = \"%u\";\n", stomp_entry.ServerRetryInitialInterval);
			nBytesSent += boaWrite(wp, "STOMP_Connection_ServerRetryIntervalMultiplier[STOMP_Connection_ServerRetryIntervalMultiplier.length] = \"%u\";\n", stomp_entry.ServerRetryIntervalMultiplier);
			nBytesSent += boaWrite(wp, "STOMP_Connection_ServerRetryMaxInterval[STOMP_Connection_ServerRetryMaxInterval.length] = \"%u\";\n", stomp_entry.ServerRetryMaxInterval);
			nBytesSent += boaWrite(wp, "STOMP_Connection_EnableEncryption[STOMP_Connection_EnableEncryption.length] = \"%u\";\n", stomp_entry.EnableEncryption);
		}
#endif
	}
	else if (!strcmp(name, "tr369_mqtt_client_table"))
	{
		int i = 0, total = 0;
		MIB_TR369_MQTT_CLIENT_T mqtt_client_entry;
		MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_T mqtt_client_sub_entry;

		/* Client table */
		nBytesSent += boaWrite(wp, "var MQTT_Client_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_BrokerAddress = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_BrokerPort = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Username = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Password = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_KeepAliveTime = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_ProtocolVersion = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_ClientID = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_TransportProtocol = new Array();\n");

		/* Subscription table */
		nBytesSent += boaWrite(wp, "var MQTT_Client_Subscription_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Subscription_Client_Index = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Subscription_Enable = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Subscription_Topic = new Array();\n");
		nBytesSent += boaWrite(wp, "var MQTT_Client_Subscription_QoS = new Array();\n");

#if defined(USP_TR369_ENABLE_MQTT)
		total = mib_chain_total(MIB_TR369_MQTT_CLIENT_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_TBL, i, (void *)&mqtt_client_entry))
				continue;

			nBytesSent += boaWrite(wp, "MQTT_Client_Index[MQTT_Client_Index.length] = \"%u\";\n", mqtt_client_entry.instnum);
			nBytesSent += boaWrite(wp, "MQTT_Client_Enable[MQTT_Client_Enable.length] = \"%u\";\n", mqtt_client_entry.Enable);
			nBytesSent += boaWrite(wp, "MQTT_Client_BrokerAddress[MQTT_Client_BrokerAddress.length] = \"%s\";\n", mqtt_client_entry.BrokerAddress);
			nBytesSent += boaWrite(wp, "MQTT_Client_BrokerPort[MQTT_Client_BrokerPort.length] = \"%u\";\n", mqtt_client_entry.BrokerPort);
			nBytesSent += boaWrite(wp, "MQTT_Client_Username[MQTT_Client_Username.length] = \"%s\";\n", mqtt_client_entry.Username);
			nBytesSent += boaWrite(wp, "MQTT_Client_Password[MQTT_Client_Password.length] = \"%s\";\n", mqtt_client_entry.Password);
			nBytesSent += boaWrite(wp, "MQTT_Client_KeepAliveTime[MQTT_Client_KeepAliveTime.length] = \"%u\";\n", mqtt_client_entry.KeepAliveTime);
			nBytesSent += boaWrite(wp, "MQTT_Client_ProtocolVersion[MQTT_Client_ProtocolVersion.length] = \"%u\";\n", mqtt_client_entry.ProtocolVersion);
			nBytesSent += boaWrite(wp, "MQTT_Client_ClientID[MQTT_Client_ClientID.length] = \"%s\";\n", mqtt_client_entry.ClientID);
			nBytesSent += boaWrite(wp, "MQTT_Client_TransportProtocol[MQTT_Client_TransportProtocol.length] = \"%u\";\n", mqtt_client_entry.TransportProtocol);
		}

		total = mib_chain_total(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL);
		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_TR369_MQTT_CLIENT_SUBSCRIPTION_TBL, i, (void *)&mqtt_client_sub_entry))
				continue;

			nBytesSent += boaWrite(wp, "MQTT_Client_Subscription_Index[MQTT_Client_Subscription_Index.length] = \"%u\";\n", mqtt_client_sub_entry.instnum);
			nBytesSent += boaWrite(wp, "MQTT_Client_Subscription_Client_Index[MQTT_Client_Subscription_Client_Index.length] = \"%u\";\n", mqtt_client_sub_entry.client_instnum);
			nBytesSent += boaWrite(wp, "MQTT_Client_Subscription_Enable[MQTT_Client_Subscription_Enable.length] = \"%u\";\n", mqtt_client_sub_entry.Enable);
			nBytesSent += boaWrite(wp, "MQTT_Client_Subscription_Topic[MQTT_Client_Subscription_Topic.length] = \"%s\";\n", mqtt_client_sub_entry.Topic);
			nBytesSent += boaWrite(wp, "MQTT_Client_Subscription_QoS[MQTT_Client_Subscription_QoS.length] = \"%u\";\n", mqtt_client_sub_entry.QoS);
		}
#endif
	}

	return nBytesSent;
}


