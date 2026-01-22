<% SendWebHeadStr();%>
<title>TR-069 <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
var supportTR111 = <% SupportTR111(); %>;

function resetClick()
{
   document.tr069.reset;
}

function periodicSel() {
	if ( document.tr069.enable[0].checked ) {
		disableTextField(document.tr069.interval);
	} else {
		enableTextField(document.tr069.interval);
	}
}

function StunSel() {
	if ( document.tr069.stun_enable[0].checked ) {
		disableTextField(document.tr069.stunsvraddr);
		disableTextField(document.tr069.stunsvrport);
		disableTextField(document.tr069.stunsvruname);
		disableTextField(document.tr069.stunsvrupasswd);
	} else {
		enableTextField(document.tr069.stunsvraddr);
		enableTextField(document.tr069.stunsvrport);
		enableTextField(document.tr069.stunsvruname);
		enableTextField(document.tr069.stunsvrupasswd);
	}
}

<% TR069ConPageShow("ShowAuthSelFun"); %>

function saveChanges(obj)
{
  if (document.tr069.url.value=="") {
	alert("<% multilang(LANG_ACS_URL_CANNOT_BE_EMPTY); %>");
	document.tr069.url.value = document.tr069.url.defaultValue;
	document.tr069.url.focus();
	return false;
  }

	if (sji_checkhttpurl(document.tr069.url.value) == false){
		alert("<% multilang(LANG_ACS_URL_INVALID); %>");
		document.tr069.url.value = document.tr069.url.defaultValue;
		document.tr069.url.focus();
		return false;
	}

	if (checkString(document.tr069.username.value) == 0) {
		alert("<% multilang(LANG_INVALID_USER_NAME); %>");
		document.tr069.username.focus();
		return false;
	}
	if (checkString(document.tr069.password.value) == 0) {
		alert("<% multilang(LANG_INVALID_PASSWORD); %>");
		document.tr069.password.focus();
		return false;
	}
  if (document.tr069.enable[1].checked) {
	if ( document.tr069.interval.value=="") {
		alert("<% multilang(LANG_PLEASE_INPUT_PERIODIC_INTERVAL_TIME_); %>");
		document.tr069.interval.focus();
		return false;
	}
	if ( validateKey( document.tr069.interval.value ) == 0 ) {
		alert("<% multilang(LANG_INTERVAL_SHOULD_BE_THE_DECIMAL_NUMBER_0_9); %>");
		document.tr069.interval.focus();
		return false;
	}
  }

	if (checkString(document.tr069.conreqname.value) == 0) {
		alert("<% multilang(LANG_INVALID_USER_NAME); %>");
		document.tr069.conreqname.focus();
		return false;
	}
	if (checkString(document.tr069.conreqpw.value) == 0) {
		alert("<% multilang(LANG_INVALID_PASSWORD); %>");
		document.tr069.conreqpw.focus();
		return false;
	}
	if (checkString(document.tr069.conreqpath.value) == 0) {
		alert("<% multilang(LANG_INVALID_PATH); %>");
		document.tr069.conreqpath.focus();
		return false;
	}
  if (document.tr069.conreqport.value=="") {
	alert("<% multilang(LANG_PLEASE_INPUT_THE_PORT_NUMBER_FOR_CONNECTION_REQUEST_); %>");
	document.tr069.conreqport.value = document.tr069.conreqport.defaultValue;
	document.tr069.conreqport.focus();
	return false;
  }
  if ( validateKey( document.tr069.conreqport.value ) == 0 ) {
	alert("<% multilang(LANG_INVALID_PORT_NUMBER_OF_CONNECTION_REQUEST_IT_SHOULD_BE_1_65535); %>");
	document.tr069.conreqport.value = document.tr069.conreqport.defaultValue;
	document.tr069.conreqport.focus();
	return false;
  }
  if ( !checkDigitRange(document.tr069.conreqport.value,1,1,65535) ) {
  	   	alert("<% multilang(LANG_INVALID_PORT_NUMBER_OF_CONNECTION_REQUEST_IT_SHOULD_BE_1_65535); %>");
	document.tr069.conreqport.value = document.tr069.conreqport.defaultValue;
	document.tr069.conreqport.focus();
	return false;
  }
  if(supportTR111){
   if (document.tr069.stun_enable[1].checked) {
	if ( document.tr069.stunsvraddr.value=="") {
		alert("Please input stun server address.");
		document.tr069.stunsvraddr.focus();
		return false;
	}
	if ( document.tr069.stunsvrport.value=="") {
		alert("Please input stun server port.");
		document.tr069.stunsvrport.focus();
		return false;
	}
	if ( document.tr069.stunsvruname.value=="") {
		alert("Please input stun server user name.");
		document.tr069.stunsvruname.focus();
		return false;
	}
	if ( document.tr069.stunsvrupasswd.value=="") {
		alert("Please input stun server user password.");
		document.tr069.stunsvrupasswd.focus();
		return false;
	}
  }
  if (checkString(document.tr069.stunsvraddr.value) == 0) {
	alert('Invalid STUN Server address.');
	document.tr069.stunsvraddr.focus();
	return false;
  }
  if (checkString(document.tr069.stunsvrport.value) == 0) {
	alert('Invalid STUN Server port.');
	document.tr069.stunsvrport.focus();
	return false;
  }
  if (checkString(document.tr069.stunsvruname.value) == 0) {
	alert('Invalid STUN Server user name.');
	document.tr069.stunsvruname.focus();
	return false;
  }
  if (checkString(document.tr069.stunsvrupasswd.value) == 0) {
	alert('Invalid STUN Server user password.');
	document.tr069.stunsvrupasswd.focus();
	return false;
  }
  } <!-- End if(supportTR111) -->

  obj.isclick = 1;
  postTableEncrypt(document.tr069.postSecurityFlag, document.tr069);
  return true;
}

function addClick(obj)
{
	if (!checkNetIP(document.tr069_acl.aclIP, 1))
		return false;
	if (!checkNetmask(document.tr069_acl.aclMask, 1))
		return false;

	obj.isclick = 1;
	postTableEncrypt(document.tr069_acl.postSecurityFlag, document.tr069_acl);
	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.tr069_acl.postSecurityFlag, document.tr069_acl);
		return true;
	}
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.tr069_acl.postSecurityFlag, document.tr069_acl);
	return true;
}
</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">TR-069 <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content">  <% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIGURE_THE_TR_069_CPE_HERE_YOU_MAY_CHANGE_THE_SETTING_FOR_THE_ACS_S_PARAMETERS); %></p>
</div>
<form action=/boaform/formTR069Config method=POST name="tr069">


<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="30%" style="font-weight: bold; font-size: 14"><% multilang(LANG_TR069_DAEMON); %>:</th>
			<td width="70%"><font size=2>
			<input type="radio" name=autoexec value=1 <% checkWrite("tr069-autoexec-1"); %> ><% multilang(LANG_ENABLED); %>&nbsp;&nbsp;
			<input type="radio" name=autoexec value=0 <% checkWrite("tr069-autoexec-0"); %> ><% multilang(LANG_DISABLED); %></font>
			</td>
		</tr>
		<tr>
			<th width="30%" style="font-weight: bold; font-size: 14"><% multilang(LANG_ENABLE); %>CWMP<% multilang(LANG_PARAMETER); %>:</th>
			<td width="70%"><font size=2>
			<input type="radio" name=enable_cwmp value=1 <% checkWrite("tr069-enable-cwmp-1"); %> ><% multilang(LANG_ENABLED); %>&nbsp;&nbsp;
			<input type="radio" name=enable_cwmp value=0 <% checkWrite("tr069-enable-cwmp-0"); %> ><% multilang(LANG_DISABLED); %></font>
			</td>
		</tr>
		<% TR069ConPageShow("ShowDataModels"); %>
	</table>
</div>
<div ID=WANshow class="data_common data_common_notitle" style="display:none">


	<table border=0 width="500" cellspacing=4 cellpadding=0>
	<tr>
		<th width="30%"><% multilang(LANG_WAN_INTERFACE); %>:</th>
		<td width="70%">
			<select name="tr069_itf">
				<option value=65535>&nbsp;</option>
				<% if_wan_list("rt"); %>
			</select>
		</td>
	</tr>
</table>
</div>
<div class="column clearfix">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_ACS); %></p>
		<div class="column_title_right"></div>
	</div>
</div>
<div class="data_common">
<table>

	<tr>
			<th width="30%"><% multilang(LANG_URL); %>:</th>
		<td width="70%"><input type="text" name="url" size="32" maxlength="256" value=<% getInfo("acs-url"); %>></td>
	</tr>
	<tr>
		<th width="30%"><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:</th>
		<td width="70%"><input type="text" name="username" size="32" maxlength="256" value=<% getInfo("acs-username"); %>></td>
	</tr>
	<tr>
		<th width="30%"><% multilang(LANG_PASSWORD); %>:</th>
		<td width="70%"><input type="text" name="password" size="32" maxlength="256" value=<% getInfo("acs-password"); %>></td>
	</tr>
	<tr>
		<th width="30%"><% multilang(LANG_PERIODIC_INFORM); %>:</th>
		<td width="70%">
		<input type="radio" name=enable value=0 <% checkWrite("tr069-inform-0"); %> onClick="return periodicSel()"><% multilang(LANG_DISABLED); %>&nbsp;&nbsp;
		<input type="radio" name=enable value=1 <% checkWrite("tr069-inform-1"); %> onClick="return periodicSel()"><% multilang(LANG_ENABLED); %>
		</td>
	</tr>
	<tr>
		<th width="30%"><% multilang(LANG_PERIODIC_INFORM_INTERVAL); %>:</th>
		<td width="70%"><input type="text" name="interval" size="32" maxlength="10" value=<% getInfo("inform-interval"); %>  <% checkWrite("tr069-interval"); %> ></td>
	</tr>
</table>
</div>
<div class="column clearfix">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_CONNECTION_REQUEST); %></p>
		<div class="column_title_right"></div>
	</div>
</div>
<div class="data_common">
<table>

	<% TR069ConPageShow("ShowAuthSelect"); %>
  <tr>
      <th width="30%"><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:</th>
      <td width="70%"><input type="text" name="conreqname" size="32" maxlength="256" value=<% getInfo("conreq-name"); %> <% TR069ConPageShow("DisConReqName"); %> ></td>
  </tr>
  <tr>
      <th width="30%"><% multilang(LANG_PASSWORD); %>:</th>
      <td width="70%"><input type="text" name="conreqpw" size="32" maxlength="256" value=<% getInfo("conreq-pw"); %> <% TR069ConPageShow("DisConReqPwd"); %> ></td>
  </tr>
  <tr>
      <th width="30%"><% multilang(LANG_PATH); %>:</th>
      <td width="70%"><input type="text" name="conreqpath" size="32" maxlength="31" value=<% getInfo("conreq-path"); %>></td>
  </tr>
  <tr>
      <th width="30%"><% multilang(LANG_PORT); %>:</th>
      <td width="70%"><input type="text" name="conreqport" size="32" maxlength="5" value=<% getInfo("conreq-port"); %>></td>
</tr>
</table>
<div id="TR111_STUN">
<div class="column clearfix">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_TR069_STUN_SETTING); %></p>
		<div class="column_title_right"></div>
	</div>
</div>
<div class="data_common">
<table>
  <tr>
	<td width="30%"><font size=2><b><% multilang(LANG_TR069_STUN); %>:</b></td>
	<td width="70%"><font size=2>
	<input type="radio" name=stun_enable value=0 <% checkWrite("stun-0"); %> onClick="return StunSel()"><% multilang(LANG_DISABLED); %>&nbsp;&nbsp;
	<input type="radio" name=stun_enable value=1 <% checkWrite("stun-1"); %> onClick="return StunSel()"><% multilang(LANG_ENABLED); %></font></td>
  </tr>
  <tr>
	<td width="30%"><font size=2><b><% multilang(LANG_TR069_STUN); %><% multilang(LANG_TR069_STUN_SERVER); %><% multilang(LANG_TR069_STUN_SERVER_ADDRESS); %>:</b></td>
	<td width="70%"><input type="text" name="stunsvraddr" size="32" maxlength="256" <% checkWrite("tr069-stun"); %> value=<% getInfo("stunsvr-addr"); %>></td>
  </tr>
  <tr>
        <td width="30%"><font size=2><b><% multilang(LANG_TR069_STUN); %><% multilang(LANG_TR069_STUN_SERVER); %><% multilang(LANG_TR069_STUN_SERVER_PORT); %>:</b></td>
        <td width="70%"><input type="text" name="stunsvrport" size="32" maxlength="5" <% checkWrite("tr069-stun"); %> value=<% getInfo("stunsvr-port"); %>></td>
  </tr>
  <tr>
	<td width="30%"><font size=2><b><% multilang(LANG_TR069_STUN); %><% multilang(LANG_TR069_STUN_SERVER); %><% multilang(LANG_TR069_STUN_SERVER_USERNAME); %>:</b></td>
	<td width="70%"><input type="text" name="stunsvruname" size="32" maxlength="256" <% checkWrite("tr069-stun"); %> value=<% getInfo("stunsvr-uname"); %>></td>
  </tr>
  <tr>
        <td width="30%"><font size=2><b><% multilang(LANG_TR069_STUN); %><% multilang(LANG_TR069_STUN_SERVER); %><% multilang(LANG_TR069_STUN_SERVER_USERPASSWD); %>:</b></td>
        <td width="70%"><input type="text" name="stunsvrupasswd" size="32" maxlength="256" <% checkWrite("tr069-stun"); %> value=<% getInfo("stunsvr-upasswd"); %>></td>
  </tr>
  </tr>
</table>
</div>
</div> <!-- End TR111_STUN div -->
<!--
<table>
  <tr><hr size=1 noshade align=top></tr>
  <tr>
      <td class="table_title">Debug:</td>
  </tr>
  <% TR069ConPageShow("ShowACSCertCPE"); %>
  <tr>
      <td class="table_item">Show Message:</td>
      <td>
      <input type="radio" name=dbgmsg value=0 <% checkWrite("tr069-dbgmsg-0"); %> >Disabled&nbsp;&nbsp;
      <input type="radio" name=dbgmsg value=1 <% checkWrite("tr069-dbgmsg-1"); %> >Enabled
      </td>
  </tr>
  <tr>
      <td class="table_item">CPE Sends GetRPC:</td>
      <td>
      <input type="radio" name=sendgetrpc value=0 <% checkWrite("tr069-sendgetrpc-0"); %> >Disabled&nbsp;&nbsp;
      <input type="radio" name=sendgetrpc value=1 <% checkWrite("tr069-sendgetrpc-1"); %> >Enabled
      </td>
  </tr>
  <tr>
      <td class="table_item">Skip MReboot:</td>
      <td>
      <input type="radio" name=skipmreboot value=0 <% checkWrite("tr069-skipmreboot-0"); %> >Disabled&nbsp;&nbsp;
      <input type="radio" name=skipmreboot value=1 <% checkWrite("tr069-skipmreboot-1"); %> >Enabled
      </td>
  </tr>
  <tr>
      <td class="table_item>"Delay:</td>
      <td>
      <input type="radio" name=delay value=0 <% checkWrite("tr069-delay-0"); %> >Disabled&nbsp;&nbsp;
      <input type="radio" name=delay value=1 <% checkWrite("tr069-delay-1"); %> >Enabled
      </td>
  </tr>
  <tr>
      <td class="table_item">Auto-Execution:</td>
      <td>
      <input type="radio" name=autoexec value=0 <% checkWrite("tr069-autoexec-0"); %> >Disabled&nbsp;&nbsp;
      <input type="radio" name=autoexec value=1 <% checkWrite("tr069-autoexec-1"); %> >Enabled
      </td>
  </tr>
  <% TR069ConPageShow("ShowCTInformExt"); %>
</table>
-->
<div class="btn_ctl">
	<input class="link_bg" type="submit" value=<% multilang(LANG_APPLY_CHANGES); %> name="save" onClick="return saveChanges(this)">&nbsp;&nbsp;
	<input class="link_bg" type="reset" value=<% multilang(LANG_UNDO); %> name="reset" onClick="resetClick()">
	<input type="hidden" value="/tr069config.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<% TR069ConPageShow("ShowMNGCertTable"); %>
<form action=/boaform/formTR069Config method=POST name="tr069_acl">

<div class="data_common data_common_notitle">
<table>
	<tr>
		<th width="30%"><% multilang(LANG_ENABLE); %> CWMP WAN ACL:</th>
		<td width="70%">
		<input type="radio" name=enable_acl value=1 <% checkWrite("tr069-enable-acl-1"); %> ><% multilang(LANG_ENABLED); %>&nbsp;&nbsp;
		<input type="radio" name=enable_acl value=0 <% checkWrite("tr069-enable-acl-0"); %> ><% multilang(LANG_DISABLED); %>&nbsp;&nbsp;&nbsp;&nbsp;
		
		<input class="inner_btn" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="applyACL" onClick="return on_submit(this)">&nbsp;&nbsp; </td>
	</tr>
	<tr>
		<th width="30%"><% multilang(LANG_IP_ADDRESS); %>:</th>
		<td width="70%"><input type="text" name="aclIP" size="15" maxlength="15"></td>
	</tr>  
  
	<tr>
		<th width="30%"><% multilang(LANG_SUBNET_MASK); %>:</th>
		<td width="70%"><input type="text" name="aclMask" size="15" maxlength="15"></td>      
	</tr>
</table>
</div>
<div class="btn_ctl">
	<td><input class="link_bg" type="submit" value="<% multilang(LANG_ADD); %>" name="addIP" onClick="return addClick(this)">&nbsp;&nbsp;</td>
</div>
<div class="column clearfix">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p>CWMP WAN ACL <% multilang(LANG_TABLE); %></p>
		<div class="column_title_right"></div>
	</div>
</div>
<div class="data_common data_vertical">
<table>
  <% showCWMPACLTable(); %>
</table>
</div>
<div class="btn_ctl">
      <input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="delIP" onClick="return deleteClick(this)">&nbsp;&nbsp;      
      <input type="hidden" value="/tr069config.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<script>
	ifIdx = <% getInfo("tr069_wan_intf"); %>;
	document.tr069.tr069_itf.selectedIndex = -1;

	for( i = 0; i < document.tr069.tr069_itf.options.length; i++ )
	{
		if( ifIdx == document.tr069.tr069_itf.options[i].value )
			document.tr069.tr069_itf.selectedIndex = i;
	}
	<% DisplayTR069WAN(); %>
	<% initPage("tr069Config"); %>
</script>
<br><br>
</body>
</html>
