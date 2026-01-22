<%SendWebHeadStr(); %>
<title>SNMP <% multilang(LANG_CONFIGURATION); %></title>

<style>
th{
    text-align: left;
}
</style>
<SCRIPT>

function saveChanges(obj)
{
	if (!isURLOrIPWithoutProtocol(document.snmpTable.snmpTrapIpAddr))
		return false;
 
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function init()
{
}
</SCRIPT>

</head>

<body onload="init()">
<div class="intro_main ">
	<p class="intro_title">SNMP <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content" style="display:none"> <% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIGURE_THE_SNMP_HERE_YOU_MAY_CHANGE_THE_SETTINGS_FOR_SYSTEM_DESCRIPTION_TRAP_IP_ADDRESS_COMMUNITY_NAME_ETC); %></p>
</div>

<form action=/boaform/formSnmpConfig method=POST name="snmpTable">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th>SNMP:</th>
			<td>
				<input type="radio" value="0" name="snmp_enable" <% checkWrite("snmpd-on"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="snmp_enable" <% checkWrite("snmpd-off"); %> ><% multilang(LANG_ENABLE); %>&nbsp;&nbsp;
			</td>     
		</tr> 
		<tr>
			<th>Community1 name:</th>
			<td>
				<input type="radio" value="0" name="snmpCommunityRO_enable" <% checkWrite("snmpCommunityRO-on"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="snmpCommunityRO_enable" <% checkWrite("snmpCommunityRO-off"); %> ><% multilang(LANG_ENABLE); %>&nbsp;&nbsp;
			</td>
		</tr>
		<tr>
			<th>Community2 name:</th>
			<td>
				<input type="radio" value="0" name="snmpCommunityRW_enable" <% checkWrite("snmpCommunityRW-on"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="snmpCommunityRW_enable" <% checkWrite("snmpCommunityRW-off"); %> ><% multilang(LANG_ENABLE); %>&nbsp;&nbsp;
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_TRAP_IP_ADDRESS); %>:</th>
			<td><input type="text" name="snmpTrapIpAddr" size="50" maxlength="64" value=<% getInfo("snmpTrapIpAddr"); %>></td>
		</tr>
	</table>
</div>

<div class="btn_ctl">
    <input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges(this)">&nbsp;&nbsp;
    <input type="hidden" value="/snmpv3.asp" name="submit-url">
    <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
</body>

</html>
