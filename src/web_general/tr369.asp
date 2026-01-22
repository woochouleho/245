<%SendWebHeadStr(); %>
<title>TR-369 <% multilang(LANG_CONFIGURATION); %></title>

<script>
<% showTR369List("tr369_mtp_protocol_table"); %>
<% showTR369List("tr369_localagent_mtp_table"); %>
<% showTR369List("tr369_localagent_controller_table"); %>
<% showTR369List("tr369_localagent_controller_mtp_table"); %>
<% showTR369List("tr369_stomp_connection_table"); %>
<% showTR369List("tr369_mqtt_client_table"); %>

function display_CoAP(id_type, id_prefix, display_flag)
{
	with (document.forms[0])
	{
		var i = 0;
		var display_id_prefix = id_type + '_' +  id_prefix;
		var CoAP_tr_id = ['tr_localagent_mtp_coap_encryption',
							'tr_localagent_mtp_coap_path',
							'tr_localagent_mtp_coap_port',
							'tr_localagent_controller_mtp_coap_encryption',
							'tr_localagent_controller_mtp_coap_host',
							'tr_localagent_controller_mtp_coap_port',
							'tr_localagent_controller_mtp_coap_path'];

		for (i = 0; i < CoAP_tr_id.length; i++)
		{
			if (CoAP_tr_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(CoAP_tr_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'table-row';
				}
			}
		}
	}
	return true;
}

function display_STOMP(id_type, id_prefix, display_flag)
{
	with (document.forms[0])
	{
		var i = 0;
		var display_id_prefix = id_type + '_' + id_prefix;
		var STOMP_tr_id = ['tr_localagent_mtp_stomp_reference',
							'tr_localagent_mtp_stomp_destination',
							'tr_localagent_controller_mtp_stomp_reference',
							'tr_localagent_controller_mtp_stomp_destination'];

		for (i = 0; i < STOMP_tr_id.length; i++)
		{
			if (STOMP_tr_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(STOMP_tr_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'table-row';
				}
			}
		}

		var STOMP_div_id = ['div_stomp_connection_config',
							'div_stomp_connection_apply'];

		for (i = 0; i < STOMP_div_id.length; i++)
		{
			if (STOMP_div_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(STOMP_div_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'block';
				}
			}
		}
	}
	return true;
}

function display_WebSocket(id_type, id_prefix, display_flag)
{
	with (document.forms[0])
	{
		var i = 0;
		var display_id_prefix = id_type + '_' +  id_prefix;
		var WebSocket_tr_id = ['tr_localagent_controller_mtp_websocket_host',
							'tr_localagent_controller_mtp_websocket_port',
							'tr_localagent_controller_mtp_websocket_path',
							'tr_localagent_controller_mtp_websocket_encryption',
							'tr_localagent_controller_mtp_websocket_keepaliveinterval'];

		for (i = 0; i < WebSocket_tr_id.length; i++)
		{
			if (WebSocket_tr_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(WebSocket_tr_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'table-row';
				}
			}
		}
	}
	return true;
}

function display_MQTT(id_type, id_prefix, display_flag)
{
	with (document.forms[0])
	{
		var i = 0;
		var display_id_prefix = id_type + '_' + id_prefix;
		var MQTT_tr_id = ['tr_localagent_mtp_mqtt_reference',
							'tr_localagent_mtp_mqtt_response_topic',
							'tr_localagent_mtp_mqtt_publishqos',
							'tr_localagent_controller_mtp_mqtt_reference',
							'tr_localagent_controller_mtp_mqtt_topic',
							'tr_mqtt_client_subscription_enable',
							'tr_mqtt_client_subscription_topic',
							'tr_mqtt_client_subscription_qos'];

		for (i = 0; i < MQTT_tr_id.length; i++)
		{
			if (MQTT_tr_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(MQTT_tr_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'table-row';
				}
			}
		}

		var MQTT_div_id = ['div_mqtt_client_config',
							'div_mqtt_client_apply'];

		for (i = 0; i < MQTT_div_id.length; i++)
		{
			if (MQTT_div_id[i].indexOf(display_id_prefix) > -1)
			{
				var result_style = document.getElementById(MQTT_div_id[i]).style;
				if (display_flag == 0)
				{
					result_style.display = 'none';
				}
				else
				{
					result_style.display = 'block';
				}
			}
		}
	}
	return true;
}

function build_mtp_list(select_id)
{
	with (document.forms[0])
	{
		var select_list = document.getElementById(select_id);
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < MTP_Protocol.length; i++)
		{
			select_list.options.add(new Option(MTP_Protocol[i], MTP_Protocol[i]));
		}
	}
	return true;
}

function build_controller_mtp_list()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('localagent_controller_mtp_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < LocalAgentController_MTP_Index.length; i++)
		{
			if (LocalAgentController_MTP_Controller_Index[i] == localagent_controller_instnum.value)
				select_list.options.add(new Option("MTP - " + LocalAgentController_MTP_Index[i], LocalAgentController_MTP_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		build_stomp_reference_list('localagent_controller_mtp_stomp_reference');
		build_mqtt_reference_list('localagent_controller_mtp_mqtt_reference');
	}
	return true;
}

function view_controller_mtp()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('localagent_controller_mtp_instnum');
		if (!select_list) return false;
		if (localagent_controller_mtp_instnum.value == -1)
		{
			localagent_controller_mtp_enable.checked = false;
			localagent_controller_mtp_protocol.selectedIndex = 0;
			localagent_controller_mtp_stomp_reference.selectedIndex = 0;
			localagent_controller_mtp_stomp_destination.value = "";
			localagent_controller_mtp_coap_encryption.checked = false;
			localagent_controller_mtp_coap_host.value = "";
			localagent_controller_mtp_coap_port.value = "";
			localagent_controller_mtp_coap_path.value = "";
			localagent_controller_mtp_websocket_host.value = "";
			localagent_controller_mtp_websocket_port.value = "";
			localagent_controller_mtp_websocket_path.value = "";
			localagent_controller_mtp_websocket_encryption.checked = false;
			localagent_controller_mtp_websocket_keepaliveinterval.value = "";
			localagent_controller_mtp_mqtt_reference.selectedIndex = 0;
			localagent_controller_mtp_mqtt_topic.value = "";
			localagent_controller_mtp_delete.style.display = 'none';
		}
		else
		{
			var i = 0, j = 0;
			for (i = 0; i < LocalAgentController_MTP_Index.length; i++)
			{
				if (LocalAgentController_MTP_Controller_Index[i] == localagent_controller_instnum.value)
				{
					if (LocalAgentController_MTP_Index[i] == localagent_controller_mtp_instnum.value)
					{
						localagent_controller_mtp_enable.checked = (LocalAgentController_MTP_Enable[i] == "1") ? true:false;
						localagent_controller_mtp_protocol.value = LocalAgentController_MTP_Protocol[i];

						localagent_controller_mtp_stomp_reference.selectedIndex = 0;
						for (j = 0; j < STOMP_Connection_Index.length; j++)
						{
							var ref = "Device.STOMP.Connection." + STOMP_Connection_Index[j];
							if (ref.indexOf(LocalAgentController_MTP_STOMP_Reference[i]) > -1)
							{
								localagent_controller_mtp_stomp_reference.selectedIndex = j;
								break;
							}
						}

						localagent_controller_mtp_stomp_destination.value = LocalAgentController_MTP_STOMP_Destination[i];
						localagent_controller_mtp_coap_encryption.checked = (LocalAgentController_MTP_CoAP_EnableEncryption[i] == "1") ? true:false;
						localagent_controller_mtp_coap_host.value = LocalAgentController_MTP_CoAP_Host[i];
						localagent_controller_mtp_coap_port.value = LocalAgentController_MTP_CoAP_Port[i];
						localagent_controller_mtp_coap_path.value = LocalAgentController_MTP_CoAP_Path[i];
						localagent_controller_mtp_websocket_host.value = LocalAgentController_MTP_WebSocket_Host[i];
						localagent_controller_mtp_websocket_port.value = LocalAgentController_MTP_WebSocket_Port[i];
						localagent_controller_mtp_websocket_path.value = LocalAgentController_MTP_WebSocket_Path[i];
						localagent_controller_mtp_websocket_encryption.checked = (LocalAgentController_MTP_WebSocket_EnableEncryption[i] == "1") ? true:false;
						localagent_controller_mtp_websocket_keepaliveinterval.value = LocalAgentController_MTP_WebSocket_KeepAliveInterval[i];

						localagent_controller_mtp_mqtt_reference.selectedIndex = 0;
						for (j = 0; j < MQTT_Client_Index.length; j++)
						{
							var ref = "Device.MQTT.Client." + MQTT_Client_Index[j];
							if (ref.indexOf(LocalAgentController_MTP_MQTT_Reference[i]) > -1)
							{
								localagent_controller_mtp_mqtt_reference.selectedIndex = j;
								break;
							}
						}
						localagent_controller_mtp_mqtt_topic.value = LocalAgentController_MTP_MQTT_Topic[i];

						localagent_controller_mtp_delete.style.display = 'inline';
						break;
					}
				}
			}
		}
		view_mtp_protocol_config('localagent_controller_mtp');
	}
	return true;
}

function view_mtp_protocol_config(id_prefix)
{
	with (document.forms[0])
	{
		var i = 0;
		var select_list = document.getElementById(id_prefix + '_protocol');
		if (!select_list) return false;

		display_CoAP('tr', id_prefix, 0);
		display_STOMP('tr', id_prefix, 0);
		display_WebSocket('tr', id_prefix, 0);
		display_MQTT('tr', id_prefix, 0);
		if (select_list.value == "CoAP")
		{
			display_CoAP('tr', id_prefix, 1);
		}
		else if (select_list.value == "STOMP")
		{
			display_STOMP('tr', id_prefix, 1);
		}
		else if (select_list.value == "WebSocket")
		{
			display_WebSocket('tr', id_prefix, 1);
		}
		else if (select_list.value == "MQTT")
		{
			display_MQTT('tr', id_prefix, 1);
		}
		else
		{
			return false;
		}
	}
	return true;
}

function view_mtp()
{
	with (document.forms[0])
	{
		var i = 0, j = 0, find = 0;
		var mtp_index = localagent_mtp_instnum.value;
		for (i = 0; i < LocalAgentMTP_Index.length; i++)
		{
			if (LocalAgentMTP_Index[i] == mtp_index)
			{
				localagent_mtp_enable.checked = (LocalAgentMTP_Enable[i] == "1") ? true:false;
				localagent_mtp_protocol.value = LocalAgentMTP_Protocol[i];

				localagent_mtp_stomp_reference.selectedIndex = 0;
				for (j = 0; j < STOMP_Connection_Index.length; j++)
				{
					var ref = "Device.STOMP.Connection." + STOMP_Connection_Index[j];
					if (ref.indexOf(LocalAgentMTP_STOMP_Reference[i]) > -1)
					{
						localagent_mtp_stomp_reference.selectedIndex = j;
						break;
					}
				}

				localagent_mtp_stomp_destination.value = LocalAgentMTP_STOMP_Destination[i];
				localagent_mtp_coap_encryption.checked = (LocalAgentMTP_CoAP_EnableEncryption[i] == "1") ? true:false;
				localagent_mtp_coap_path.value = LocalAgentMTP_CoAP_Path[i];
				localagent_mtp_coap_port.value = LocalAgentMTP_CoAP_Port[i];

				localagent_mtp_mqtt_reference.selectedIndex = 0;
				for (j = 0; j < MQTT_Client_Index.length; j++)
				{
					var ref = "Device.MQTT.Client." + MQTT_Client_Index[j];
					if (ref.indexOf(LocalAgentMTP_MQTT_Reference[i]) > -1)
					{
						localagent_mtp_mqtt_reference.selectedIndex = j;
						break;
					}
				}
				localagent_mtp_mqtt_response_topic.value = LocalAgentMTP_MQTT_ResponseTopicConfigured[i];
				localagent_mtp_mqtt_publishqos.value = LocalAgentMTP_MQTT_PublishQoS[i];

				localagent_mtp_delete.style.display = 'inline';
				find = 1;
				break;
			}
		}
		if (find == 0)
		{
			localagent_mtp_enable.checked = false;
			localagent_mtp_protocol.selectedIndex = 0;
			localagent_mtp_stomp_reference.selectedIndex = 0;
			localagent_mtp_stomp_destination.value = "";
			localagent_mtp_coap_encryption.checked = false;
			localagent_mtp_coap_path.value = "";
			localagent_mtp_coap_port.value = "";
			localagent_mtp_mqtt_reference.selectedIndex = 0;
			localagent_mtp_mqtt_response_topic.value = "";
			localagent_mtp_mqtt_publishqos.value = "0";
			localagent_mtp_delete.style.display = 'none';
		}
		view_mtp_protocol_config('localagent_mtp');
	}
	return true;
}

function build_localagent_mtp_select()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('localagent_mtp_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < LocalAgentMTP_Index.length; i++)
		{
			select_list.options.add(new Option("MTP - " + LocalAgentMTP_Index[i], LocalAgentMTP_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		build_stomp_reference_list('localagent_mtp_stomp_reference');
		build_mqtt_reference_list('localagent_mtp_mqtt_reference');
		view_mtp();
	}
	return true;
}

function view_controller()
{
	with (document.forms[0])
	{
		var i = 0, find = 0;
		build_controller_mtp_list();
		for (i = 0; i < LocalAgentController_Index.length; i++)
		{
			if (LocalAgentController_Index[i] == localagent_controller_instnum.value)
			{
				localagent_controller_enable.checked = (LocalAgentController_Enable[i] == "1") ? true:false;
				localagent_controller_endpointid.value = LocalAgentController_EndpointID[i];
				localagent_controller_periodicnotifinterval.value = LocalAgentController_PeriodicNotifInterval[i];
				localagent_controller_mtp_instnum.selectedIndex = 0;
				localagent_controller_delete.style.display = 'inline';
				find = 1;
				break;
			}
		}
		if (find == 0)
		{
			localagent_controller_enable.checked = false;
			localagent_controller_endpointid.value = "";
			localagent_controller_periodicnotifinterval.value = "";
			localagent_controller_mtp_instnum.value = -1;
			localagent_controller_delete.style.display = 'none';
		}
		view_controller_mtp();
	}
	return true;
}

function build_localagent_controller_select()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('localagent_controller_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < LocalAgentController_Index.length; i++)
		{
			select_list.options.add(new Option("Controller - " + LocalAgentController_Index[i], LocalAgentController_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		view_controller();
	}
	return true;
}

function localagent_mtp_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "localagent_mtp";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function localagent_mtp_save()
{
	with (document.forms[0])
	{
		tr369_target.value = "localagent_mtp";
		tr369_action.value = "1";
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function localagent_controller_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "localagent_controller";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function localagent_controller_mtp_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "localagent_controller_mtp";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function localagent_controller_mtp_save()
{
	with (document.forms[0])
	{
		tr369_target.value = "localagent_controller_mtp";
		tr369_action.value = "1";
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function view_stomp_connection()
{
	with (document.forms[0])
	{
		var i = 0, find = 0;
		var stomp_connection_index = stomp_connection_instnum.value;
		for (i = 0; i < STOMP_Connection_Index.length; i++)
		{
			if (STOMP_Connection_Index[i] == stomp_connection_index)
			{
				stomp_connection_enable.checked = (STOMP_Connection_Enable[i] == "1") ? true:false;
				stomp_connection_host.value = STOMP_Connection_Host[i];
				stomp_connection_port.value = STOMP_Connection_Port[i];
				stomp_connection_username.value = STOMP_Connection_Username[i];
				stomp_connection_password.value = STOMP_Connection_Password[i];
				stomp_connection_heartbeat.checked = (STOMP_Connection_EnableHeartbeats[i] == "1") ? true:false;
				stomp_connection_outgoingheartbeat.value = STOMP_Connection_OutgoingHeartbeat[i];
				stomp_connection_incomingheartbeat.value = STOMP_Connection_IncomingHeartbeat[i];
				stomp_connection_encryption.checked = (STOMP_Connection_EnableEncryption[i] == "1") ? true:false;
				stomp_connection_delete.style.display = 'inline';
				find = 1;
				break;
			}
		}
		if (find == 0)
		{
			stomp_connection_enable.checked = false;
			stomp_connection_host.value = "";
			stomp_connection_port.value = "";
			stomp_connection_username.value = "";
			stomp_connection_password.value = "";
			stomp_connection_heartbeat.checked = false;
			stomp_connection_outgoingheartbeat.value = "0";
			stomp_connection_incomingheartbeat.value = "0";
			stomp_connection_encryption.checked = false;
			stomp_connection_delete.style.display = 'none';
		}
	}
	return true;
}

function build_stomp_connection_select()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('stomp_connection_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < STOMP_Connection_Index.length; i++)
		{
			select_list.options.add(new Option("STOMP Connection - " + STOMP_Connection_Index[i], STOMP_Connection_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		view_stomp_connection();
	}
	return true;
}

function build_stomp_reference_list(select_id)
{
	with (document.forms[0])
	{
		var select_list = document.getElementById(select_id);
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < STOMP_Connection_Index.length; i++)
		{
			select_list.options.add(new Option("STOMP Connection - " + STOMP_Connection_Index[i], "Device.STOMP.Connection." + STOMP_Connection_Index[i]));
		}
	}
	return true;
}

function stomp_connection_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "stomp_connection";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function stomp_connection_save()
{
	with (document.forms[0])
	{
		tr369_target.value = "stomp_connection";
		tr369_action.value = "1";
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function view_mqtt_client_subscription()
{
	with (document.forms[0])
	{
		if (mqtt_client_subscription_instnum.value == -2)
		{
			display_MQTT('tr', 'mqtt_client_subscription', 0);
		}
		else
		{
			display_MQTT('tr', 'mqtt_client_subscription', 1);
		}

		if (mqtt_client_subscription_instnum.value == -1 || mqtt_client_subscription_instnum.value == -2)
		{
			mqtt_client_subscription_enable.checked = false;
			mqtt_client_subscription_topic.value = "";
			mqtt_client_subscription_qos.value = "0";
			mqtt_client_subscription_delete.style.display = 'none';
		}
		else
		{
			var i = 0, j = 0;
			for (i = 0; i < MQTT_Client_Subscription_Index.length; i++)
			{
				if (MQTT_Client_Subscription_Client_Index[i] == mqtt_client_instnum.value)
				{
					if (MQTT_Client_Subscription_Index[i] == mqtt_client_subscription_instnum.value)
					{
						mqtt_client_subscription_enable.checked = (MQTT_Client_Subscription_Enable[i] == "1") ? true:false;
						mqtt_client_subscription_topic.value = MQTT_Client_Subscription_Topic[i];
						mqtt_client_subscription_qos.value = MQTT_Client_Subscription_QoS[i];
						mqtt_client_subscription_delete.style.display = 'inline';
						break;
					}
				}
			}
		}
	}
	return true;
}

function build_client_subscription_list()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('mqtt_client_subscription_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		select_list.options.add(new Option("", "-2"));
		for (i = 0; i < MQTT_Client_Subscription_Index.length; i++)
		{
			if (MQTT_Client_Subscription_Client_Index[i] == mqtt_client_instnum.value)
				select_list.options.add(new Option("Subscription - " + MQTT_Client_Subscription_Index[i], MQTT_Client_Subscription_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		view_mqtt_client_subscription();
	}
	return true;
}

function view_mqtt_client()
{
	with (document.forms[0])
	{
		var i = 0, find = 0;
		var mqtt_client_index = mqtt_client_instnum.value;
		for (i = 0; i < MQTT_Client_Index.length; i++)
		{
			if (MQTT_Client_Index[i] == mqtt_client_index)
			{
				mqtt_client_enable.checked = (MQTT_Client_Enable[i] == "1") ? true:false;
				mqtt_client_brokeraddress.value = MQTT_Client_BrokerAddress[i];
				mqtt_client_brokerport.value = MQTT_Client_BrokerPort[i];
				mqtt_client_username.value = MQTT_Client_Username[i];
				mqtt_client_password.value = MQTT_Client_Password[i];
				mqtt_client_clientid.value = MQTT_Client_ClientID[i];
				mqtt_client_keepalivetime.value = MQTT_Client_KeepAliveTime[i];
				mqtt_client_protocol_version.value = MQTT_Client_ProtocolVersion[i];
				mqtt_client_transport_protocol.value = MQTT_Client_TransportProtocol[i];
				mqtt_client_delete.style.display = 'inline';
				find = 1;
				break;
			}
		}
		if (find == 0)
		{
			mqtt_client_enable.checked = false;
			mqtt_client_brokeraddress.value = "";
			mqtt_client_brokerport.value = "";
			mqtt_client_username.value = "";
			mqtt_client_password.value = "";
			mqtt_client_clientid.value = "";
			mqtt_client_keepalivetime.value = "0";
			mqtt_client_protocol_version.value = "0";
			mqtt_client_transport_protocol.value = "0";
			mqtt_client_delete.style.display = 'none';
		}
		build_client_subscription_list();
	}
	return true;
}

function build_mqtt_client_select()
{
	with (document.forms[0])
	{
		var select_list = document.getElementById('mqtt_client_instnum');
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < MQTT_Client_Index.length; i++)
		{
			select_list.options.add(new Option("MQTT Client - " + MQTT_Client_Index[i], MQTT_Client_Index[i]));
		}
		select_list.options.add(new Option("New", "-1"));
		view_mqtt_client();
	}
	return true;
}

function build_mqtt_reference_list(select_id)
{
	with (document.forms[0])
	{
		var select_list = document.getElementById(select_id);
		if (!select_list) return false;
		var i = 0;
		for (i = (select_list.options.length - 1); i >= 0; i--)
		{
			select_list.options.remove(i);
		}
		for (i = 0; i < MQTT_Client_Index.length; i++)
		{
			select_list.options.add(new Option("MQTT Client - " + MQTT_Client_Index[i], "Device.MQTT.Client." + MQTT_Client_Index[i]));
		}
	}
	return true;
}

function mqtt_client_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "mqtt_client";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function mqtt_client_save()
{
	with (document.forms[0])
	{
		tr369_target.value = "mqtt_client";
		tr369_action.value = "1";
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function mqtt_client_subscription_del()
{
	with (document.forms[0])
	{
		if (!confirm('Do you really want to delete the selected entry?'))
		{
			return false;
		}
		else
		{
			tr369_target.value = "mqtt_client_subscription";
			tr369_action.value = "0";
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			return true;
		}
	}
	return true;
}

function formLoad()
{
	with (document.forms[0])
	{
		build_mtp_list('localagent_mtp_protocol');
		build_mtp_list('localagent_controller_mtp_protocol');
		build_localagent_mtp_select();
		build_localagent_controller_select();
		build_stomp_connection_select();
		build_mqtt_client_select();

		if (ENABLE_CoAP == 0)
		{
			display_CoAP('tr', 'localagent_mtp', 0);
			display_CoAP('tr', 'localagent_controller_mtp', 0);
		}
		if (ENABLE_STOMP == 0)
		{
			display_STOMP('tr', 'localagent_mtp', 0);
			display_STOMP('tr', 'localagent_controller_mtp', 0);
			display_STOMP('div', 'stomp_connection', 0);
		}
		if (ENABLE_WebSocket == 0)
		{
			display_WebSocket('tr', 'localagent_controller_mtp', 0);
		}
		if (ENABLE_MQTT == 0)
		{
			display_MQTT('tr', 'localagent_mtp', 0);
			display_MQTT('tr', 'localagent_controller_mtp', 0);
			display_MQTT('div', 'mqtt_client', 0);
		}
	}
	return true;
}
</script>
</head>
<body onLoad="formLoad();">
<div class="intro_main ">
	<p class="intro_title">TR-369 Configuration</p>
</div>
<form action="/boaform/formTR369" method="POST" name="tr369form">
	<div class="data_common data_common_notitle">
		<div class="column_title">
			<div class="column_title_left"></div>
			<p>LocalAgent MTP</p>
			<div class="column_title_right"></div>
		</div>
		<table id="table_localagent_mtp">
			<tr>
				<th width="30%">Select:</th>
				<td width="70%">
					<select name="localagent_mtp_instnum" id="localagent_mtp_instnum" onChange="view_mtp();"></select>
					<input class="link_bg" type="submit" id="localagent_mtp_delete" value="Delete" onClick="return localagent_mtp_del();">
				</td>
			</tr>
			<tr>
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="localagent_mtp_enable" id="localagent_mtp_enable" value="1"></td>
			</tr>
			<tr>
				<th width="30%">Protocol:</th>
				<td width="70%">
					<select name="localagent_mtp_protocol" id="localagent_mtp_protocol" onChange="view_mtp_protocol_config('localagent_mtp');"></select>
				</td>
			</tr>
			<tr id="tr_localagent_mtp_stomp_reference">
				<th width="30%">STOMP - Reference:</th>
				<td width="70%">
					<select name="localagent_mtp_stomp_reference" id="localagent_mtp_stomp_reference"></select>
				</td>
			</tr>
			<tr id="tr_localagent_mtp_stomp_destination">
				<th width="30%">STOMP - Destination:</th>
				<td width="70%">
					<input type="text" name="localagent_mtp_stomp_destination" id="localagent_mtp_stomp_destination" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_localagent_mtp_coap_encryption">
				<th width="30%">CoAP - Encryption:</th>
				<td width="70%"><input type="checkbox" name="localagent_mtp_coap_encryption" id="localagent_mtp_coap_encryption" value="1"></td>
			</tr>
			<tr id="tr_localagent_mtp_coap_path">
				<th width="30%">CoAP - Path:</th>
				<td width="70%">
					<input type="text" name="localagent_mtp_coap_path" id="localagent_mtp_coap_path" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_mtp_coap_port">
				<th width="30%">CoAP - Port:</th>
				<td width="70%">
					<input type="text" name="localagent_mtp_coap_port" id="localagent_mtp_coap_port" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_mtp_mqtt_reference">
				<th width="30%">MQTT - Reference:</th>
				<td width="70%">
					<select name="localagent_mtp_mqtt_reference" id="localagent_mtp_mqtt_reference"></select>
				</td>
			</tr>
			<tr id="tr_localagent_mtp_mqtt_response_topic">
				<th width="30%">MQTT - Response Topic:</th>
				<td width="70%">
					<input type="text" name="localagent_mtp_mqtt_response_topic" id="localagent_mtp_mqtt_response_topic" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_localagent_mtp_mqtt_publishqos">
				<th width="30%">MQTT - Publish QoS:</th>
				<td width="70%">
					<select name="localagent_mtp_mqtt_publishqos" id="localagent_mtp_mqtt_publishqos">
						<option value="0">0</option>
						<option value="1">1</option>
						<option value="2">2</option>
					</select>
				</td>
			</tr>
		</table>
	</div>
	<div id="div_localagent_mtp_apply" class="btn_ctl">
		<input class="link_bg" type="submit" value="Apply Changes" onClick="return localagent_mtp_save();">
	</div>
	<div class="data_common data_common_notitle">
		<div class="column_title">
			<div class="column_title_left"></div>
			<p>LocalAgent Controller</p>
			<div class="column_title_right"></div>
		</div>
		<table id="table_localagent_controller_mtp">
			<tr>
				<th width="30%">Select:</th>
				<td width="70%">
					<select name="localagent_controller_instnum" id="localagent_controller_instnum" onChange="view_controller();"></select>
					<input class="link_bg" type="submit" id="localagent_controller_delete" value="Delete" onClick="return localagent_controller_del();">
				</td>
			</tr>
			<tr>
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="localagent_controller_enable" id="localagent_controller_enable" value="1"></td>
			</tr>
			<tr>
				<th width="30%">Endpoint ID:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_endpointid" id="localagent_controller_endpointid" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">Periodic Notification Interval:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_periodicnotifinterval" id="localagent_controller_periodicnotifinterval" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">MTP:</th>
				<td width="70%">
					<select name="localagent_controller_mtp_instnum" id="localagent_controller_mtp_instnum" onChange="view_controller_mtp();"></select>
					<input class="link_bg" type="submit" id="localagent_controller_mtp_delete" value="Delete" onClick="return localagent_controller_mtp_del();">
				</td>
			</tr>
			<tr>
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="localagent_controller_mtp_enable" id="localagent_controller_mtp_enable" value="1"></td>
			</tr>
			<tr>
				<th width="30%">Protocol:</th>
				<td width="70%">
					<select name="localagent_controller_mtp_protocol" id="localagent_controller_mtp_protocol" onChange="view_mtp_protocol_config('localagent_controller_mtp');"></select>
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_stomp_reference">
				<th width="30%">STOMP - Reference:</th>
				<td width="70%">
					<select name="localagent_controller_mtp_stomp_reference" id="localagent_controller_mtp_stomp_reference"></select>
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_stomp_destination">
				<th width="30%">STOMP - Destination:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_stomp_destination" id="localagent_controller_mtp_stomp_destination" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_coap_encryption">
				<th width="30%">CoAP - Encryption:</th>
				<td width="70%"><input type="checkbox" name="localagent_controller_mtp_coap_encryption" id="localagent_controller_mtp_coap_encryption" value="1"></td>
			</tr>
			<tr id="tr_localagent_controller_mtp_coap_host">
				<th width="30%">CoAP - Host:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_coap_host" id="localagent_controller_mtp_coap_host" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_coap_port">
				<th width="30%">CoAP - Port:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_coap_port" id="localagent_controller_mtp_coap_port" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_coap_path">
				<th width="30%">CoAP - Path:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_coap_path" id="localagent_controller_mtp_coap_path" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_websocket_host">
				<th width="30%">WebSocket - Host:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_websocket_host" id="localagent_controller_mtp_websocket_host" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_websocket_port">
				<th width="30%">WebSocket - Port:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_websocket_port" id="localagent_controller_mtp_websocket_port" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_websocket_path">
				<th width="30%">WebSocket - Path:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_websocket_path" id="localagent_controller_mtp_websocket_path" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_websocket_encryption">
				<th width="30%">WebSocket - Encryption:</th>
				<td width="70%"><input type="checkbox" name="localagent_controller_mtp_websocket_encryption" id="localagent_controller_mtp_websocket_encryption" value="1"></td>
			</tr>
			<tr id="tr_localagent_controller_mtp_websocket_keepaliveinterval">
				<th width="30%">WebSocket - Keep Alive Interval:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_websocket_keepaliveinterval" id="localagent_controller_mtp_websocket_keepaliveinterval" size="30" maxlength="64">
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_mqtt_reference">
				<th width="30%">MQTT - Reference:</th>
				<td width="70%">
					<select name="localagent_controller_mtp_mqtt_reference" id="localagent_controller_mtp_mqtt_reference"></select>
				</td>
			</tr>
			<tr id="tr_localagent_controller_mtp_mqtt_topic">
				<th width="30%">MQTT - Topic:</th>
				<td width="70%">
					<input type="text" name="localagent_controller_mtp_mqtt_topic" id="localagent_controller_mtp_mqtt_topic" size="30" maxlength="256">
				</td>
			</tr>
		</table>
	</div>
	<div id="div_localagent_controller_mtp_apply" class="btn_ctl">
		<input class="link_bg" type="submit" value="Apply Changes" onClick="return localagent_controller_mtp_save();">
	</div>
	<div id="div_stomp_connection_config" class="data_common data_common_notitle">
		<div class="column_title">
			<div class="column_title_left"></div>
			<p>STOMP Connection</p>
			<div class="column_title_right"></div>
		</div>
		<table id="table_stomp_connection">
			<tr>
				<th width="30%">Select:</th>
				<td width="70%">
					<select name="stomp_connection_instnum" id="stomp_connection_instnum" onChange="view_stomp_connection();"></select>
					<input class="link_bg" type="submit" id="stomp_connection_delete" value="Delete" onClick="return stomp_connection_del();">
				</td>
			</tr>
			<tr>
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="stomp_connection_enable" id="stomp_connection_enable" value="1"></td>
			</tr>
			<tr>
				<th width="30%">Host:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_host" id="stomp_connection_host" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Port:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_port" id="stomp_connection_port" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">Username:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_username" id="stomp_connection_username" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Password:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_password" id="stomp_connection_password" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Heartbeat:</th>
				<td width="70%"><input type="checkbox" name="stomp_connection_heartbeat" id="stomp_connection_heartbeat" value="1"></td>
			</tr>
			<tr>
				<th width="30%">Outgoing Heartbeat:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_outgoingheartbeat" id="stomp_connection_outgoingheartbeat" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">Incoming Heartbeat:</th>
				<td width="70%">
					<input type="text" name="stomp_connection_incomingheartbeat" id="stomp_connection_incomingheartbeat" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">Encryption:</th>
				<td width="70%"><input type="checkbox" name="stomp_connection_encryption" id="stomp_connection_encryption" value="1"></td>
			</tr>
		</table>
	</div>
	<div id="div_stomp_connection_apply" class="btn_ctl">
		<input class="link_bg" type="submit" value="Apply Changes" onClick="return stomp_connection_save();">
	</div>
	<div id="div_mqtt_client_config" class="data_common data_common_notitle">
		<div class="column_title">
			<div class="column_title_left"></div>
			<p>MQTT Client</p>
			<div class="column_title_right"></div>
		</div>
		<table id="table_mqtt_client">
			<tr>
				<th width="30%">Select:</th>
				<td width="70%">
					<select name="mqtt_client_instnum" id="mqtt_client_instnum" onChange="view_mqtt_client();"></select>
					<input class="link_bg" type="submit" id="mqtt_client_delete" value="Delete" onClick="return mqtt_client_del();">
				</td>
			</tr>
			<tr>
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="mqtt_client_enable" id="mqtt_client_enable" value="1"></td>
			</tr>
			<tr>
				<th width="30%">BrokerAddress:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_brokeraddress" id="mqtt_client_brokeraddress" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">BrokerPort:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_brokerport" id="mqtt_client_brokerport" size="30" maxlength="64">
				</td>
			</tr>
			<tr>
				<th width="30%">Username:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_username" id="mqtt_client_username" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Password:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_password" id="mqtt_client_password" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Client ID:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_clientid" id="mqtt_client_clientid" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Keep Alive Time:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_keepalivetime" id="mqtt_client_keepalivetime" size="30" maxlength="256">
				</td>
			</tr>
			<tr>
				<th width="30%">Protocol Version:</th>
				<td width="70%">
					<select name="mqtt_client_protocol_version" id="mqtt_client_protocol_version">
						<option value="0">3.1</option>
						<option value="1">3.1.1</option>
						<option value="2">5.0</option>
					</select>
				</td>
			</tr>
			<tr>
				<th width="30%">Transport Protocol:</th>
				<td width="70%">
					<select name="mqtt_client_transport_protocol" id="mqtt_client_transport_protocol">
						<option value="0">TCP/IP</option>
						<option value="1">TLS</option>
					</select>
				</td>
			</tr>
			<tr>
				<th width="30%">Subscription:</th>
				<td width="70%">
					<select name="mqtt_client_subscription_instnum" id="mqtt_client_subscription_instnum" onChange="view_mqtt_client_subscription();"></select>
					<input class="link_bg" type="submit" id="mqtt_client_subscription_delete" value="Delete" onClick="return mqtt_client_subscription_del();">
				</td>
			</tr>
			<tr id="tr_mqtt_client_subscription_enable">
				<th width="30%">Enable:</th>
				<td width="70%"><input type="checkbox" name="mqtt_client_subscription_enable" id="mqtt_client_subscription_enable" value="1"></td>
			</tr>
			<tr id="tr_mqtt_client_subscription_topic">
				<th width="30%">Topic:</th>
				<td width="70%">
					<input type="text" name="mqtt_client_subscription_topic" id="mqtt_client_subscription_topic" size="30" maxlength="256">
				</td>
			</tr>
			<tr id="tr_mqtt_client_subscription_qos">
				<th width="30%">QoS:</th>
				<td width="70%">
					<select name="mqtt_client_subscription_qos" id="mqtt_client_subscription_qos">
						<option value="0">0</option>
						<option value="1">1</option>
						<option value="2">2</option>
					</select>
				</td>
			</tr>
		</table>
	</div>
	<div id="div_mqtt_client_apply" class="btn_ctl">
		<input class="link_bg" type="submit" value="Apply Changes" onClick="return mqtt_client_save();">
	</div>
	<input type="hidden" name="tr369_target" value="">
	<input type="hidden" name="tr369_action" value="0">
	<input type="hidden" value="/tr369.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</form>
</body>
</html>
