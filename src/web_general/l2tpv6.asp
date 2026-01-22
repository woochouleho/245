<%SendWebHeadStr(); %>
<title>L2TP VPN <% multilang(LANG_CONFIGURATION); %></title>

<script type="text/javascript" src="base64_code.js"></script>
<script language="javascript">

var L2TPOverIPSec=<% checkWrite("L2TPOverIPSec"); %>

function checkLacOverIPSec(){
	if(L2TPOverIPSec){
		if(document.l2tp.overipsecLAC.checked){
			if(document.l2tp.IKEPreshareKeyLAC.value == ""){
				alert("IKEPreshareKey cannot be empty in PSK mode");
				document.l2tp.IKEPreshareKeyLAC.focus();
				return false;
			}
			else if((document.l2tp.IKEPreshareKeyLAC.value.length < 8) || (document.l2tp.IKEPreshareKeyLAC.value.length  >128)){
				alert("IKEPreshareKey character length is8~128");
				document.l2tp.IKEPreshareKeyLAC.focus();
				return false;
			}

			if(!checkString(document.l2tp.IKEPreshareKeyLAC.value))
			{
				alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
				document.l2tp.IKEPreshareKeyLAC.focus();
				return false;
			}
			document.l2tp.encodepskValueLAC.value = encode64(document.l2tp.IKEPreshareKeyLAC.value);
			document.l2tp.IKEPreshareKeyLAC.disabled=true;
		}
	}

	return true;
}

function checkLnsOverIPSec(){
	if(L2TPOverIPSec){
		if(document.l2tp.overipsecLNS.checked){
			if(document.l2tp.IKEPreshareKeyLNS.value == ""){
				alert("IKEPreshareKey cannot be empty in PSK mode");
				document.l2tp.IKEPreshareKeyLNS.focus();
				return false;
			}
			else if((document.l2tp.IKEPreshareKeyLNS.value.length < 8) || (document.l2tp.IKEPreshareKeyLNS.value.length  >128)){
				alert("IKEPreshareKey character length is8~128");
				document.l2tp.IKEPreshareKeyLNS.focus();
				return false;
			}

			if(!checkString(document.l2tp.IKEPreshareKeyLNS.value))
			{
				alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
				document.l2tp.IKEPreshareKeyLNS.focus();
				return false;
			}
			document.l2tp.encodepskValueLNS.value = encode64(document.l2tp.IKEPreshareKeyLNS.value);
			document.l2tp.IKEPreshareKeyLNS.disabled=true;
		}
	}

	return true;
}

function onClickLacOverIPSec()
{
	if(L2TPOverIPSec){
		if (document.l2tp.overipsecLAC.checked)
			enableTextField(document.l2tp.IKEPreshareKeyLAC);
		else
			disableTextField(document.l2tp.IKEPreshareKeyLAC);
	}
}

function onClickLnsOverIPSec()
{
	if(L2TPOverIPSec){
		if (document.l2tp.overipsecLNS.checked)
			enableTextField(document.l2tp.IKEPreshareKeyLNS);
		else
			disableTextField(document.l2tp.IKEPreshareKeyLNS);
	}
}

function checkTextStr(str)
{
	for (var i=0; i<str.length; i++) 
	{
		if ( str.charAt(i) == '%' || str.charAt(i) =='&' ||str.charAt(i) =='\\' || str.charAt(i) =='?' || str.charAt(i)=='"') 
			return false;			
	}
	return true;
}

function pppAuthChange()
{
	if (document.l2tp.auth.value==3)
	{
		document.l2tp.enctype.disabled = false;
	}
	else
	{
		document.l2tp.enctype.disabled = true;
	}
}

function pppAuthChange_lns()
{
	if (document.l2tp.s_auth.value==3) {
		document.l2tp.s_enctype.disabled = false;
	}else
		document.l2tp.s_enctype.disabled = true;
}

function onClickTunnelAuth()
{
	if (document.l2tp.tunnel_auth.checked)
		document.l2tp.tunnel_secret.disabled = false;
	else
		document.l2tp.tunnel_secret.disabled = true;
}

function onClickTunnelAuth_lns()
{
	if(document.l2tp.s_tunnelAuth.checked)
		document.l2tp.s_authKey.disabled = false;
	else
		document.l2tp.s_authKey.disabled = true;
}

function setL2tpServer(obj)
{
	if(document.l2tp.l2tpen[0].checked)
		return false;

	if (document.l2tp.s_name.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_RULE_NAME); %>');
		document.l2tp.s_name.focus();
		return false;
	}

	if (document.l2tp.peeraddr.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_PEER_START_ADDRESS); %>');
		document.l2tp.peeraddr.focus();
		return false;
	}
	if (!checkHostIP(document.l2tp.peeraddr, 0))
		return false;

	if (document.l2tp.localaddr.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_LOCAL_ADDRESS); %>');
		document.l2tp.localaddr.focus();
		return false;
	}
	if (!checkHostIP(document.l2tp.localaddr, 0))
		return false;

	if(false == checkLnsOverIPSec()){
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function addL2tpAccount(obj)
{
	if(document.l2tp.l2tpen[0].checked)
		return false;

	if (document.l2tp.s_accout_name.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_RULE_NAME); %>');
		document.l2tp.s_accout_name.focus();
		return false;
	}

	if(document.l2tp.s_tunnelAuth.checked){
		if(document.l2tp.s_authKey.value ==""){
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_AUTHKEY); %>');
			document.l2tp.s_authKey.focus();
			return false;
		}
		if(!checkTextStr(document.l2tp.s_authKey))
		{
			return false;
		}
	}

	if (document.l2tp.s_username.value=="")
	{
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_USERNAME); %>');
		document.l2tp.s_username.focus();
		return false;
	}
	if(!checkTextStr(document.l2tp.s_username))
	{
		return false;
	}

	if (document.l2tp.s_password.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_PASSWORD); %>');
		document.l2tp.s_password.focus();
		return false;
	}

	if (includeSpace(document.l2tp.s_password.value)) {
		alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD_PLEASE_TRY_IT_AGAIN); %>');
		document.l2tp.s_password.focus();
		return false;
	}
	if(!checkString(document.l2tp.s_password.value))
	{
		alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
		document.l2tp.s_password.focus();
		return false;
	}
	document.l2tp.encodeSPassword.value=encode64(document.l2tp.s_password.value);
	document.l2tp.s_password.disabled=true;

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function connTypeChange()
{
	if(document.l2tp.pppconntype.selectedIndex==1)
	{
		document.l2tp.idletime.disabled=false;
	}else{
		document.l2tp.idletime.disabled=true;
	}
}

function l2tpSelection()
{
	if (document.l2tp.l2tpen[0].checked) {
		/* LAC */
		document.l2tp.c_name.disabled = true;
		document.l2tp.IpProtocolType.disabled = true;
		document.l2tp.server.disabled = true;
		document.l2tp.tunnel_auth.disabled = true;
		document.l2tp.tunnel_secret.disabled = true;
		document.l2tp.auth.disabled = true;
		document.l2tp.username.disabled = true;
		document.l2tp.password.disabled = true;
		document.l2tp.pppconntype.disabled = true;
		document.l2tp.idletime.disabled = true;
		document.l2tp.mtu.disabled = true;
		document.l2tp.defaultgw.disabled = true;
		document.l2tp.addClient.disabled = true;
		document.l2tp.enctype.disabled = true;
		document.l2tp.delSelClient.disabled = true;
		/* LNS */
		document.l2tp.s_auth.disabled = true;
		document.l2tp.s_enctype.disabled = true;
		document.l2tp.peeraddr.disabled = true;
		document.l2tp.localaddr.disabled = true;
		document.l2tp.addServer.disabled = true;
		document.l2tp.s_accout_name.disabled = true;
		document.l2tp.tunnelen.disabled = true;
		document.l2tp.s_tunnelAuth.disabled = true;
		document.l2tp.s_authKey.disabled = true;
		document.l2tp.s_username.disabled = true;
		document.l2tp.s_password.disabled = true;
		document.l2tp.addAccount.disabled = true;
		document.l2tp.delSelAccount.disabled = true;
	}
	else {
		/* LAC */
		document.l2tp.c_name.disabled = false;
		document.l2tp.IpProtocolType.disabled = false;
		document.l2tp.server.disabled = false;
		document.l2tp.tunnel_auth.disabled = false;
		document.l2tp.tunnel_secret.disabled = false;
		document.l2tp.auth.disabled = false;
		document.l2tp.username.disabled = false;
		document.l2tp.password.disabled = false;
		document.l2tp.pppconntype.disabled = false;
		document.l2tp.idletime.disabled = false;
		document.l2tp.mtu.disabled = false;
		document.l2tp.defaultgw.disabled = false;
		document.l2tp.addClient.disabled = false;
		document.l2tp.enctype.disabled = false;
		document.l2tp.delSelClient.disabled = false;
		/* LNS */
		document.l2tp.s_auth.disabled = false;
		document.l2tp.s_enctype.disabled = false;
		document.l2tp.peeraddr.disabled = false;
		document.l2tp.localaddr.disabled = false;
		document.l2tp.addServer.disabled = false;
		document.l2tp.s_accout_name.disabled = false;
		document.l2tp.tunnelen.disabled = false;
		document.l2tp.s_tunnelAuth.disabled = false;
		document.l2tp.s_authKey.disabled = false;
		document.l2tp.s_username.disabled = false;
		document.l2tp.s_password.disabled = false;
		document.l2tp.addAccount.disabled = false;
		document.l2tp.delSelAccount.disabled = false;
	}
	onClickTunnelAuth();
	onClickTunnelAuth_lns();
	pppAuthChange();
	connTypeChange()
}

function onClickL2TPEnable()
{
	l2tpSelection();
	
	if (document.l2tp.l2tpen[0].checked)
		document.l2tp.lst.value = "disable";
	else
		document.l2tp.lst.value = "enable";
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	document.l2tp.submit();
}

function addL2TPItf(obj)
{
	if(document.l2tp.l2tpen[0].checked)
		return false;
	
	if (document.l2tp.c_name.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_CLIENT_RULE_NAME); %>');
		document.l2tp.c_name.focus();
		return false;
	}

	if (document.l2tp.server.value=="") {		
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_ADDRESS); %>');
		document.l2tp.server.focus();
		return false;
	}

	if (document.l2tp.IpProtocolType.value == 2)
	{
		if (! isGlobalIpv6Address( document.l2tp.server.value) )
		{			
			alert('<% multilang(LANG_PLEASE_INPUT_IPV6_ADDRESS); %>');
			document.l2tp.server.focus();
			return false;
		}
	}
	else if (document.l2tp.IpProtocolType.value == 1)
	{
		if (!checkTextStr(document.l2tp.server.value))
		{			
			alert('<% multilang(LANG_PLEASE_INPUT_IPV4_ADDRESS); %>');
			document.l2tp.server.focus();
			return false;
		}
	}

	if (document.l2tp.tunnel_auth.checked)
	{
		if (document.l2tp.tunnel_secret.value=="")
		{			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_AUTHENTICATION_SECRET); %>');
			document.l2tp.tunnel_secret.focus();
			return false;
		}
		if(!checkTextStr(document.l2tp.tunnel_secret.value))
		{			
			alert('<% multilang(LANG_INVALID_VALUE_IN_TUNNEL_AUTHENTICATION_SECRET); %>');
			document.l2tp.tunnel_secret.focus();
			return false;		
		}
	}
	
	if (document.l2tp.auth.selectedIndex!=3)
	{
		if (document.l2tp.username.value=="")
		{			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_CLIENT_USERNAME); %>');
			document.l2tp.username.focus();
			return false;
		}
		if(!checkTextStr(document.l2tp.username.value))
		{			
			alert('<% multilang(LANG_INVALID_VALUE_IN_USERNAME); %>');
			document.l2tp.username.focus();
			return false;		
		}
		if (document.l2tp.password.value=="") {			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_CLIENT_PASSWORD); %>');
			document.l2tp.password.focus();
			return false;
		}
		if (includeSpace(document.l2tp.password.value)) {
			alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD_PLEASE_TRY_IT_AGAIN); %>');
			document.l2tp.password.focus();
			return false;
		}
		if(!checkString(document.l2tp.password.value))
		{
			alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
			document.l2tp.password.focus();
			return false;
		}
		document.l2tp.encodePassword.value=encode64(document.l2tp.password.value);
		document.l2tp.password.disabled=true;
	}

	if (document.l2tp.pppconntype.selectedIndex==1)
	{
		if (document.l2tp.idletime.value=="")
		{			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_IDLE_TIME); %>');
			document.l2tp.idletime.focus();
			return false;
		}
	}

	if (document.l2tp.mtu.value=="")
	{		
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_MTU); %>');
		document.l2tp.mtu.focus();
		return false;
	}


	if(false == checkLacOverIPSec()){
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	
	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		return true;
	}
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function roleSelect()
{
	var myval = document.getElementById("sel_rol").value;
	if (myval == 0) {
		document.getElementById("l2tp_lac").style.display = "";
		document.getElementById("l2tp_lns").style.display = "none";
	}
	else {
		document.getElementById("l2tp_lac").style.display = "none";
		document.getElementById("l2tp_lns").style.display = "";
	}
}

function show_password(id)
{
	var x= document.l2tp.password;
	if(id==1){
		x= document.l2tp.password;
	}
	else if(id==2){
		x= document.l2tp.s_password;
	}
	else if(id==3){
		x= document.l2tp.IKEPreshareKeyLAC;
	}
	else if(id==4){
		x= document.l2tp.IKEPreshareKeyLNS;
	}
	if (x.type == "password") {
		x.type = "text";
	} else {
		x.type = "password";
	}
}
</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">L2TP VPN <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content"> <% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIGURE_THE_PARAMETERS_FOR_L2TP_MODE_VPN); %></p>
</div>

<form action=/boaform/formL2TP method=POST name="l2tp">
<div id="l2tp_type" class="data_common data_common_notitle" style=display:none>
<table>
<tr>
<th>L2TP Type:</th>
<td>
<select id="sel_rol" name="ltype" onChange="roleSelect()">
	<option value=0>LAC</option>
	<option value=1>LNS</option>
</select>
</td>
</tr>
</table>
</div>
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th>L2TP VPN:</th>
			<td>
				<input type="radio" value="0" name="l2tpen" <% checkWrite("l2tpenable0"); %> onClick="onClickL2TPEnable()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="l2tpen" <% checkWrite("l2tpenable1"); %> onClick="onClickL2TPEnable()"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
	</table>
</div>
<input type="hidden" id="lst" name="lst" value="">

<div id="l2tp_lac">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th><% multilang(LANG_NAME); %>:</th>
			<td><input type="text" name="c_name" size="16" maxlength="256"></td>
		</tr>
		<tr id="l2tp_ipv6" style=display:none>
			<th>IP <% multilang(LANG_PROTOCOL); %>:</th>
			<td><select id="IpProtocolType" style="WIDTH: 130px" name="IpProtocolType">
					<option value="1" > IPv4</option>
					<option value="2" > IPv6</option>
				</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_SERVER); %>:</th>
			<td><input type="text" name="server" size="32" maxlength="256"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_TUNNEL_AUTHENTICATION); %>:</th>
			<td><input type=checkbox name="tunnel_auth" value=1 onClick=onClickTunnelAuth()></td>
		</tr>
		<tr>
			<th><% multilang(LANG_TUNNEL_AUTHENTICATION_SECRET); %>:</th>
			<td><input type="text" name="tunnel_secret" size="15" maxlength="35"></td>
		</tr>
		<tr>
			<th>PPP <% multilang(LANG_AUTHENTICATION); %>:</th>
			<td><select name="auth" onChange="pppAuthChange()">
					<option value="0"><% multilang(LANG_AUTO); %></option>
					<option value="1">PAP</option>
					<option value="2">CHAP</option>
					<option value="3">CHAPMSV2</option>
				</select>
			</td>
		</tr>
		<tr>
			<th>PPP <% multilang(LANG_ENCRYPTION); %>:</th>
			<td><select name="enctype" >
					<option value="0"><% multilang(LANG_NONE); %></option>
					<option value="1">MPPE</option>
					<option value="2">MPPC</option>
					<option value="3">MPPE&MPPC</option>
				</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:</th>
			<td><input type="text" name="username" size="15" maxlength="35"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_PASSWORD); %>:</th>
			<td><input type="password" name="password" size="15" maxlength="35">
			    <input type="checkbox" onClick="show_password(1)">Show Password</td>
		</tr>
		<tr>
			<th>PPP <% multilang(LANG_CONNECTION_TYPE); %>:</th>
			<td><select name="pppconntype" onChange="connTypeChange()">
					<option value="0"><% multilang(LANG_PERSISTENT); %></option>
					<option value="1"><% multilang(LANG_DIAL_ON_DEMAND); %></option>
					<option value="2"><% multilang(LANG_MANUAL); %></option>
					<option value="3"><% multilang(LANG_NONE); %></option>
				</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_IDLE_TIME_MIN); %>:</th>
			<td><input type="text" name="idletime" size="32" maxlength="256"></td>
		</tr>
		<tr>
			<th>MTU:</th>
			<td><input type="text" name="mtu" size="32" maxlength="256" value="1458"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_DEFAULT_GATEWAY); %>:</th>
			<td><input type="checkbox" name="defaultgw"></td>
		</tr>
		<% l2tpWithIPSecLAC(); %>
	</table>
</div>

<div class="btn_ctl">
      <input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="addClient" onClick="return addL2TPItf(this)">&nbsp;&nbsp;
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>L2TP <% multilang(LANG_TABLE); %></p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th align=center width="3%"><% multilang(LANG_SELECT); %></th>
				<th align=center width="5%"><% multilang(LANG_NAME); %></th>
				<th align=center width="5%"><% multilang(LANG_SERVER); %></th>
				<th align=center width="5%"><% multilang(LANG_TUNNEL_AUTHENTICATION); %></th>
				<th align=center width="5%">PPP <% multilang(LANG_AUTHENTICATION); %></th>
				<th align=center width="5%">MTU</th>
				<th align=center width="5%"><% multilang(LANG_DEFAULT_GATEWAY); %></th>
				<% l2tpWithIPSecLACTable(); %>
				<th align=center width="8%"><% multilang(LANG_ACTION); %></th>
			</tr>
			<% l2tpList(); %>
		</table>
	</div>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delSelClient" onClick="return deleteClick(this)">&nbsp;&nbsp;
	<input type="hidden" name="encodePassword" value="">
	<input type="hidden" name="encodepskValueLAC" value="">
</div>
</div>

<div id="l2tp_lns">
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>L2TP Server</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th><% multilang(LANG_NAME); %>:</th>
				<td><input type="text" name="s_name" size="16" maxlength="256"></td>
			</tr>
			<tr>
				<th><% multilang(LANG_AUTHENTICATION_METHOD); %>:</th>
				<td>
					<select name="s_auth" onChange="pppAuthChange_lns()">
						<option value="0"><% multilang(LANG_AUTO); %></option>
						<option value="1">PAP</option>
						<option value="2">CHAP</option>
						<option value="3">MS-CHAPV2</option>
					</select>
				</td>
			</tr>
			<tr>
				<th>Encryption Mode:</th>
				<td>
					<select name="s_enctype" >
						<option value="0"><% multilang(LANG_NONE); %></option>
						<option value="1">MPPE</option>
						<option value="2">MPPC</option>
						<option value="3">MPPE&MPPC</option>
					</select>
				</td>
			</tr>
			<tr>
				<th><% multilang(LANG_TUNNEL_AUTHENTICATION); %>:</th>
				<td><input type="checkbox" name="s_tunnelAuth" value="1" onClick="onClickTunnelAuth_lns()"></td>
			</tr>
			<tr>
				<th><% multilang(LANG_TUNNEL_AUTHENTICATION_SECRET); %>:</th>
				<td><input type="text" name="s_authKey" size="16" maxlength="256"></td>
			</tr>
			<tr>
				<th>Peer Address:</th>
				<td>start from:<input type="text" name="peeraddr" size="16" maxlength="80"></td>
			</tr>
			<tr>
				<th>Local Address:</th>
				<td><input type="text" name="localaddr" size="16" maxlength="80"></td>
			</tr>
			<% l2tpWithIPSecLNS(); %>
		</table>
	</div>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="addServer" onClick="return setL2tpServer(this)">
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>Server Account</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<th><% multilang(LANG_NAME); %>:</th>
			<td>
				<select id="s_accout_name" name="s_accout_name">
					<% lns_name_list(); %>
				</select>
			</td>
			<tr>
				<th>Tunnel:</th>
				<td>
					<input type="radio" value="0" name="tunnelen"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
					<input type="radio" value="1" name="tunnelen" checked><% multilang(LANG_ENABLE); %>
				</td>
			</tr>
			<tr>
				<th><% multilang(LANG_USERNAME); %>:</th>
				<td><input type="text" name="s_username" size="16" maxlength="256"></td>
			</tr>
			<tr>
				<th><% multilang(LANG_PASSWORD); %>:</th>
				<td><input type="password" name="s_password" size="16" maxlength="256">
					<input type="checkbox" onClick="show_password(2)">Show Password</td>
			</tr>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_ADD); %>" name="addAccount" onClick="return addL2tpAccount(this)">
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>L2TP Server Table</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th align=center><% multilang(LANG_SELECT); %></th>
				<th align=center><% multilang(LANG_NAME); %></th>
				<th align=center><% multilang(LANG_ENABLE); %></th>
				<th align=center><% multilang(LANG_USERNAME); %></th>
				<th align=center><% multilang(LANG_PASSWORD); %></th>
			</tr>
			<% l2tpServerList(); %>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="saveAccount" onClick="return on_submit(this)">&nbsp;&nbsp;
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delSelAccount" onClick="return deleteClick(this)">&nbsp;&nbsp;
	<input type="hidden" name="encodeSPassword" value="">
	<input type="hidden" name="encodepskValueLNS" value="">
</div>
</div>
<input type="hidden" name=lns_en value=0>
<input type="hidden" name=ipv6_en value=0>
<input type="hidden" value="/l2tpv6.asp" name="submit-url">
<input type="hidden" name="postSecurityFlag" value="">

<script>
	<% initPage("l2tp"); %>
	if (document.l2tp.lns_en.value != 0)
		document.getElementById("l2tp_type").style.display = "";
	if (document.l2tp.ipv6_en.value != 0)
		document.getElementById("l2tp_ipv6").style.display = "";
	roleSelect();
	l2tpSelection();
</script>
</form> 
</body>
</html>
