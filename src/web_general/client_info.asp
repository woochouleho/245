<%SendWebHeadStr(); %>
<title><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></title>
<!--<link rel="stylesheet" href="admin/content.css">-->

<SCRIPT>
var brip = '<% getInfo("lan-dhcp-gateway"); %>';
var subnet = '<% getInfo("lan-dhcpSubnetMask"); %>';

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.formClientTbl.postSecurityFlag, document.formClientTbl);
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
		postTableEncrypt(document.formClientTbl.postSecurityFlag, document.formClientTbl);
		document.getElementById('formClientTbl').submit();
	}
	return true;
}

function addClick(obj)
{
	if (isAttackXSS(document.nickname.mac.value)) {
		alert("ERROR: Please check the value again");
		document.nickname.mac.focus();
		return false;
	}

	/* checkMac */
	if (checkMacIncColon(document.nickname.mac, 1) == false) {
	   alert("<% multilang(LANG_INVALID_MAC_ADDR); %>");
	   document.nickname.mac.focus();
	   return false;
	}

	if (isAttackXSS(document.nickname.nickname.value)) {
		alert("ERROR: Please check the value again");
		document.nickname.nickname.focus();
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.nickname.postSecurityFlag, document.nickname);

	return true;
}

function modifyClick(obj)
{
	if (isAttackXSS(document.nickname.mac.value)) {
		alert("ERROR: Please check the value again");
		document.nickname.mac.focus();
		return false;
	}

	/* checkMac */
	if (checkMacIncColon(document.nickname.mac, 1) == false) {
	   alert("<% multilang(LANG_INVALID_MAC_ADDR); %>");
	   document.nickname.mac.focus();
	   return false;
	}

	if (isAttackXSS(document.nickname.nickname.value)) {
		alert("ERROR: Please check the value again");
		document.nickname.nickname.focus();
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.nickname.postSecurityFlag, document.nickname);

	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.nickname.postSecurityFlag, document.nickname);
		return true;
	}
}

function setMac(){
	with ( document.nickname ) 
	{   
		if(document.nickname.selectMac.value != 'NONE')
		{
			document.nickname.mac.value = document.nickname.selectMac.value;
			document.nickname.mac.focus();
		}
	}

	return true;
}

</SCRIPT>
</head>

<body>
<form action=/boaform/admin/formReflashClientTbl method=POST name="formClientTbl" id="formClientTbl">

<div class="intro_main ">
	<p class="intro_title">접속 단말 정보</p>
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
		<% dhcpClientListWithNickname("onlyAP"); %>
	</table>
</div>
<br>

<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></p>
	<p class="intro_content"><% multilang(LANG_THIS_TABLE_SHOWS_THE_ASSIGNED_IP_ADDRESS_MAC_ADDRESS_AND_TIME_EXPIRED_FOR_EACH_DHCP_LEASED_CLIENT); %></p>
</div>
<div class="data_common data_vertical">
	<table>
		<tr>
			<th width="20%"><% multilang(LANG_MAC_ADDRESS); %></th>
			<th width="15%"><% multilang(LANG_NICKNAME); %></th>
			<th width="15%"><% multilang(LANG_IP_ADDRESS); %></th>
			<th width="30%"><% multilang(LANG_HOSTNAME); %></th>
			<th width="10%"><% multilang(LANG_PORT); %></th>
			<th width="10%"><% multilang(LANG_EXPIRED_TIME_SEC); %></th>
		</tr>
		<% dhcpClientListWithNickname("all"); %>
	</table>
</div>

<div class="btn_ctl">
	<input type="hidden" name="ip_address" id="ip_address">
	<input type="hidden" name="ap_ip_address" id="ap_ip_address">
	<input type="hidden" value="/admin/client_info.asp" name="submit-url">
	<input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit(this)" class="link_bg">&nbsp;&nbsp;
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br>

<form action=/boaform/admin/formNickname method=POST name="nickname" id="nickname">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_NICKNAME); %> <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content"></p>
</div>
<div class="data_common data_common_notitle">
    <table>
        <tr>
          <th width="30%">단말찾기</th>
		  <td>
            <select name='selectMac' onClick='setMac()' size='1'>
                <option value='NONE'>선택 안함
                <% getConnectHostList(); %>
            </select>
		  </td>
        </tr>

        <tr>
          <th width="30%"><% multilang(LANG_MAC); %></th>
          <td width="70%"><input type="text" name="mac" size="15" maxlength="17"></td>
        </tr>
        <tr>
          <th width="30%"><% multilang(LANG_NICKNAME); %></th>
          <td width="70%"><input type="text" name="nickname" size="15" maxlength="30"></td>
        </tr>
        <input type="hidden" value="" name="select_id">
    </table>
</div>
<div class="btn_ctl">
    <input class="link_bg" type="submit" value="<% multilang(LANG_ADD); %>" name="addNickname" onClick="return addClick(this)">&nbsp;&nbsp;
    <input class="link_bg" type="submit" value="<% multilang(LANG_MODIFY); %>" name="modNickname" onClick="return modifyClick(this)">&nbsp;&nbsp;
    <input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delNickname" onClick="return deleteClick(this)">&nbsp;&nbsp;
</div>
<div class="column">
    <div class="column_title">
        <div class="column_title_left"></div>
        <p><% multilang(LANG_NICKNAME_TABLE); %></p>
        <div class="column_title_right"></div>
    </div>
    <div class="data_common data_vertical">
        <table>
            <% ShowNicknameTable(); %>
        </table>
    </div>
</div>

<div class="column">
	<input type="hidden" value="/admin/client_info.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>

</form>
<br><br>
</body>
</html>
