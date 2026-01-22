<%SendWebHeadStr(); %>
<TITLE><% multilang(LANG_SPEED_LIMIT); %></TITLE>
<SCRIPT language="javascript" type="text/javascript">


var iffs = new it_nr("");
<% initQosLanif(); %>

var rule_mode_up = 0;
var rule_mode_down = 0;
var rule_intf_up = new Array();
var rule_intf_down = new Array();
var rule_vlan_up = new Array();
var rule_vlan_down = new Array();
var rule_ip_up = new Array();
var rule_ip_down = new Array();
var rule_mac_up = new Array();
var rule_mac_down = new Array();
<% initQosSpeedLimitRule(); %>
var rtk_limit_speed_unit_one = <% checkWrite("limit_speed_unit_one"); %>;

function rule_display(table_id, rule_array)
{
	with (document.forms[0])
	{
		var table_element = document.getElementById(table_id);
		if (table_element.rows) {
			while (table_element.rows.length > 1) {
				table_element.deleteRow(1);
			}
		}

		for (var i = 0; i < rule_array.length; i++) {
			var row = table_element.insertRow(i + 1);
			row.align = "center";

			var cell = row.insertCell(0);
			if (rule_array == rule_intf_up || rule_array == rule_intf_down) {
				cell.innerHTML = iffs[rule_array[i].if_id];
			}
			else if (rule_array == rule_vlan_up || rule_array == rule_vlan_down) {
				cell.innerHTML = rule_array[i].vlan;
			}
			else if (rule_array == rule_ip_up || rule_array == rule_ip_down) {
				cell.innerHTML = rule_array[i].ip_start + " - " + rule_array[i].ip_end;
			}
			else if (rule_array == rule_mac_up || rule_array == rule_mac_down) {
				cell.innerHTML = rule_array[i].mac;
			}

			cell = row.insertCell(1);
			cell.innerHTML = rule_array[i].speed_unit;

			if (rule_array == rule_intf_up) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showInterfaceEdit('up', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showInterfaceEdit('up', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_intf_down) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showInterfaceEdit('down', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showInterfaceEdit('down', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_vlan_up) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showVlanTagEdit('up', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showVlanTagEdit('up', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_vlan_down) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showVlanTagEdit('down', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showVlanTagEdit('down', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_ip_up) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showIPEdit('up', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showIPEdit('up', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_ip_down) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showIPEdit('down', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"btndeleup\" onclick=\"showIPEdit('down', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_mac_up) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showMACEdit('up', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"BtnDel\" onclick=\"showMACEdit('up', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
			else if (rule_array == rule_mac_down) {
				cell = row.insertCell(2);
				cell.innerHTML = "<input type=\"button\" class=\"btnsaveup\" onclick=\"showMACEdit('down', 'edit', "+ rule_array[i].idx +")\" value=\"Edit\">";

				cell = row.insertCell(3);
				cell.innerHTML = "<input type=\"button\" class=\"btndeleup\" onclick=\"showMACEdit('down', 'del', "+ rule_array[i].idx +")\" value=\"<% multilang(LANG_DELETE); %>\">";
			}
		}
	}
}

function rule_edit_display(rule_array, rule_idx)
{
	with (document.forms[0])
	{
		for (var i = 0; i < rule_array.length; i++) {
			if (rule_array[i].idx == rule_idx) {
				if (rule_array == rule_intf_up) {
					setSelect("InterfaceName_up", rule_array[i].if_id);
					InterfaceSpeed_up.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_intf_down) {
					setSelect("InterfaceName_down", rule_array[i].if_id);
					InterfaceSpeed_down.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_vlan_up) {
					VlanTagValue_up.value = rule_array[i].vlan;
					VlanTagSpeed_up.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_vlan_down) {
					VlanTagValue_down.value = rule_array[i].vlan;
					VlanTagSpeed_down.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_ip_up) {
					IP_Start_up.value = rule_array[i].ip_start;
					IP_End_up.value = rule_array[i].ip_end;
					IPSpeed_up.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_ip_down) {
					IP_Start_down.value = rule_array[i].ip_start;
					IP_End_down.value = rule_array[i].ip_end;
					IPSpeed_down.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_mac_up) {
					MAC_up.value = rule_array[i].mac;
					MACSpeed_up.value = rule_array[i].speed_unit;
				}
				else if (rule_array == rule_mac_down) {
					MAC_down.value = rule_array[i].mac;
					MACSpeed_down.value = rule_array[i].speed_unit;
				}
				return;
			}
		}
	}
}

function on_init()
{
	with (document.forms[0])
	{
		setSelect("ModeSwitch_up", rule_mode_up);
		ModeChange('up');

		setSelect("ModeSwitch_down", rule_mode_down);
		ModeChange('down');

		rule_display('InterfaceTable_up', rule_intf_up);
		rule_display('VlanTagTable_up', rule_vlan_up);
		rule_display('IPTable_up', rule_ip_up);
		rule_display('MACTable_up', rule_mac_up);
		rule_display('InterfaceTable_down', rule_intf_down);
		rule_display('VlanTagTable_down', rule_vlan_down);
		rule_display('IPTable_down', rule_ip_down);
		rule_display('MACTable_down', rule_mac_down);

		var intf_select_up = document.getElementById('InterfaceName_up');
		var intf_select_down = document.getElementById('InterfaceName_down');
		for (var i in iffs)
		{
			if (i == "name" || (typeof iffs[i] != "string" && typeof iffs[i] != "number")) continue;
			intf_select_up.options.add(new Option(iffs[i], i));
			intf_select_down.options.add(new Option(iffs[i], i));
		}
	}
}

function UpModeChange()
{
	with (document.forms[0])
	{
		var ModeIndex = getValue('ModeSwitch_up');
		if (ModeIndex == 0)
		{
			showhide('InterfaceLimit_up', 0);
			showhide('InterfaceEdit_up', 0);
			showhide('VlanTagLimit_up', 0);
			showhide('VlanTagEdit_up', 0);
			showhide('IPLimit_up', 0);
			showhide('IPEdit_up', 0);
			showhide('MACLimit_up', 0);
			showhide('MACEdit_up', 0);
		}
		else if (ModeIndex == 1)
		{
			if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
			{ 
					setSelect("ModeSwitch_up", rule_mode_up);
					return; 
			}			
			showhide('InterfaceLimit_up', 1);
			showhide('InterfaceEdit_up', 0);
			showhide('VlanTagLimit_up', 0);
			showhide('VlanTagEdit_up', 0);
			showhide('IPLimit_up', 0);
			showhide('IPEdit_up', 0);
			showhide('MACLimit_up', 0);
			showhide('MACEdit_up', 0);
		}
		else if (ModeIndex == 2)
		{
			if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
			{ 
					setSelect("ModeSwitch_up", rule_mode_up);
					return; 
			}	
			showhide('InterfaceLimit_up', 0);
			showhide('InterfaceEdit_up', 0);
			showhide('VlanTagLimit_up', 1);
			showhide('VlanTagEdit_up', 0);
			showhide('IPLimit_up', 0);
			showhide('IPEdit_up', 0);
			showhide('MACLimit_up', 0);
			showhide('MACEdit_up', 0);
		}
		else if (ModeIndex == 3)
		{
			if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
			{ 
					setSelect("ModeSwitch_up", rule_mode_up);
					return; 
			}	
			showhide('InterfaceLimit_up', 0);
			showhide('InterfaceEdit_up', 0);
			showhide('VlanTagLimit_up', 0);
			showhide('VlanTagEdit_up', 0);
			showhide('IPLimit_up', 1);
			showhide('IPEdit_up', 0);
			showhide('MACLimit_up', 0);
			showhide('MACEdit_up', 0);
		}
		else if (ModeIndex == 4)
		{
			if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
			{
					setSelect("ModeSwitch_up", rule_mode_up);
					return; 
			}
			showhide('InterfaceLimit_up', 0);
			showhide('InterfaceEdit_up', 0);
			showhide('VlanTagLimit_up', 0);
			showhide('VlanTagEdit_up', 0);
			showhide('IPLimit_up', 0);
			showhide('IPEdit_up', 0);
			showhide('MACLimit_up', 1);
			showhide('MACEdit_up', 0);
		}
	}
}

function DownModeChange()
{
	with (document.forms[0])
	{
		var ModeIndex = getValue('ModeSwitch_down');
		if (ModeIndex == 0)
		{
			showhide('InterfaceLimit_down', 0);
			showhide('InterfaceEdit_down', 0);
			showhide('VlanTagLimit_down', 0);
			showhide('VlanTagEdit_down', 0);
			showhide('IPLimit_down', 0);
			showhide('IPEdit_down', 0);
			showhide('MACLimit_down', 0);
			showhide('MACEdit_down', 0);
		}
		else if (ModeIndex == 1)
		{
			showhide('InterfaceLimit_down', 1);
			showhide('InterfaceEdit_down', 0);
			showhide('VlanTagLimit_down', 0);
			showhide('VlanTagEdit_down', 0);
			showhide('IPLimit_down', 0);
			showhide('IPEdit_down', 0);
			showhide('MACLimit_down', 0);
			showhide('MACEdit_down', 0);
		}
		else if (ModeIndex == 2)
		{
			showhide('InterfaceLimit_down', 0);
			showhide('InterfaceEdit_down', 0);
			showhide('VlanTagLimit_down', 1);
			showhide('VlanTagEdit_down', 0);
			showhide('IPLimit_down', 0);
			showhide('IPEdit_down', 0);
			showhide('MACLimit_down', 0);
			showhide('MACEdit_down', 0);
		}
		else if (ModeIndex == 3)
		{
			showhide('InterfaceLimit_down', 0);
			showhide('InterfaceEdit_down', 0);
			showhide('VlanTagLimit_down', 0);
			showhide('VlanTagEdit_down', 0);
			showhide('IPLimit_down', 1);
			showhide('IPEdit_down', 0);
			showhide('MACLimit_down', 0);
			showhide('MACEdit_down', 0);
		}
		else if (ModeIndex == 4)
		{
			showhide('InterfaceLimit_down', 0);
			showhide('InterfaceEdit_down', 0);
			showhide('VlanTagLimit_down', 0);
			showhide('VlanTagEdit_down', 0);
			showhide('IPLimit_down', 0);
			showhide('IPEdit_down', 0);
			showhide('MACLimit_down', 1);
			showhide('MACEdit_down', 0);
		}
		}
}

function ModeChange(direction)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			//var ModeIndex = document.getElementById('ModeSwitch_up').selectedIndex;
			var ModeIndex = getValue('ModeSwitch_up');
			if (ModeIndex == 0)
			{
				showhide('InterfaceLimit_up', 0);
				showhide('InterfaceEdit_up', 0);
				showhide('VlanTagLimit_up', 0);
				showhide('VlanTagEdit_up', 0);
				showhide('IPLimit_up', 0);
				showhide('IPEdit_up', 0);
				showhide('MACLimit_up', 0);
				showhide('MACEdit_up', 0);
			}
			else if (ModeIndex == 1)
			{
				if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
				{ 
						setSelect("ModeSwitch_up", rule_mode_up);
						return; 
				}			
				showhide('InterfaceLimit_up', 1);
				showhide('InterfaceEdit_up', 0);
				showhide('VlanTagLimit_up', 0);
				showhide('VlanTagEdit_up', 0);
				showhide('IPLimit_up', 0);
				showhide('IPEdit_up', 0);
				showhide('MACLimit_up', 0);
				showhide('MACEdit_up', 0);
			}
			else if (ModeIndex == 2)
			{
				if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
				{ 
						setSelect("ModeSwitch_up", rule_mode_up);
						return; 
				}	
				showhide('InterfaceLimit_up', 0);
				showhide('InterfaceEdit_up', 0);
				showhide('VlanTagLimit_up', 1);
				showhide('VlanTagEdit_up', 0);
				showhide('IPLimit_up', 0);
				showhide('IPEdit_up', 0);
				showhide('MACLimit_up', 0);
				showhide('MACEdit_up', 0);
			}
			else if (ModeIndex == 3)
			{
				if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
				{ 
						setSelect("ModeSwitch_up", rule_mode_up);
						return; 
				}	
				showhide('InterfaceLimit_up', 0);
				showhide('InterfaceEdit_up', 0);
				showhide('VlanTagLimit_up', 0);
				showhide('VlanTagEdit_up', 0);
				showhide('IPLimit_up', 1);
				showhide('IPEdit_up', 0);
				showhide('MACLimit_up', 0);
				showhide('MACEdit_up', 0);
			}
			else if (ModeIndex == 4)
			{
				if (rule_mode_up==0&&!confirm('<% multilang(LANG_SPEED_LIMIT_ALERT); %>')) 
				{
						setSelect("ModeSwitch_up", rule_mode_up);
						return; 
				}
				showhide('InterfaceLimit_up', 0);
				showhide('InterfaceEdit_up', 0);
				showhide('VlanTagLimit_up', 0);
				showhide('VlanTagEdit_up', 0);
				showhide('IPLimit_up', 0);
				showhide('IPEdit_up', 0);
				showhide('MACLimit_up', 1);
				showhide('MACEdit_up', 0);
			}
		}
		else if (direction == "down")
		{
			//var ModeIndex = document.getElementById('ModeSwitch_down').selectedIndex;
			var ModeIndex = getValue('ModeSwitch_down');
			if (ModeIndex == 0)
			{
				showhide('InterfaceLimit_down', 0);
				showhide('InterfaceEdit_down', 0);
				showhide('VlanTagLimit_down', 0);
				showhide('VlanTagEdit_down', 0);
				showhide('IPLimit_down', 0);
				showhide('IPEdit_down', 0);
				showhide('MACLimit_down', 0);
				showhide('MACEdit_down', 0);
			}
			else if (ModeIndex == 1)
			{
				showhide('InterfaceLimit_down', 1);
				showhide('InterfaceEdit_down', 0);
				showhide('VlanTagLimit_down', 0);
				showhide('VlanTagEdit_down', 0);
				showhide('IPLimit_down', 0);
				showhide('IPEdit_down', 0);
				showhide('MACLimit_down', 0);
				showhide('MACEdit_down', 0);
			}
			else if (ModeIndex == 2)
			{
				showhide('InterfaceLimit_down', 0);
				showhide('InterfaceEdit_down', 0);
				showhide('VlanTagLimit_down', 1);
				showhide('VlanTagEdit_down', 0);
				showhide('IPLimit_down', 0);
				showhide('IPEdit_down', 0);
				showhide('MACLimit_down', 0);
				showhide('MACEdit_down', 0);
			}
			else if (ModeIndex == 3)
			{
				showhide('InterfaceLimit_down', 0);
				showhide('InterfaceEdit_down', 0);
				showhide('VlanTagLimit_down', 0);
				showhide('VlanTagEdit_down', 0);
				showhide('IPLimit_down', 1);
				showhide('IPEdit_down', 0);
				showhide('MACLimit_down', 0);
				showhide('MACEdit_down', 0);
			}
			else if (ModeIndex == 4)
			{
				showhide('InterfaceLimit_down', 0);
				showhide('InterfaceEdit_down', 0);
				showhide('VlanTagLimit_down', 0);
				showhide('VlanTagEdit_down', 0);
				showhide('IPLimit_down', 0);
				showhide('IPEdit_down', 0);
				showhide('MACLimit_down', 1);
				showhide('MACEdit_down', 0);
			}
		}
	}
}

function showInterfaceEdit(direction, act, idx)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			ruleAction_up.value = act;
			ruleIndex_up.value = idx;

			if (act == "add")
			{
				showhide('InterfaceEdit_up', 1);
				InterfaceName_up.selectedIndex = 0;
				InterfaceSpeed_up.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_intf_up, idx);
				showhide('InterfaceEdit_up', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
		else if (direction == "down")
		{
			ruleAction_down.value = act;
			ruleIndex_down.value = idx;

			if (act == "add")
			{
				showhide('InterfaceEdit_down', 1);
				InterfaceName_down.selectedIndex = 0;
				InterfaceSpeed_down.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_intf_down, idx);
				showhide('InterfaceEdit_down', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
	}
}

function showVlanTagEdit(direction, act, idx)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			ruleAction_up.value = act;
			ruleIndex_up.value = idx;


			if (act == "add")
			{
				showhide('VlanTagEdit_up', 1);
				VlanTagValue_up.value = 0;
				VlanTagSpeed_up.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_vlan_up, idx);
				showhide('VlanTagEdit_up', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
		else if (direction == "down")
		{
			ruleAction_down.value = act;
			ruleIndex_down.value = idx;

			if (act == "add")
			{
				showhide('VlanTagEdit_down', 1);
				VlanTagValue_down.value = 0;
				VlanTagSpeed_down.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_vlan_down, idx);
				showhide('VlanTagEdit_down', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
	}
}

function showIPEdit(direction, act, idx)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			ruleAction_up.value = act;
			ruleIndex_up.value = idx;

			if (act == "add")
			{
				showhide('IPEdit_up', 1);
				IP_Start_up.value = 0;
				IP_End_up.value = 0;
				IPSpeed_up.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_ip_up, idx);
				showhide('IPEdit_up', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
		else if (direction == "down")
		{
			ruleAction_down.value = act;
			ruleIndex_down.value = idx;

			if (act == "add")
			{
				showhide('IPEdit_down', 1);
				IP_Start_down.value = 0;
				IP_End_down.value = 0;
				IPSpeed_down.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_ip_down, idx);
				showhide('IPEdit_down', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
	}
}

function showMACEdit(direction, act, idx)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			ruleAction_up.value = act;
			ruleIndex_up.value = idx;

			if (act == "add")
			{
				showhide('MACEdit_up', 1);
				MAC_up.value = 0;
				MACSpeed_up.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_mac_up, idx);
				showhide('MACEdit_up', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
		else if (direction == "down")
		{
			ruleAction_down.value = act;
			ruleIndex_down.value = idx;

			if (act == "add")
			{
				showhide('MACEdit_down', 1);
				MAC_down.value = 0;
				MACSpeed_down.value = 0;
			}
			else if (act == "edit")
			{
				rule_edit_display(rule_mac_down, idx);
				showhide('MACEdit_down', 1);
			}
			else if (act == "del")
			{
				if (!confirm("<% multilang(LANG_ARE_YOU_SURE_YOU_WANT_TO_DELETE); %>")) {
					return false;
				}
				ruleDirection.value = direction;
				submitAction.value = "rule";
				postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
				submit();
			}
		}
	}
}

function isInvalidSpeed(s)
{
	if(rtk_limit_speed_unit_one){
		if (isNaN(parseInt(s, 10)) || parseInt(s, 10) <= 0 || parseInt(s, 10) > 1024000) {
			return true;
		}
	}else{
		if (isNaN(parseInt(s, 10)) || parseInt(s, 10) <= 0 || parseInt(s, 10) > 2047) {
			return true;
		}
	}
	return false;
}

function isInvalidVLAN(s)
{
	if (s == "untagged") {
		return false;
	}

	if (isNaN(parseInt(s, 10)) || parseInt(s, 10) < 1 || parseInt(s, 10) > 4096) {
		return true;
	}
	return false;
}

function RuleSubmit(direction)
{
	with (document.forms[0])
	{
		if (direction == "up")
		{
			//var ModeIndex = document.getElementById('ModeSwitch_up').selectedIndex;
			var ModeIndex = getValue('ModeSwitch_up');
			if (ModeIndex == 1) // Interface
			{
				if (isInvalidSpeed(InterfaceSpeed_up.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 2) // VlanTag
			{
				if (isInvalidVLAN(VlanTagValue_up.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_VLAN_ALERT); %>");
					return false;
				}

				if (isInvalidSpeed(VlanTagSpeed_up.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 3) // IP
			{
				if((!sji_checkvip(IP_Start_up.value) && !isIPv6(IP_Start_up.value)) || (!sji_checkvip(IP_End_up.value) && !isIPv6(IP_End_up.value))){
					alert("<% multilang(LANG_SPEED_LIMIT_IP_ALERT); %>");
					return false;
				}

				if (isInvalidSpeed(IPSpeed_up.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 4) // MAC
			{
				if(!isValidMacAddress(MAC_up.value)) {
					alert("<% multilang(LANG_INVALID_MAC_ADDR); %>  XX:XX:XX:XX:XX:XX");
					return false;
				}
				
				if (isInvalidSpeed(MACSpeed_up.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else {
				return false;
			}
		}
		else if (direction == "down")
		{
			//var ModeIndex = document.getElementById('ModeSwitch_down').selectedIndex;
			var ModeIndex = getValue('ModeSwitch_down');
			if (ModeIndex == 1) // Interface
			{
				if (isInvalidSpeed(InterfaceSpeed_down.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 2) // VlanTag
			{
				if (isInvalidVLAN(VlanTagValue_down.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_VLAN_ALERT); %>");
					return false;
				}

				if (isInvalidSpeed(VlanTagSpeed_down.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 3) // IP
			{
				if((!sji_checkvip(IP_Start_down.value) && !isIPv6(IP_Start_down.value)) || (!sji_checkvip(IP_End_down.value) && !isIPv6(IP_End_down.value))){
					alert("<% multilang(LANG_SPEED_LIMIT_IP_ALERT); %>");
					return false;
				}

				if (isInvalidSpeed(IPSpeed_down.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else if (ModeIndex == 4) // MAC
			{
				if(!isValidMacAddress(MAC_down.value)) {
					alert("<% multilang(LANG_INVALID_MAC_ADDR); %>  XX:XX:XX:XX:XX:XX");
					return false;
				}
				
				if (isInvalidSpeed(MACSpeed_down.value)) {
					alert("<% multilang(LANG_SPEED_LIMIT_RANGE); %>");
					return false;
				}
			}
			else {
				return false;
			}
		}

		ruleDirection.value = direction;
		submitAction.value = "rule";
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		submit();
	}
}

function ModeSubmit()
{
	with (document.forms[0])
	{
		submitAction.value = "mode";
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		submit();
	}
}
function clickPage(filename)
{
			location.replace(filename);
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--UI-->

<body onLoad="on_init();">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_SPEED_LIMIT); %></p>
</div>

	
			<form action=/boaform/admin/formQosSpeedLimit method=POST name="form">
<!-------------------------------------------------------------------------------------->
			<div class="data_common data_common_notitle">
				<table border="0" cellpadding="2" cellspacing="0">
					<tr>
						<th><% multilang(LANG_UPLINK_SPEED_LIMIT_MODE); %>:</th>
						<td>
							<select id="ModeSwitch_up" size="1" name="ModeSwitch_up" onchange="UpModeChange()">
								<option value="0"><% multilang(LANG_SPEED_LIMIT_DISABLED); %></option>					
								<option value="1"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_INTERFACE); %></option>	
						<!--	<option value="2"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_VLAN_ID); %></option>	-->
								<option value="3"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_IP_ADDRESS); %></option>
								<option value="4"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_MAC_ADDRESS); %></option>
							</select>
						</td>
					</tr>
				</table>

				<div id="InterfaceLimit_up">
					<table  class="flat" id="InterfaceTable_up" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr>
								<th align="center" width="40%">LAN/SSID</th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showInterfaceEdit('up', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="InterfaceEdit_up">
					<table>
						<tbody>
							<tr>
								<th width="20%">LAN/SSID: </th>
								<td width="80%"><select size="1" id="InterfaceName_up" name="InterfaceName_up"></select></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="InterfaceSpeed_up" type="text" id="InterfaceSpeed_up" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('up')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>

				<div id="VlanTagLimit_up">
					<table class="flat" id="VlanTagTable_up" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%">VLAN</th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showVlanTagEdit('up', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="VlanTagEdit_up">
					<table>
						<tbody>
						<tr>
							<th width="20%">VLAN: </th>
							<td width="80%"><input name="VlanTagValue_up" type="text" id="VlanTagValue_up" value="0" size="15" maxlength="15"></td>
						</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="VlanTagSpeed_up" type="text" id="VlanTagSpeed_up" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('up')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>

				<div id="IPLimit_up">
					<table class="flat" id="IPTable_up" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%"><% multilang(LANG_IP_RANGE); %></th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showIPEdit('up', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="IPEdit_up">
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_IP_RANGE); %>: </th>
								<td width="80%">
									<input name="IP_Start_up" type="text" id="IP_Start_up" value="0" size="20" maxlength="39">-
									<input name="IP_End_up" type="text" id="IP_End_up" value="0" size="20" maxlength="39">
								</td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="IPSpeed_up" type="text" id="IPSpeed_up" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('up')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="MACLimit_up">
					<table class="flat" id="MACTable_up" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%"><% multilang(LANG_MAC_ADDRESS); %></th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showMACEdit('up', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="MACEdit_up">
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_MAC_ADDRESS); %>: </th>
								<td width="80%"><input name="MAC_up" type="text" id="MAC_up" value="0" size="17" maxlength="17"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="MACSpeed_up" type="text" id="MACSpeed_up" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('up')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
<!-------------------------------------------------------------------------------------->
				<br><br>
<!-------------------------------------------------------------------------------------->
			<div class="data_common data_common_notitle">
				<table border="0" cellpadding="2" cellspacing="0">
					<tr>
						<th><% multilang(LANG_DOWNLINK_SPEED_LIMIT_MODE); %>:</th>
						<td>
							<select id="ModeSwitch_down" size="1" name="ModeSwitch_down" onchange="DownModeChange()">
								<option value="0"><% multilang(LANG_SPEED_LIMIT_DISABLED); %></option>					
								<option value="1"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_INTERFACE); %></option>	
						<!--	<option value="2"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_VLAN_ID); %></option>	-->
								<option value="3"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_IP_ADDRESS); %></option>
								<option value="4"><% multilang(LANG_SPEED_LIMIT_BASED_ON_LOCAL_MAC_ADDRESS); %></option>
							</select>
						</td>
					</tr>
				</table>

				<div id="InterfaceLimit_down">
					<table class="flat" id="InterfaceTable_down" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr>
								<th align="center" width="40%">LAN/SSID</th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showInterfaceEdit('down', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="InterfaceEdit_down">
					<table>
						<tbody>
							<tr>
								<th width="20%">LAN/SSID: </th>
								<td width="80%"><select size="1" id="InterfaceName_down" name="InterfaceName_down"></select></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="InterfaceSpeed_down" type="text" id="InterfaceSpeed_down" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('down')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>

				<div id="VlanTagLimit_down">
					<table class="flat" id="VlanTagTable_down" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%">VLAN</th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showVlanTagEdit('down', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="VlanTagEdit_down">
					<table>
						<tbody>
						<tr>
							<th width="20%">VLAN: </th>
							<td width="80%"><input name="VlanTagValue_down" type="text" id="VlanTagValue_down" value="0" size="15" maxlength="15"></td>
						</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="VlanTagSpeed_down" type="text" id="VlanTagSpeed_down" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('down')" value="<% multilang(LANG_SUBMIT); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>

				<div id="IPLimit_down">
					<table class="flat" id="IPTable_down" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%"><% multilang(LANG_IP_RANGE); %></th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showIPEdit('down', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="IPEdit_down">
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_IP_RANGE); %>: </th>
								<td width="80%">
									<input name="IP_Start_down" type="text" id="IP_Start_down" value="0" size="20" maxlength="39">-
									<input name="IP_End_down" type="text" id="IP_End_down" value="0" size="20" maxlength="39">
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="IPSpeed_down" type="text" id="IPSpeed_down" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('down')" value="<% multilang(LANG_SUBMIT); %>">
									
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="MACLimit_down">
					<table class="flat" id="MACTable_down" border="1" cellpadding="1" cellspacing="1" width="100%">
						<tbody>
							<tr align="middle">
								<th align="center" width="40%"><% multilang(LANG_MAC_ADDRESS); %></th>
								<th align="center" width="30%"><% multilang(LANG_SPEED_LIMIT_UNIT); %></th>
								<th align="center" width="15%"><% multilang(LANG_MODIFY); %></th>
								<th align="center" width="15%"><% multilang(LANG_DELETE); %></th>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnaddup" onclick="showMACEdit('down', 'add', -1)" value="<% multilang(LANG_ADD); %>">
								</td>
							</tr>
						</tbody>
					</table>
				</div>
				<div id="MACEdit_down">
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_MAC_ADDRESS); %>: </th>
								<td width="80%"><input name="MAC_down" type="text" id="MAC_down" value="0" size="17" maxlength="17"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<th width="20%"><% multilang(LANG_DATA_RATE); %>: </th>
								<td width="80%"><input name="MACSpeed_down" type="text" id="MACSpeed_down" value="0" size="10" maxlength="10"></td>
							</tr>
						</tbody>
					</table>
					<table>
						<tbody>
							<tr>
								<td>
									<input type="button" class="btnsaveup" onclick="RuleSubmit('down')" value="<% multilang(LANG_SUBMIT); %>">
									
								</td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
<!-------------------------------------------------------------------------------------->
				<br><br><br>
			<div class="btn_ctl">
				<input type="hidden" name="submitAction" value="">
				<input type="hidden" name="ruleDirection" value="">
				<input type="hidden" name="ruleAction_up" value="">
				<input type="hidden" name="ruleIndex_up" value="">
				<input type="hidden" name="ruleAction_down" value="">
				<input type="hidden" name="ruleIndex_down" value="">
				
				<input class="link_bg" type="button" class="btnOK" value="<% multilang(LANG_APPLY_CHANGES); %>" onClick="ModeSubmit();">
				<input type="hidden" name="submit-url" value="/net_qos_data_speed_limit_ct.asp">
				<input type="hidden" name="postSecurityFlag" value="">
			</div>
			</form>
	
</body>
<%addHttpNoCache();%>
</html>
