<% SendWebHeadStr();%>
<title><% multilang(LANG_WLAN_EASY_MESH_INTERFACE_SETUP); %></title>
<style>
.on {display:on}
.off {display:none}
</style>

<SCRIPT>

var role=<% getInfo("multi_ap_controller"); %>;

var backhaul_radio=<% checkWrite("backhaul_link"); %>;
var is_agent_enabled = <% checkWrite("is_agent_enabled"); %>;

function wpsTrigger(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

    return true;
}

function loadInfo()
{
	//load role from mib and set radio button accordingly
	if (role == 0) {
		document.getElementById("role_disabled").checked = true;
	} else if (role == 1) {
		document.getElementById("role_controller").checked = true;
		document.getElementById("wsc_trigger").innerHTML   = '<th width="30%">'+'<% multilang(LANG_WLAN_WPS_TRIGGER); %>'+':</th><td width="70%"><input type="submit" value="'+'<%multilang(LANG_START_PBC); %>'+'" class="link_bg" name="start_wsc" onClick="return wpsTrigger(this)"></td>';
	} else if ((is_agent_enabled == 1) && (role == 2)) {
		document.getElementById("role_agent").checked = true;
		document.getElementById("wsc_trigger").innerHTML   = '<th width="30%">'+'<% multilang(LANG_WLAN_WPS_TRIGGER); %>'+':</th><td width="70%"><input type="submit" value="'+'<%multilang(LANG_START_PBC); %>'+'" class="link_bg" name="start_wsc" onClick="return wpsTrigger(this)"></td>';
	}

	//save role into prev_role
	if (role == 0) {
		document.getElementById("role_prev").value = "disabled";
	} else if (role == 1) {
		document.getElementById("role_prev").value = "controller";
	} else if ((is_agent_enabled == 1) && (role == 2)) {
		document.getElementById("role_prev").value = "agent";
	}

	if(is_agent_enabled == 1){
		document.getElementById("agent_div").style.display = "inline-block";
	}

	document.getElementById("controller_backhaul").style.display = "none";
	document.getElementById("agent_backhaul").style.display = "none";

	if (role) {
		if (backhaul_radio == 1) {
			document.getElementById("controller_backhaul_wlan1").checked = true;
			document.getElementById("agent_backhaul_wlan1").checked = true;
		} else if (backhaul_radio == 2) {
			document.getElementById("controller_backhaul_wlan2").checked = true;
			document.getElementById("agent_backhaul_wlan2").checked = true;
		} else {
			document.getElementById("controller_backhaul_wlan0").checked = true;
			document.getElementById("agent_backhaul_wlan0").checked = true;
		}

		if (role == 1) {
			document.getElementById("controller_backhaul").style.display = "";
			document.getElementById("agent_backhaul").style.display = "none";
		} else if ((is_agent_enabled == 1) && (role == 2)) {
			document.getElementById("controller_backhaul").style.display = "none";
			document.getElementById("agent_backhaul").style.display = "";
		}
	}
}

function resetClick()
{
	location.reload(true);
}

function saveChanges(obj)
{
	if (!document.getElementById("role_disabled").checked) {
		if ("" == document.getElementById("device_name_text").value) {
			alert("<% multilang(LANG_WLAN_EASY_MESH_DEVICE_NAME_CANNOT_BE_EMPTY); %>");
			return false;
		}

		if (document.getElementById("device_name_text").value.length > 32) {
			alert("<% multilang(LANG_WLAN_EASY_MESH_DEVICE_NAME_SHOULD_BE_LESS_OR_EQUAL_TO_32_CHARACTERS); %>");
			return false;
		}
	}

	var dot11kvDisabled = <% checkWrite("is_dot11kv_disabled"); %>;
	if (dot11kvDisabled && (!document.getElementById("role_disabled").checked)) {
		if(!confirm("<% multilang(LANG_WLAN_EASY_MESH_11KV_ENABLE_WARNING_MESSAGE); %>")){
			return false;
		}
		document.getElementById("needEnable11kv").value = "1";
	}

	//var securitySettingWrong = <% checkWrite("is_security_setting_wrong"); %>;
	//if (securitySettingWrong && (role == 1)) {
	//	alert("<% multilang(LANG_WLAN_EASY_MESH_SECURTITY_WRONG_MESSAGE); %>");
	//	return false;
	//}

	if (<% checkWrite("needPopupBackhaul"); %> && (role == 1)) {
		if(!confirm("<% multilang(LANG_WLAN_EASY_MESH_ALERT_VAP1_AUTO_MANAGED_MESSAGE); %>")){
			return false;
		}
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

    return true;
}

function isControllerOnChange(){
	if (document.getElementById("role_controller").checked == true) {
		role = 1;
		document.getElementById("controller_backhaul").style.display = "";
		document.getElementById("agent_backhaul").style.display = "none";
	} else if ((is_agent_enabled == 1) && (document.getElementById("role_agent").checked == true)){
		role = 2;
		document.getElementById("controller_backhaul").style.display = "none";
		document.getElementById("agent_backhaul").style.display = "";
	} else {
		role = 0;
		document.getElementById("controller_backhaul").style.display = "none";
		document.getElementById("agent_backhaul").style.display = "none";
	}

	if (backhaul_radio == 1) {
		document.getElementById("controller_backhaul_wlan1").checked = true;
		document.getElementById("agent_backhaul_wlan1").checked = true;
	} else if (backhaul_radio == 2) {
		document.getElementById("controller_backhaul_wlan2").checked = true;
		document.getElementById("agent_backhaul_wlan2").checked = true;
	} else {
		document.getElementById("controller_backhaul_wlan0").checked = true;
		document.getElementById("agent_backhaul_wlan0").checked = true;
	}
}

function isBackhaulOnChange(){
	if(role == 1) {
		if (document.getElementById("controller_backhaul_wlan2") && document.getElementById("controller_backhaul_wlan2").checked == true) {
			backhaul_radio = 2;
		} else if (document.getElementById("controller_backhaul_wlan1").checked == true){
			backhaul_radio = 1;
		} else {
			backhaul_radio = 0;
		}
	} else if(role == 2) {
		if (document.getElementById("agent_backhaul_wlan2") && document.getElementById("agent_backhaul_wlan2").checked == true) {
			backhaul_radio = 2;
		} else if (document.getElementById("agent_backhaul_wlan1").checked == true){
			backhaul_radio = 1;
		} else {
			backhaul_radio = 0;
		}
	}
}

</SCRIPT>
</head>

<body onload="loadInfo();">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WLAN_EASY_MESH_INTERFACE_SETUP); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_EASY_MESH_DESC); %></p>
</div>

<form action=/boaform/formMultiAP method=POST name="MultiAP">
<div class="data_common data_common_notitle">
<table>

	<tr id="device_name">
		<th width="30%"><% multilang(LANG_DEVICE_NAME); %>:</th>
		<td width="70%">
			<input type="text" id="device_name_text" name="device_name_text" value="<% getInfo("map_device_name"); %>">
		</td>
	</tr>

	<tr id="is_controller">
		<th width="30%"><% multilang(LANG_ROLE); %>:</th>
		<td width="70%">
		<input type="radio" id="role_controller" name="role" value="controller" onclick="isControllerOnChange()"><% multilang(LANG_CONTROLLER); %>&nbsp;&nbsp;
		<div id="agent_div" style="display:none">
		<input type="radio" id="role_agent" name="role" value="agent" onclick="isControllerOnChange()"><% multilang(LANG_AGENT); %>&nbsp;&nbsp;
		</div>
		<input type="radio" id="role_disabled" name="role" value="disabled" onclick="isControllerOnChange()"><% multilang(LANG_DISABLED); %></td>
	</tr>

	<% showBackhaulSelection(); %>

	<tr id="wsc_trigger">

	</tr>


  </table>

	<table style="display:none;" id="staticIpTable" border="0" width=640>
		<% dhcpRsvdIp_List();%>
	</table>
</div>
<div class="btn_ctl">
      <input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" class="link_bg" name="save_apply" onClick="return saveChanges(this)">&nbsp;&nbsp;
      <input type="reset" value="<% multilang(LANG_RESET); %>" class="link_bg" name="reset" onClick="resetClick()">

	  <input type="hidden" value="/multi_ap_setting_general.asp" name="submit-url">
	  <input type="hidden" value="0" name="needEnable11kv" id="needEnable11kv">
	  <input type="hidden" value="<% getIndex("needPopupBackhaul"); %>" name="needPopupBackhaul">
	  <input type="hidden" value="" name="role_prev" id="role_prev">
	  <input type="hidden" name="postSecurityFlag" value="">

 </div>
 </form>
</body>

</html>
