<%SendWebHeadStr(); %>
<title>PPTP VPN <% multilang(LANG_CONFIGURATION); %></title>

<script type="text/javascript" src="base64_code.js"></script>
<script language="javascript">

function checkTextStr(str)
{
	for (var i=0; i<str.length; i++) 
	{
		if ( str.charAt(i) == '%' || str.charAt(i) =='&' ||str.charAt(i) =='\\' || str.charAt(i) =='?' || str.charAt(i)=='"') 
			return false;			
	}
	return true;
}

function disButton(id)
{
	getObj = document.getElementById(id);
	window.setTimeout("getObj.disabled=true", 100);
	return false;
}

function pptpSelection()
{
	if (document.pptp.pptpen[0].checked) {
		document.pptp.IpProtocolType.disabled = true;
		document.pptp.server.disabled = true;
		document.pptp.username.disabled = true;
		document.pptp.password.disabled = true;
		document.pptp.auth.disabled = true;
		document.pptp.defaultgw.disabled = true;
		document.pptp.addPPtP.disabled = true;
		document.pptp.enctype.disabled = true;
		/* Server */
		document.pptp.s_name.disabled = true;
		document.pptp.s_auth.disabled = true;
		document.pptp.s_enctype.disabled = true;
		document.pptp.peeraddr.disabled = true;
		//document.pptp.localaddr.disabled = true;
		document.pptp.addServer.disabled = true;
		//document.pptp.s_accout_name.disabled = true;
		document.pptp.tunnelen[0].disabled = true;
		document.pptp.tunnelen[1].disabled = true;
		//document.pptp.s_tunnelAuth.disabled = true;
		//document.pptp.s_authKey.disabled = true;
		document.pptp.s_username.disabled = true;
		document.pptp.s_password.disabled = true;
		document.pptp.addAccount.disabled = true;
		document.pptp.delSelAccount.disabled = true;
	}
	else {
		document.pptp.IpProtocolType.disabled = false;
		document.pptp.server.disabled = false;
		document.pptp.username.disabled = false;
		document.pptp.password.disabled = false;
		document.pptp.auth.disabled = false;
		document.pptp.defaultgw.disabled = false;
		document.pptp.addPPtP.disabled = false;
		document.pptp.enctype.disabled = true;
		/* Server */
		document.pptp.s_name.disabled = false;
		document.pptp.s_auth.disabled = false;
		document.pptp.s_enctype.disabled = false;
		document.pptp.peeraddr.disabled = false;
		//document.pptp.localaddr.disabled = false;
		document.pptp.addServer.disabled = false;
		//document.pptp.s_accout_name.disabled = false;
		document.pptp.tunnelen[0].disabled = false;
		document.pptp.tunnelen[1].disabled = false;
		//document.pptp.s_tunnelAuth.disabled = false;
		//document.pptp.s_authKey.disabled = false;
		document.pptp.s_username.disabled = false;
		document.pptp.s_password.disabled = false;
		document.pptp.addAccount.disabled = false;
		document.pptp.delSelAccount.disabled = false;
	}
}


function encryClick()
{
	if (document.pptp.auth.value==3) {
		document.pptp.enctype.disabled = false;
	}else
		document.pptp.enctype.disabled = true;
}

function onClickPPtpEnable()
{
	pptpSelection();
	if (document.pptp.pptpen[0].checked)
		document.pptp.lst.value = "disable";
	else
	document.pptp.lst.value = "enable";
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	document.pptp.submit();
}

function addPPtPItf(obj)
{
	if(document.pptp.pptpen[0].checked)
		return false;
	
	if (document.pptp.server.value=="") {
		alert("<% multilang(LANG_PLEASE_ENTER_PPTP_SERVER_ADDRESS); %>");
		document.pptp.server.focus();
		return false;
	}
	
	if(!checkTextStr(document.pptp.server.value))
	{
		alert("<% multilang(LANG_INVALID_VALUE_IN_SERVER_ADDRESS); %>");
		document.pptp.server.focus();
		return false;		
	}
	
	if (document.pptp.username.value=="")
	{
		alert("<% multilang(LANG_PLEASE_ENTER_PPTP_CLIENT_USERNAME); %>");
		document.pptp.username.focus();
		return false;
	}
	if(!checkTextStr(document.pptp.username.value))
	{
		alert("<% multilang(LANG_INVALID_VALUE_IN_USERNAME); %>");
		document.pptp.username.focus();
		return false;		
	}
	if (document.pptp.password.value=="") {
		alert("<% multilang(LANG_PLEASE_ENTER_PPTP_CLIENT_PASSWORD); %>");
		document.pptp.password.focus();
		return false;
	}

	if (includeSpace(document.pptp.password.value)) {
		alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD_PLEASE_TRY_IT_AGAIN); %>');
		document.pptp.password.focus();
		return false;
	}
	if(!checkString(document.pptp.password.value))
	{
		alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
		document.pptp.password.focus();
		return false;
	}
	document.pptp.encodePassword.value=encode64(document.pptp.password.value);
	document.pptp.password.disabled=true;

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
		document.getElementById("pptp_client").style.display = "";
		document.getElementById("pptp_server").style.display = "none";
	}
	else {
		document.getElementById("pptp_client").style.display = "none";
		document.getElementById("pptp_server").style.display = "";
	}
}

function pppAuthChange_server()
{
	if (document.pptp.s_auth.value==3) {
		document.pptp.s_enctype.disabled = false;
	}else
		document.pptp.s_enctype.disabled = true;
}

function setPPTPServer(obj)
{
	if(document.pptp.pptpen[0].checked)
		return false;

	if (document.pptp.s_name.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_PPTP_SERVER_RULE_NAME); %>');
		document.pptp.s_name.focus();
		return false;
	}

	if (document.pptp.peeraddr.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_PEER_START_ADDRESS); %>');
		document.pptp.peeraddr.focus();
		return false;
	}
	if (!checkHostIP(document.pptp.peeraddr, 0))
		return false;

	/*
	if (document.pptp.localaddr.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_LOCAL_ADDRESS); %>');
		document.pptp.localaddr.focus();
		return false;
	}
	if (!checkHostIP(document.pptp.localaddr, 0))
		return false;
	*/

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function addPPTPAccount(obj)
{
	if(document.pptp.pptpen[0].checked)
		return false;

	if (document.pptp.s_username.value=="")
	{
		alert('<% multilang(LANG_PLEASE_ENTER_PPTP_SERVER_USERNAME); %>');
		document.pptp.s_username.focus();
		return false;
	}
	if(!checkTextStr(document.pptp.s_username))
	{
		return false;
	}

	if (document.pptp.s_password.value=="") {
		alert('<% multilang(LANG_PLEASE_ENTER_PPTP_SERVER_PASSWORD); %>');
		document.pptp.s_password.focus();
		return false;
	}

	if (includeSpace(document.pptp.s_password.value)) {
		alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD_PLEASE_TRY_IT_AGAIN); %>');
		document.pptp.s_password.focus();
		return false;
	}
	if(!checkString(document.pptp.s_password.value))
	{
		alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
		document.pptp.s_password.focus();
		return false;
	}
	document.pptp.encodeSPassword.value=encode64(document.pptp.s_password.value);
	document.pptp.s_password.disabled=true;

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function show_password(id)
{
	var x= document.pptp.password;
	if(id==1){
		x= document.pptp.password;
	}
	else if(id==2){
		x= document.pptp.s_password;
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
	<p class="intro_title">PPTP VPN <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content"> <% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIGURE_THE_PARAMETERS_FOR_PPTP_MODE_VPN); %></p>
</div>

<form action=/boaform/formPPtP method=POST name="pptp">
<div id="pptp_type" class="data_common data_common_notitle" style=display:none>
<table>
<tr>
<th>PPTP Type:</th>
<td>
<select id="sel_rol" name="ltype" onChange="roleSelect()">
	<option value=0>Client</option>
	<option value=1>Server</option>
</select>
</td>
</tr>
</table>
</div>

<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th>PPTP VPN:</th>
			<td> 
				<input type="radio" value="0" name="pptpen" <% checkWrite("pptpenable0"); %> onClick="onClickPPtpEnable()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="pptpen" <% checkWrite("pptpenable1"); %> onClick="onClickPPtpEnable()"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
	</table>
</div>
<input type="hidden" id="lst" name="lst" value="">

<div id="pptp_client">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th>IP <% multilang(LANG_PROTOCOL); %>:</th>
			<td ><select id="IpProtocolType" style="WIDTH: 130px" onChange="protocolChange()" name="IpProtocolType">
			<option value="1" > IPv4</option>
			<option value="2" > IPv6</option>
			</select></td>
		</tr>
		<tr>
			<th><% multilang(LANG_SERVER); %>:</th>
			<td><input type="text" name="server" size="32" maxlength="256"></td>
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
			<th><% multilang(LANG_AUTHENTICATION); %>:</th>
			<td><select name="auth" onClick="encryClick()">
				<option value="0"><% multilang(LANG_AUTO); %></option>
				<option value="1">PAP</option>
				<option value="2">CHAP</option>
				<option value="3">CHAPMSV2</option>
			</select>
		</td>
		</tr>
		<tr>
			<th><% multilang(LANG_ENCRYPTION); %>:</th>
			<td><select name="enctype" >
					<option value="0"><% multilang(LANG_NONE); %></option>
					<option value="1">MPPE</option>
					<option value="2">MPPC</option>
					<option value="3">MPPE&MPPC</option>
				</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_DEFAULT_GATEWAY); %>:</th>
			<td><input type="checkbox" name="defaultgw"></td>
		</tr>
	</table>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="addPPtP" onClick="return addPPtPItf(this)">
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>PPTP <% multilang(LANG_TABLE); %></p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th align=center><% multilang(LANG_SELECT); %></th>
				<th align=center><% multilang(LANG_INTERFACE); %></th>
				<th align=center><% multilang(LANG_SERVER); %></th>
				<th align=center><% multilang(LANG_ACTION); %></th>
			</tr>
			<% pptpList(); %>
		</table>
	</div>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delSel" onClick="return deleteClick(this)">&nbsp;&nbsp;
	<input type="hidden" name="encodePassword" value="">
	<input type="hidden" value="/pptpv6.asp" name="submit-url">
</div>
</div>

<div id="pptp_server">
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>PPTP Server</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th><% multilang(LANG_NAME); %>:</th>
				<td><input type="text" name="s_name" size="16" maxlength="256"></td>
			</tr>
			<tr>
				<th>Auth. Type:</th>
				<td>
					<select name="s_auth" onChange="pppAuthChange_server()">
						<option value="0">PAP and CHAP</option>
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
				<th>Peer Address:</th>
				<td>start from:<input type="text" name="peeraddr" size="16" maxlength="80"></td>
			</tr>
			<tr>
				<th>Local Address:</th>
				<td><input type="text" name="localaddr" size="16" maxlength="80"></td>
			</tr>
		</table>
	</div>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="Apply" name="addServer" onClick="return setPPTPServer(this)">
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>Server Account</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<tr>
				<th>Tunnel:</th>
				<td>
					<input type="radio" value="0" name="tunnelen"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
					<input type="radio" value="1" name="tunnelen" checked><% multilang(LANG_ENABLE); %>
				</td>
			</tr>
			<tr>
				<th>Username:</th>
				<td><input type="text" name="s_username" size="16" maxlength="256"></td>
			</tr>
			<tr>
				<th>Password:</th>
				<td><input type="password" name="s_password" size="16" maxlength="256">
				<input type="checkbox" onClick="show_password(2)">Show Password</td>
			</tr>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="Add" name="addAccount" onClick="return addPPTPAccount(this)">
</div>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>PPTP Server Table</p>
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
			<% pptpServerList(); %>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delSelAccount" onClick="return deleteClick(this)">&nbsp;&nbsp;
	<input type="hidden" name="encodeSPassword" value="">
	<input type="hidden" value="/pptpv6.asp" name="submit-url">
</div>
</div>

<input type="hidden" name=server_en value=0>
<input type="hidden" name="postSecurityFlag" value="">

<script>
	document.pptp.localaddr.disabled = true;
	<% initPage("pptp-vpn"); %>
	if (document.pptp.server_en.value != 0)
		document.getElementById("pptp_type").style.display = "";
	roleSelect();
	pptpSelection();
</script>
</form>
</blockquote>
</body>
</html>
