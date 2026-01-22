<%SendWebHeadStr(); %>
<title><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></title>
<!--<link rel="stylesheet" href="admin/content.css">-->

<SCRIPT>
var brip = '<% getInfo("lan-dhcp-gateway"); %>';
var subnet = '<% getInfo("lan-dhcpSubnetMask"); %>';

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	window.location.reload();
	return true;
}

function apconClickLogin(ipaddress) {
	if((window.location.hostname == "wifi.skbroadband.com") 
		|| (window.location.hostname == "wifi.skbroadband.co.kr") 
		|| (window.location.hostname == "192.168.45.1"))
	{
		var strUrl = 'http://' + ipaddress + ':8080';
		var win = window.open(strUrl,'_blank');
		win.focus();
	}
	else if(isLocalIpAddress(window.location.hostname, brip, subnet) == true)
	{
		var strUrl = 'http://' + ipaddress + ':8080';
		var win = window.open(strUrl,'_blank');
		win.focus();
	}
	else
	{
		document.getElementById('ip_address').value = ipaddress;
		document.getElementById('ap_ip_address').value = window.location.hostname;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		document.getElementById('formClientTbl').submit();
	}
	return true;
}
</SCRIPT>
</head>

<body>
<form action=/boaform/formReflashClientTbl method=POST name="formClientTbl" id="formClientTbl">
<div class="intro_main ">
	<p class="intro_title">AP DHCP Clients</p>
	<p class="intro_content"><% multilang(LANG_THIS_TABLE_SHOWS_THE_ASSIGNED_IP_ADDRESS_MAC_ADDRESS_AND_TIME_EXPIRED_FOR_EACH_DHCP_LEASED_CLIENT); %></p>
</div>
<div class="data_common data_vertical">
	<table>
		<tr>
			<th width="25%"><% multilang(LANG_HOSTNAME); %></th>
			<th width="20%"><% multilang(LANG_SSID); %></th>
			<th width="15%"><% multilang(LANG_IP_ADDRESS); %></th>
			<th width="25%"><% multilang(LANG_MAC_ADDRESS); %></th>
			<th width="5%"><% multilang(LANG_PORT); %></th>
			<th width="5%"><% multilang(LANG_EXPIRED_TIME_SEC); %></th>
			<th width="5%"><% multilang(LANG_CONNECT); %></th>

		</tr>
		<% dhcpClientList("onlyAP"); %>
	</table>
</div>

<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></p>
	<p class="intro_content"><% multilang(LANG_THIS_TABLE_SHOWS_THE_ASSIGNED_IP_ADDRESS_MAC_ADDRESS_AND_TIME_EXPIRED_FOR_EACH_DHCP_LEASED_CLIENT); %></p>
</div>
<div class="data_common data_vertical">
	<table>
		<tr>
			<th width="30%"><% multilang(LANG_MAC_ADDRESS); %></th>
			<th width="20%"><% multilang(LANG_IP_ADDRESS); %></th>
			<th width="30%"><% multilang(LANG_HOSTNAME); %></th>
			<th width="10%"><% multilang(LANG_PORT); %></th>
			<th width="10%"><% multilang(LANG_EXPIRED_TIME_SEC); %></th>
		</tr>
		<% dhcpClientList("all"); %>
	</table>
</div>

<div class="btn_ctl">
	<input type="hidden" name="ip_address" id="ip_address">
	<input type="hidden" name="ap_ip_address" id="ap_ip_address">
	<input type="hidden" value="/dhcptbl.asp" name="submit-url">
	<input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit(this)" class="link_bg">&nbsp;&nbsp;
	<input type="button" value="<% multilang(LANG_CLOSE); %>" name="close" onClick="javascript: window.close();" class="link_bg">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
