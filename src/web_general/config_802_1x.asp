<%SendWebHeadStr(); %>
<title><% multilang(LANG_802_1X_TITLE); %> </title>

<SCRIPT>
function saveChanges(obj)
{
	var ad_state_map = 0;
	var port_based_auth = 0;

	with (document.forms[0]) {
	
		if (port_based_authentication.checked == true)
			port_based_auth = 1;
		else
			port_based_auth = 0;

		portBasedAuthEnabled.value = port_based_auth;
	
		if (checkHostIP(radiusIP, 1) == false)
			return false;
			
		if (radiusPort.value == "") {
			alert("RADIUS Server port number cannot be empty! It should be a decimal number between 1-65535.");
			radiusPort.focus();
			return false;
		}
		
		port = parseInt(radiusPort.value, 10);
		if (port > 65535 || port < 1) {
			alert("Invalid port number of RADIUS Server! It should be a decimal number between 1-65535.");
			radiusPort.focus();
			return false;
		}
		
		for (var i = 0; i < 4; i++) {
			if (admin_state[i].value == 1){
				ad_state_map |= (0x1 << i);
			}
		}
		if (ad_state_map == 0xf) {
			alert("Administrative State can't be set Auto in same time");
			return false;
		}
		adminStatePortmap.value = ad_state_map;
		
		//alert("radius IP: " + radiusIP.value + "\n" + "radius Port: " + radiusPort.value + "\n" + "radius Pass: " + radiusPass.value + "\n" + "administrative state: " + ad_state_map + "\n" + "port_based_auth: " + port_based_auth);
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function on_submit()
{
	document.forms[0].refresh.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_802_1X_TITLE); %> </p>
	<p class="intro_content"> <% multilang(LANG_SUPPORT_LAN_PORT_802_1X); %></p>
</div>

<form action=/boaform/formConfig_802_1x method=POST name="config_802_1x">
<div class="column clearfix">
        <div class="column_title">
                <div class="column_title_left"></div>
                <p><% multilang(LANG_CONFIGURATION); %></p>
                <div class="column_title_right"></div>
        </div>
</div>
<div class="data_common">
<table id = "show_8021x">
	<tr>
		<th width="30%"><% multilang(LANG_PORT_BASED_AUTHENTICATION); %>:</th>
	<td width="70%"><input type="checkbox" name="port_based_authentication" value="ON"></td>
	</tr>
	<tr>
                <th width="30%"><% multilang(LANG_RADIUS_IP); %>:</th>
                <td width="70%"><input id="radius_ip" name="radiusIP" size="15" maxlength="15" value="0.0.0.0"></td>
        </tr>
	<tr>
		<th width="30%"><% multilang(LANG_RADIUS_UDP_PORT); %>:</th>
		<td width="70%"><input id="radius_port" name="radiusPort" size="15" maxlength="5" value="0"></td>
	</tr>
	<tr>
                <th width="30%"><% multilang(LANG_RADIUS_SECRET); %>:</th>
                <td width="70%"><input type="password" name="radiusPass" size="15" maxlength="15" value="\0"></td>
        </tr>
</table>
</div>
<div class="column clearfix">
        <div class="column_title">
                <div class="column_title_left"></div>
                <p><% multilang(LANG_PORT_TABLE); %></p>
                <div class="column_title_right"></div>
        </div>
</div>
<div class="data_common">
<table>
	<tr>
                <th width="10%"><% multilang(LANG_LAN); %></th>
                <th width="15%"><% multilang(LANG_ADMINISTRATIVE_STATE); %></th>
                <th width="15%"><% multilang(LANG_PORT_STATE); %></th>
	</tr>
	<tr>
                <td width="5">1</td>
                <td width="15%">
			<select name="admin_state">
				<option selected value ="0"><% multilang(LANG_FORCE_AUTHORIZED); %></option>
				<option value="1"><% multilang(LANG_AUTO); %></option>
			</select>
		</td>
                <td width="10%"><% getPortStatus("port1"); %></td>
        </tr>
	<tr>
                <td width="5">2</td>
                <td width="15%">
                        <select name="admin_state">
                                <option selected value ="0"><% multilang(LANG_FORCE_AUTHORIZED); %></option>
                                <option value="1"><% multilang(LANG_AUTO); %></option>
                        </select>
                </td>
                <td width="10%"><% getPortStatus("port2"); %></td>
        </tr>
	<tr>
                <td width="5">3</td>
                <td width="15%">
                        <select name="admin_state">
                                <option selected value ="0"><% multilang(LANG_FORCE_AUTHORIZED); %></option>
                                <option value="1"><% multilang(LANG_AUTO); %></option>
                        </select>
                </td>
                <td width="10%"><% getPortStatus("port3"); %></td>
        </tr>
	<tr>
                <td width="5">4</td>
                <td width="15%">
                        <select name="admin_state">
                                <option selected value="0"><% multilang(LANG_FORCE_AUTHORIZED); %></option>
                                <option value="1"><% multilang(LANG_AUTO); %></option>
                        </select>
                </td>
                <td width="10%"><% getPortStatus("port4"); %></td>
        </tr>
</table>
</div>
<div class="btn_ctl">
      <input class="link_bg" type=submit value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges(this)">
      <input type=hidden value="/config_802_1x.asp" name="submit-url">
      <input class="link_bg" type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit()">&nbsp;&nbsp;
      <input type="hidden" name="portBasedAuthEnabled" value=0>
      <input type="hidden" name="adminStatePortmap" value=0>
      <input type="hidden" name="postSecurityFlag" value="">
</div>

<script>
	<% initPage("config_802_1x"); %>
</script>
</form>
</body>

</html>
